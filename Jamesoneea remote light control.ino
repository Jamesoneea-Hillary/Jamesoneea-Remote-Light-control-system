#include <WiFi.h>
#include <WebServer.h>

// ---------------------- USER SETTINGS ----------------------
const char *ssid = "Jamesoneea";
const char *password = "#jamesoneea@tech.com";

const char *ADMIN_USER = "James Hillary";
const char *ADMIN_PASS = "hillary@okello";

const char *BRAND_NAME = "JAMESONEEA REMOTE LIGHT CONTROL SYSTEM";
const char *NAME = "OKELLO JAMES HILLARY";

// GPIO pins for 3 lights (update to match wiring)
int ledPins[3] = {2, 4, 5}; // example: onboard LED=2, others=4,5
// -----------------------------------------------------------

WebServer server(80);

// Track LED states
bool ledState[3] = {false, false, false};

// Simple in-memory login session (cleared if ESP32 resets)
bool loggedIn = false;

// ---------- HTML Helpers ----------
String htmlHeader(const String &title)
{
    String h = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
    h += "<title>" + title + "</title>";
    h += "<style>";
    h += "body{font-family:Arial;text-align:center;background:#f0f8ff;padding:20px;}"; // very light blue background
    h += "h1{margin-bottom:10px;color:#333;}";
    h += ".btn{padding:12px 20px;font-size:18px;margin:8px;border:none;border-radius:8px;cursor:pointer;}";
    h += ".on{background:#4CAF50;color:white;}.off{background:#f44336;color:white;}"; // green/red buttons
    h += ".logout{background:#555;color:white;font-size:14px;padding:6px 12px;margin-top:20px;}";
    h += "form{display:inline-block;background:#f5f5f5;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,.1);}"; // grey form background
    h += "input[type=text],input[type=password]{width:200px;padding:8px;margin:5px;border:1px solid #ccc;border-radius:4px;font-size:16px;}";
    h += "input[type=submit]{width:220px;margin-top:10px;background:#5a5a5a;color:white;border:none;padding:10px;border-radius:4px;cursor:pointer;}"; // grey submit button
    h += "table{margin:0 auto;margin-top:20px;border-collapse:collapse;font-size:20px;background:#f5f5f5;border-radius:10px;padding:15px;}";          // grey table background
    h += "td{padding:10px 15px;border-bottom:1px solid #ddd;}";
    h += "tr:last-child td{border-bottom:none;}";
    h += "</style></head><body>";
    return h;
}

String htmlFooter()
{
    String f = "<footer style='margin-top:30px;font-size:14px;color:#555;'>Powered by ";
    f += NAME;
    f += "</footer></body></html>";
    return f;
}

// ---------- Login Page ----------
String loginPage(const String &msg = "")
{
    String html = htmlHeader("Login");
    html += "<h1>" + String(BRAND_NAME) + " Login</h1>";
    if (msg.length())
    {
        html += "<p style='color:red;'>" + msg + "</p>";
    }
    html += "<form method='POST' action='/login'>";
    html += "<input type='text' name='username' placeholder='Username' required><br>";
    html += "<input type='password' name='password' placeholder='Password' required><br>";
    html += "<input class='btn' type='submit' value='Login'>";
    html += "</form>";
    html += htmlFooter();
    return html;
}

// ---------- Control Dashboard ----------
String controlPage()
{
    String html = htmlHeader("Remote Light Control");
    html += "<h1>Remote Light Control</h1>";
    html += "<p>Welcome, authenticated user.</p>";
    html += "<table>";
    for (int i = 0; i < 3; i++)
    {
        html += "<tr><td>Light " + String(i + 1) + ":</td><td><strong>";
        html += (ledState[i] ? "ON" : "OFF");
        html += "</strong></td><td>";
        if (ledState[i])
        {
            html += "<a href='/off?led=" + String(i) + "'><button class='btn off'>Turn OFF</button></a>";
        }
        else
        {
            html += "<a href='/on?led=" + String(i) + "'><button class='btn on'>Turn ON</button></a>";
        }
        html += "</td></tr>";
    }
    html += "</table>";
    html += "<a href='/logout'><button class='logout'>Logout</button></a>";
    html += htmlFooter();
    return html;
}

// ---------- Auth Check ----------
bool ensureAuthed()
{
    if (!loggedIn)
    {
        server.send(200, "text/html", loginPage());
        return false;
    }
    return true;
}

// ---------- Route Handlers ----------
void handleRoot()
{
    if (!ensureAuthed())
        return;
    server.send(200, "text/html", controlPage());
}

void handleLogin()
{
    // Expect POST
    if (server.method() == HTTP_POST)
    {
        String user = server.arg("username");
        String pass = server.arg("password");
        if (user == ADMIN_USER && pass == ADMIN_PASS)
        {
            loggedIn = true;
            server.sendHeader("Location", "/");
            server.send(303); // redirect
            return;
        }
        else
        {
            server.send(200, "text/html", loginPage("Invalid credentials."));
            return;
        }
    }
    // If GET, just show form
    server.send(200, "text/html", loginPage());
}

void handleLogout()
{
    loggedIn = false;
    server.send(200, "text/html", loginPage("Logged out."));
}

void handleOn()
{
    if (!ensureAuthed())
        return;
    if (!server.hasArg("led"))
    {
        server.send(400, "text/plain", "Missing led parameter");
        return;
    }
    int idx = server.arg("led").toInt();
    if (idx < 0 || idx > 2)
    {
        server.send(400, "text/plain", "Invalid LED index");
        return;
    }
    digitalWrite(ledPins[idx], HIGH);
    ledState[idx] = true;
    server.send(200, "text/html", controlPage());
}

void handleOff()
{
    if (!ensureAuthed())
        return;
    if (!server.hasArg("led"))
    {
        server.send(400, "text/plain", "Missing led parameter");
        return;
    }
    int idx = server.arg("led").toInt();
    if (idx < 0 || idx > 2)
    {
        server.send(400, "text/plain", "Invalid LED index");
        return;
    }
    digitalWrite(ledPins[idx], LOW);
    ledState[idx] = false;
    server.send(200, "text/html", controlPage());
}

void handleNotFound()
{
    server.send(404, "text/plain", "Not Found");
}

// ---------- Setup ----------
void setup()
{
    Serial.begin(115200);
    delay(100);

    // Init LED pins
    for (int i = 0; i < 3; i++)
    {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
        ledState[i] = false;
    }

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // Block until connected
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected.");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // Routes
    server.on("/", handleRoot);
    server.on("/login", handleLogin);
    server.on("/logout", handleLogout);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started.");
}

// ---------- Loop ----------
void loop()
{
    server.handleClient();
}
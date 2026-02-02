#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>

#define LED_PIN 8
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
WebServer server(80);

bool ledOn = false;
String oledMessage = "Hello!";
int ipScrollOffset = 0;
unsigned long lastScrollTime = 0;

void updateOled() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);

    // Status line at top (scrolls if IP too wide)
    String ip = WiFi.localIP().toString();
    int ipWidth = ip.length() * 6;
    if (ipWidth <= 72) {
        u8g2.drawStr(0, 9, ip.c_str());
    } else {
        int gap = 30; // pixel gap between copies
        int totalWidth = ipWidth + gap;
        int x = -ipScrollOffset;
        u8g2.drawStr(x, 9, ip.c_str());
        u8g2.drawStr(x + totalWidth, 9, ip.c_str());
    }

    // LED state
    u8g2.drawStr(0, 20, ledOn ? "LED: ON" : "LED: OFF");

    // Message (wrap if needed)
    u8g2.drawStr(0, 32, oledMessage.substring(0, 12).c_str());
    if (oledMessage.length() > 12) {
        u8g2.drawStr(0, 40, oledMessage.substring(12, 24).c_str());
    }

    u8g2.sendBuffer();
}

const char *PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32-C3 Test</title>
<style>
  body { font-family: sans-serif; max-width: 400px; margin: 40px auto; padding: 0 20px; }
  h1 { font-size: 1.4em; }
  input[type=text] { width: 100%; padding: 8px; box-sizing: border-box; font-size: 1em; }
  button { padding: 10px 20px; font-size: 1em; margin-top: 10px; cursor: pointer; }
  .status { margin: 16px 0; padding: 10px; background: #f0f0f0; border-radius: 4px; }
</style>
</head>
<body>
<h1>ESP32-C3 Test</h1>

<div class="status">
  LED is currently: <strong id="ledstate">%LED%</strong>
</div>

<button onclick="fetch('/led').then(r=>r.text()).then(t=>{document.getElementById('ledstate').innerText=t})">
  Toggle LED
</button>

<h2>OLED Message</h2>
<form action="/msg" method="GET">
  <input type="text" name="t" maxlength="24" placeholder="Type a message..." value="%MSG%">
  <button type="submit">Send to OLED</button>
</form>
</body>
</html>
)rawliteral";

void handleRoot() {
    String page = String(PAGE);
    page.replace("%LED%", ledOn ? "ON" : "OFF");
    page.replace("%MSG%", oledMessage);
    server.send(200, "text/html", page);
}

void handleLed() {
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? LOW : HIGH); // inverted logic
    updateOled();
    server.send(200, "text/plain", ledOn ? "ON" : "OFF");
}

void handleMsg() {
    if (server.hasArg("t")) {
        oledMessage = server.arg("t");
        oledMessage.trim();
        if (oledMessage.isEmpty()) oledMessage = " ";
    }
    updateOled();
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);

    // LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // OFF (inverted)

    // OLED
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 10, "Connecting");
    u8g2.drawStr(0, 22, "WiFi...");
    u8g2.sendBuffer();

    // WiFi - STA mode
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Connecting to '%s'\n", WIFI_SSID);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.printf("  status: %d (attempt %d/30)\n", WiFi.status(), attempts + 1);
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("WiFi FAILED. Final status: %d\n", WiFi.status());
        u8g2.clearBuffer();
        u8g2.drawStr(0, 10, "WiFi FAIL");
        u8g2.drawStr(0, 22, "Check serial");
        u8g2.sendBuffer();
        while (true) delay(1000);
    }

    Serial.printf("Connected! IP: %s  RSSI: %d\n",
        WiFi.localIP().toString().c_str(), WiFi.RSSI());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("Subnet: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("Channel: %d\n", WiFi.channel());
    Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());

    // Web server routes
    server.on("/", handleRoot);
    server.on("/led", handleLed);
    server.on("/msg", handleMsg);
    server.begin();

    Serial.println("Web server started.");
    updateOled();
}

void loop() {
    server.handleClient();

    // Scroll IP if it doesn't fit
    String ip = WiFi.localIP().toString();
    int ipWidth = ip.length() * 6;
    if (ipWidth > 72 && millis() - lastScrollTime > 300) {
        lastScrollTime = millis();
        int gap = 30;
        int totalWidth = ipWidth + gap;
        ipScrollOffset += 2;
        if (ipScrollOffset >= totalWidth) {
            ipScrollOffset = 0;
        }
        updateOled();
    }
}

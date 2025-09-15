#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000
#define TRIGGER_LED_PIN 2  // Onboard LED (adjust if needed)
#define out 19  // Onboard LED (adjust if needed)

PulseOximeter pox;
uint32_t tsLastReport = 0;
WebServer server(80);

const char* ssid = "abcde";
const char* password = "123456789";

// Trigger state
bool triggered = false;
unsigned long triggerTime = 0;

// === HTML with Trigger Notification ===
String getHTML(float bpm, float spo2, bool triggered) {
  String alert = triggered ? "<p style='color:red;font-size:2em;'>⚠️ Triggered!</p>" : "";

  String html = R"====(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>MAX30100 Monitor</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background-color: #f8f8f8;
    }
    h2 {
      background-color: #4CAF50;
      color: white;
      padding: 20px;
      font-size: 2.5em;
    }
    .box {
      margin: 40px auto;
      padding: 30px;
      border-radius: 12px;
      background: white;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
      max-width: 90%;
    }
    h3 {
      font-size: 2.5em;
      margin: 20px 0;
    }
    span {
      font-weight: bold;
      font-size: 2.5em;
      color: #333;
    }
    .alert {
      margin-top: 20px;
    }
  </style>
  <script>
    setInterval(() => {
      fetch('/data')
        .then(r => r.json())
        .then(d => {
          document.getElementById('bpm').innerText = d.bpm;
          document.getElementById('spo2').innerText = d.spo2;
          document.getElementById('alert').innerHTML = d.triggered ? "⚠️ Triggered!" : "";
        });
    }, 1000);
  </script>
</head>
<body>
  <h2>MAX30100 Monitor</h2>
  <div class='box'>
    <h3>BPM: <span id='bpm'>--</span></h3>
    <h3>SpO₂: <span id='spo2'>--</span></h3>
    <div class='alert'><p id='alert'></p></div>
  </div>
</body>
</html>
)====";

  return html;
}

float bpm = 0.0, spo2 = 0.0;

void setup() {
  Serial.begin(115200);

  pinMode(TRIGGER_LED_PIN, OUTPUT);
    pinMode(out, OUTPUT);

  digitalWrite(TRIGGER_LED_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("Receiver IP address: ");
  Serial.println(WiFi.localIP());

  Wire.begin();

  if (!pox.begin()) {
    Serial.println("FAILED to initialize MAX30100");
    while (1);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  server.on("/", []() {
    server.send(200, "text/html", getHTML(bpm, spo2, triggered));
  });

  server.on("/data", []() {
    String json = "{\"bpm\":" + String(bpm, 1) +
                  ",\"spo2\":" + String(spo2, 1) +
                  ",\"triggered\":" + String(triggered ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.on("/trigger", []() {
    Serial.println("📩 Trigger signal received from ESP32-CAM");
    triggered = true;
    triggerTime = millis();
    digitalWrite(TRIGGER_LED_PIN, HIGH);
        digitalWrite(out, HIGH);

    server.send(200, "text/plain", "Signal received");
  });

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  pox.update();

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    bpm = pox.getHeartRate();
    spo2 = pox.getSpO2();
    tsLastReport = millis();
    Serial.printf("BPM: %.1f | SpO2: %.1f\n", bpm, spo2);
  }

  // Auto-reset trigger after 3 seconds
  if (triggered && millis() - triggerTime > 3000) {
    triggered = false;
    digitalWrite(TRIGGER_LED_PIN, LOW);
        digitalWrite(out, LOW);

    Serial.println("Trigger reset");
  }
}

#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial SIM900(7, 8);     // SIM900 TX->7, RX->8
SoftwareSerial gpsSerial(11, 12); // GPS TX->9, RX->10
TinyGPSPlus gps;

const int stick = 6; // sensor pin
const char phone_no[] = "+8801552463360"; // Must include country code (+880 for Bangladesh)

float latitude = 0.0;
float longitude = 0.0;
bool smsSent = false;

void setup() {
  Serial.begin(9600);
  pinMode(stick, INPUT);

  // Start SIM900
  SIM900.begin(9600);
  delay(200);
  SIM900.println("AT"); 
  delay(200);
  SIM900.println("AT+CMGF=1");  // SMS in text mode
  delay(200);

  // Start GPS
  gpsSerial.begin(9600);

  Serial.println("System Ready...");
}

void loop() {
  // Read GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Update latitude & longitude
  if (gps.location.isUpdated()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
  }

  int broke = digitalRead(stick);
  Serial.println(broke);

  // If stick is broken and SMS not sent yet
  if (broke == HIGH && !smsSent) {
    sendSMS(" 🔴 SOS Emergency: Button pressed. Please verify location and Telegram updates immediately.  ");
    smsSent = true;  // prevent repeated messages
  }

  // Reset SMS flag if stick restored
  if (broke == LOW) {
    smsSent = false;
  }

  delay(500);
}

void sendSMS(String text) {
  SIM900.println("AT+CMGF=1");  
  delay(500);
  SIM900.print("AT+CMGS=\"");
  SIM900.print(phone_no);
  SIM900.println("\"");
  delay(500);

  SIM900.println(text);

  if (latitude != 0.0 && longitude != 0.0) {
    String gpsLink = "Google Maps: https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);
    SIM900.println(gpsLink);
  } else {
    SIM900.println(" No GPS data available.");
  }

  delay(500);
  SIM900.write(26); // Ctrl+Z to send
  delay(2000);

  Serial.println("SMS Sent!");
}

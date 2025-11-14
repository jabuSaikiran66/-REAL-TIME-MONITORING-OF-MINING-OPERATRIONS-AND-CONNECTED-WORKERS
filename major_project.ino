#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Wi-Fi Credentials
const char* ssid = "Saikiran ";
const char* password = "saikiran123";

// ThingSpeak Settings
WiFiClient client;
unsigned long myChannelNumber = 3115250;
const char* myWriteAPIKey = "4C6CV5GZKM4235BI";

// DHT Sensor Setup
#define DHTPIN D5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Other Sensor Pins
const int gasSensorPin = A0;    // MQ-2 Gas Sensor
const int flameSensorPin = D0;  // Flame Sensor
const int buzzerPin = D2;       // Buzzer
const int alertButtonPin = D3;  // Manual Alert Button

// GPS Setup
SoftwareSerial gpsSerial(D7, D8); // RX, TX
TinyGPSPlus gps;

// Thresholds
int gasThreshold = 1600;         // Adjust based on calibration

void setup() {
  Serial.begin(115200);
  Serial.println("Multi-Sensor Safety System Initializing...");

  dht.begin();
  gpsSerial.begin(9600);

  pinMode(flameSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(alertButtonPin, INPUT_PULLUP); // Button uses internal pull-up

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  delay(2000); // Sensor reading interval

  // Read DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Read MQ-2 Gas Sensor
  int gasValue = analogRead(gasSensorPin);

  // Read Flame Sensor
  int flameValue = digitalRead(flameSensorPin);

  // Read Alert Button
  bool alertPressed = digitalRead(alertButtonPin) == LOW;

  // Read GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Print Sensor Readings
  Serial.println("----- Sensor Readings -----");
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\tTemperature: ");
    Serial.print(temperature);
    Serial.println(" *C");
  }

  Serial.print("Gas Sensor Value: ");
  Serial.println(gasValue);

  Serial.print("Flame Sensor Value: ");
  Serial.println(flameValue);

  Serial.print("Alert Button: ");
  Serial.println(alertPressed ? "PRESSED" : "Not pressed");

  // Danger Detection
  if (gasValue > gasThreshold) {
    Serial.println("🔥 Gas Leak Detected!");
    digitalWrite(buzzerPin, HIGH);
  } else if (flameValue == LOW) {
    Serial.println("🔥 Fire Detected!");
    digitalWrite(buzzerPin, HIGH);
  } else if (alertPressed) {
    Serial.println("🚨 Manual Alert Triggered!");
    digitalWrite(buzzerPin, HIGH);

    // Show GPS only during alert
    if (gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();
      Serial.print("📍 Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("📍 Longitude: ");
      Serial.println(longitude, 6);

      // Send GPS to ThingSpeak
      ThingSpeak.setField(6, latitude);
      ThingSpeak.setField(7, longitude);
    } else {
      Serial.println("⚠️ GPS signal not available");
    }
  } else {
    digitalWrite(buzzerPin, LOW);
    Serial.println("✅ Environment Normal");
  }

  // Send Data to ThingSpeak
  ThingSpeak.setField(1, humidity);
  ThingSpeak.setField(2, temperature);
  ThingSpeak.setField(3, gasValue);
  ThingSpeak.setField(4, flameValue);
  ThingSpeak.setField(5, alertPressed ? 1 : 0);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("✅ Data sent to ThingSpeak");
  } else {
    Serial.println("❌ Error sending data: " + String(x));
  }

  Serial.println("---------------------------\n");
}
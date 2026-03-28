#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_system.h"

// --- WiFi Credentials ---
const char* ssid = "Confidential";
const char* password = "Confidential";

// --- Google Sheet Script URL ---
const String scriptURL = "https://script.google.com/macros/s/AKfycbyKl4DBdCOoAvrCtOe927VQ-HSJe-kjyXI22rLxALK9YNZtSSo9yf7CYm93x9o6C1dokA/exec?status=";

// --- HC12 & GSM Serial Definitions ---
HardwareSerial HC12(1); // GPIO16 (RX), GPIO17 (TX)
HardwareSerial GSM(2);  // GPIO4 (RX), GPIO5 (TX)

// --- Ultrasonic Sensor Pins ---
const int trigPin1 = 13;
const int echoPin1 = 14;
const int trigPin2 = 26;
const int echoPin2 = 27;

// --- LED Pins ---
#define LED_PIN1 33
#define LED_PIN2 32

// --- State Tracking ---
String lastState = "";

void setup() {
  Serial.begin(115200);

  HC12.begin(9600, SERIAL_8N1, 16, 17);
  GSM.begin(115200, SERIAL_8N1, 4, 5);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);

  // WiFi connect
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected");

  sendSMS("System Started");
  logToGoogleSheet("System Started");
}

void loop() {
  float distance1 = getDistance(trigPin1, echoPin1);
  float distance2 = getDistance(trigPin2, echoPin2);

  // Skip if both read 0.00
  if (distance1 == 0.0 && distance2 == 0.0) return;


  String currentState = "";

  // If either sensor triggers ON condition
  if (distance1 > 80.0 || distance2 > 80.5) {
    currentState = "ON";
  }
  // If either sensor triggers OFF condition
  else if (distance1 < 35.0 || distance2 < 35.0) {
    currentState = "OFF";
  }

  // LED and Serial Display
  if (distance1 > 80.5) {
    digitalWrite(LED_PIN1, HIGH);
    //Serial.println("Water flowing to Tank 1");
  } else {
    digitalWrite(LED_PIN1, LOW);
  }

  if (distance2 > 80.5) {
    digitalWrite(LED_PIN2, HIGH);
    //Serial.println("Water flowing to Tank 2");
  } else {
    digitalWrite(LED_PIN2, LOW);
  }

  // Send only if state has changed
  if (currentState != "" && currentState != lastState) {
    HC12.println(currentState);
    Serial.print("Sent: "); Serial.println(currentState);
    sendSMS("Turning " + currentState + " Motor");
    logToGoogleSheet(currentState);
    lastState = currentState;
  }

  receiver(); // Check for incoming HC12 data
  delay(500);
}

// --- Get Distance ---
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  float distance = duration * 0.034 / 2;
  return distance;
}

// --- Send SMS ---
void sendSMS(String message) {
  GSM.println("AT+CMGF=1");
  delay(500);
  GSM.println("AT+CMGS=\"+919884338903\"");
  delay(500);
  GSM.print(message);
  GSM.write(26);
  delay(3000);
  Serial.println("SMS Sent: " + message);
}

// --- Log to Google Sheet ---
void logToGoogleSheet(String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullURL = scriptURL + status;
    http.begin(fullURL);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Logged to Google Sheet");
    } else {
      Serial.println("Log Failed: HTTP Code " + String(httpCode));
    }
    http.end();
  } else {
    Serial.println("Wi-Fi Disconnected!");
  }
}

// --- HC12 Data Receiver ---
void receiver() {
  while (HC12.available()) {
    String received = HC12.readStringUntil('\n');
    received.trim();
    Serial.print("Received: ");
    Serial.println(received);
    sendSMS(received);
    if (received == "No water in sump."){
      
      delay(10000);
      Serial.print("Turning off. Bye");
      esp_restart();
    }
  }
}

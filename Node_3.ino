#include <HardwareSerial.h>

#define LED_PIN 26
#define TRIG_PIN 2
#define ECHO_PIN 4
#define LORA_TX 16
#define LORA_RX 17

HardwareSerial HC12(2);
int distance;
String received = "";
int count;
bool command = false;

void setup() {
  Serial.begin(115200);
  HC12.begin(9600, SERIAL_8N1, LORA_TX, LORA_RX);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("System is ready...");
}

void loop() {
  
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  int duration = pulseIn(ECHO_PIN, HIGH, 25000);
  distance = duration * 0.034 / 2;

  Serial.println(distance);
  delay(2000);

  if (HC12.available()) {
    received = HC12.readStringUntil('\n');
    received.trim();
    Serial.print("RECEIVED: ");
    Serial.println(received);

    if (received.equalsIgnoreCase("No water in well.")) {
      command = true;
      count = 0;
    } 
    else if (received.equalsIgnoreCase("OFF")) {
      command = false;
      digitalWrite(LED_PIN, LOW);
      HC12.println("Motor turned off through command.");
      HC12.print("Water level: ");
      HC12.println(((220 - distance) / 220) * 100);
    }
  }

  if (distance > 165 && command == true) {
    digitalWrite(LED_PIN, HIGH);
    sendon();
  }
  else if (distance < 165) {
    digitalWrite(LED_PIN, LOW);
    sendoff();
  }
}

void sendon() {
  if (count == 0) {
    Serial.print("Water is available in sump. Water Level: ");
    HC12.println("Water is available in sump. Water Level: ");
    HC12.println(((220 - distance) / 220) * 100);
    count++;
  }
}

void sendoff() {
  if (count == 1) {
    Serial.print("No water in sump. Water Level: ");
    HC12.println("No water in sump.");
    HC12.print("Water Level: ");
    HC12.println(((220 - distance) / 220) * 100);
    count++;
  }
}
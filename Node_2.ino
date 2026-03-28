#define HC12_RX 16
#define HC12_TX 17
#define LED_PIN 18
#define flowSensorPin 5

float calibrationFactor = 7.5;
float flowRate;
HardwareSerial hc12(2);

bool lastFlowState = LOW;
int pulseCount = 0;

void setup() {
  Serial.begin(115200);
  hc12.begin(9600, SERIAL_8N1, HC12_RX, HC12_TX);
  pinMode(LED_PIN, OUTPUT);
  pinMode(flowSensorPin, INPUT_PULLUP);
  Serial.println("HC-12 & Flow Sensor Initialized");
}

void loop() {
  // Always check for incoming HC-12 commands
  if (hc12.available()) {
    String incoming = hc12.readStringUntil('\n');
    incoming.trim();
    Serial.print("Received: ");
    Serial.println(incoming);

    if (incoming.equalsIgnoreCase("ON")) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Motor ON: Checking flow for 5 seconds...");
      checkInitialFlow();
    } else if (incoming.equalsIgnoreCase("OFF")) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Motor OFF by command");
      hc12.println("Motor turned OFF");
    }
  }
}

void checkInitialFlow() {
  pulseCount = 0;
  unsigned long startTime = millis();

  while (millis() - startTime < 5000) {
    pollFlowSensor();
    delay(1); // small delay to avoid high CPU usage
  }

  flowRate = pulseCount / calibrationFactor;
  Serial.print("Flow Rate: ");
  Serial.print(flowRate);
  Serial.println(" L/min");

  if (flowRate > 2.5) {
    Serial.println("Motor turned ON: Flow detected.");
    hc12.println("Motor turned ON: Flow detected");
    pulseCount = 0;
    monitorFlowUntilStop();
  } else {
    Serial.println("No flow detected. Turning motor OFF.");
    hc12.println("No water in well");
    hc12.println("SUMPON");
    digitalWrite(LED_PIN, LOW);
  }
}

void monitorFlowUntilStop() {
  while (true) {
    // Check for new HC-12 command
    if (hc12.available()) {
      String incoming = hc12.readStringUntil('\n');
      incoming.trim();
      Serial.print("Received during monitoring: ");
      Serial.println(incoming);

      if (incoming.equalsIgnoreCase("OFF")) {
        Serial.println("Motor OFF command received during monitoring");
        hc12.println("Motor turned OFF");
        digitalWrite(LED_PIN, LOW);
        break; // Exit monitoring loop
      }
    }

    pulseCount = 0;
    unsigned long checkTime = millis();
    while (millis() - checkTime < 1000) {
      pollFlowSensor();
      delay(1);
    }

    flowRate = pulseCount / calibrationFactor;
    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.println(" L/min");

    if (flowRate < 2.5) {
      Serial.println("Flow stopped. Motor turned OFF");
      hc12.println("Flow stopped. Motor turned OFF");
      hc12.println("SUMPON");
      digitalWrite(LED_PIN, LOW);
      break;
    }
  }
}

void pollFlowSensor() {
  bool currentState = digitalRead(flowSensorPin);
  if (currentState == HIGH && lastFlowState == LOW) {
    pulseCount++;
  }
  lastFlowState = currentState;
}
/*
  ESP32 publish telemetry data to V-One Cloud (Home Security System with PIR Sensor, Gas/Smoke Detection, and LED Control)
*/

#include "VOneMqttClient.h"
#include <Adafruit_NeoPixel.h>

// Define constants
#define NUMPIXELS 1
#define DELAYVAL 500
#define GAS_THRESHOLD 1000  // Gas detection threshold

// Define device IDs (Replace with your V-One device IDs)
const char* LEDLight = "c9f1e91c-03d5-4761-955d-984c08fa0501"; // LED
const char* PIRsensor = "c8209e25-1416-456e-96c5-0f704c875ec8"; // PIR sensor
const char* GasSensor = "21dc653d-cfc7-4491-a2ee-0c8e16eafaa9"; // MQ2 sensor

// Pin assignments
const int ledPin = 21;       // LED Pin
const int motionSensor = 4;  // PIR sensor pin
const int mq2Pin = A2;       // MQ2 sensor analog pin
const int neoPin = 46;       // Onboard NeoPixel pin

// Timer variables for PIR sensor
unsigned long lastTrigger = 0;
boolean PIRvalue = false;

// Create NeoPixel instance
Adafruit_NeoPixel pixels(NUMPIXELS, neoPin, NEO_GRB + NEO_KHZ800);

// Create V-One client instance
VOneMqttClient voneClient;

// Last message time
unsigned long lastMsgTime = 0;

// Motion detection interrupt handler
void IRAM_ATTR detectsMovement() {
  PIRvalue = true;
  lastTrigger = millis();
}

// Wi-Fi setup function
void setup_wifi() {
  delay(10);
  Serial.println("\nConnecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Callback function to control LED via V-One Cloud
void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand) {
  Serial.print("Main received callback : ");
  Serial.print(actuatorDeviceId);
  Serial.print(" : ");
  Serial.println(actuatorCommand);

  if (String(actuatorDeviceId) == LEDLight) {
    JSONVar commandObj = JSON.parse(actuatorCommand);
    bool commandValue = (bool)commandObj["state"];
    digitalWrite(ledPin, commandValue);
    Serial.println(commandValue ? "LED ON" : "LED OFF");
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, true);
  }
}

// Setup function
void setup() {
  Serial.begin(115200);

  // Setup Wi-Fi and V-One client
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback);

  // Initialize components
  pinMode(ledPin, OUTPUT);
  pinMode(motionSensor, INPUT);
  digitalWrite(ledPin, LOW);
  pixels.begin();
  pixels.clear();

  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  Serial.println("System initialized");
}

// Main loop
void loop() {
  if (!voneClient.connected()) {
    voneClient.reconnect();
    voneClient.publishDeviceStatusEvent(PIRsensor, true);
    voneClient.publishDeviceStatusEvent(GasSensor, true);
  }
  voneClient.loop();

  // Handle periodic telemetry updates
  unsigned long cur = millis();
  if (cur - lastMsgTime > 1000) {  // Adjust the interval as needed
    lastMsgTime = cur;

    // Read sensors
    PIRvalue = digitalRead(motionSensor);
    int gasValue = analogRead(mq2Pin);

    // Publish telemetry data to V-One Cloud
    voneClient.publishTelemetryData(PIRsensor, "Motion", PIRvalue);
    voneClient.publishTelemetryData(GasSensor, "Gas detector", gasValue);

    // Handle LED behavior based on sensor data
    if (PIRvalue || gasValue > GAS_THRESHOLD) {
      Serial.println("Warning: Motion or high gas/smoke detected!");
      digitalWrite(ledPin, HIGH);  // Turn on LED
      pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red for alert
      pixels.show();
    } else {
      digitalWrite(ledPin, LOW);  // Turn off LED
      pixels.clear();
      pixels.show();
    }
  }
}

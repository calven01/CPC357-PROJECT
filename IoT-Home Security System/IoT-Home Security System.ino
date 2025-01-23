/*
  ESP32 publish telemetry data to VOne Cloud (Home Security System with PIR Sensor and Gas/Smoke Detection)
*/

#include "VOneMqttClient.h"
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 1
#define DELAYVAL 500

// Define device IDs
const char* LEDLight = "c9f1e91c-03d5-4761-955d-984c08fa0501";      // LED
const char* PIRsensor = "c8209e25-1416-456e-96c5-0f704c875ec8";     // PIR sensor
const char* GasSensor = "3765c65a-723e-48ba-9b26-565556a19167";     // Replace with your MQ2 sensor device ID

// Used Pins
const int ledPin = 21;            // LED (Pin 21)
const int motionSensor = 4;       // PIR sensor (Left Maker Port)
const int neoPin = 46;            // Onboard Neopixel
const int mq2Pin = A2;            // MQ2 sensor analog pin

// Input sensor variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean PIRvalue = false;

Adafruit_NeoPixel pixels(NUMPIXELS, neoPin, NEO_GRB + NEO_KHZ800);

// Motion detection interrupt function
void IRAM_ATTR detectsMovement() {
  PIRvalue = true;
  lastTrigger = millis();
}

// Create an instance of VOneMqttClient
VOneMqttClient voneClient;

// Last message time
unsigned long lastMsgTime = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand) {
  Serial.print("Main received callback : ");
  Serial.print(actuatorDeviceId);
  Serial.print(" : ");
  Serial.println(actuatorCommand);

  String errorMsg = "";
  JSONVar commandObjct = JSON.parse(actuatorCommand);
  JSONVar keys = commandObjct.keys();

  if (String(actuatorDeviceId) == LEDLight) {
    String key = "";
    bool commandValue = "";
    for (int i = 0; i < keys.length(); i++) {
      key = (const char*)keys[i];
      commandValue = (bool)commandObjct[keys[i]];
      Serial.print("Key : ");
      Serial.println(key.c_str());
      Serial.print("value : ");
      Serial.println(commandValue);
    }

    if (commandValue == true) {
      Serial.println("LED ON");
      digitalWrite(ledPin, true);
    } else {
      Serial.println("LED OFF");
      digitalWrite(ledPin, false);
    }

    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, true);
  }
}

void setup() {
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback);

  // Sensor initialization
  pinMode(ledPin, OUTPUT);
  pinMode(motionSensor, INPUT);
  pinMode(mq2Pin, INPUT);
  digitalWrite(ledPin, LOW);

  // Set motionSensor pin as interrupt
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
}

void loop() {
  if (!voneClient.connected()) {
    voneClient.reconnect();
    voneClient.publishDeviceStatusEvent(PIRsensor, true);
    voneClient.publishDeviceStatusEvent(GasSensor, true);
  }
  voneClient.loop();

  unsigned long cur = millis();
  if (cur - lastMsgTime > INTERVAL) {
    lastMsgTime = cur;

    // Publish telemetry data
    int PIRvalue = digitalRead(motionSensor);
    int ledValue = digitalRead(ledPin);
    int gasValue = analogRead(mq2Pin);  // Read gas sensor value

    if (ledValue == HIGH) {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 100, 0));
        pixels.show();
        delay(DELAYVAL);
        pixels.setPixelColor(i, pixels.Color(100, 0, 0));
        pixels.show();
        delay(DELAYVAL);
        pixels.setPixelColor(i, pixels.Color(0, 0, 100));
        pixels.show();
        delay(DELAYVAL);
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();
        delay(DELAYVAL);
      }
    } else {
      pixels.clear();
    }

    voneClient.publishTelemetryData(PIRsensor, "Motion", PIRvalue);
    voneClient.publishTelemetryData(GasSensor, "GasLevel", gasValue);  // Publish gas level to VOne Cloud

    // Take action if gas value exceeds a threshold
    if (gasValue > 500) {  // Adjust threshold as needed
      Serial.println("Warning: High gas/smoke detected!");
      digitalWrite(ledPin, HIGH);  // Trigger LED or alarm
    } else {
      digitalWrite(ledPin, LOW);
    }

    delay(1000);
  }
}

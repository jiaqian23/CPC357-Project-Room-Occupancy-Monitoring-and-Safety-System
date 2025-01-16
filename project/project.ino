// Library for VOne MQTT client to communicate with V-One IoT platform
#include "VOneMqttClient.h"

// Define device id
const char* relay = "92073075-2be1-4877-a13f-b1d9afccee2c"; // Relay module for fan
const char* MQ2 = "1ce0cc80-7d0e-4a61-b8f5-c625b8a2e07c"; // Gas sensor
const char* LEDYellow = "bcb0a97e-ebfd-465f-ad17-4d1af280d452"; // Yellow LED
const char* LEDGreen = "06e95dab-4a9d-4b8c-b193-6069b9ed8ebe"; // Green LED
const char* LEDRed = "56875643-4336-4d8f-a170-c0f9af0e396a"; // Red LED
const char* irEnt = "92cbe05b-8fbe-449c-b823-6fc0c41eb556"; // IR sensor

// Pin assignments
const int irEntrance = 7; // IR sensor for entrance
const int irExit = 42; // IR sensor for exit
const int ledGreenPin = 9; // Green LED
const int ledRedPin = 10; // Red LED
const int piezoBuzzer = 12; // Piezo buzzer
const int externalBuzzer = 5;  // External buzzer
const int relayFan = 39; // Relay module for fan
const int ledYellowPin = 4; // Yellow LED
const int gasSensor = 6; // Gas sensor

// Variables to keep track of occupancy
int currentOccupancy = 0; // Current number of people in the room
const int maxOccupancy = 10; // Maximum room capacity

// State variables to track sensor activity
bool entranceTriggered = false; // State of the entrance IR sensor
bool exitTriggered = false; // State of the exit IR sensor

// Variable to store gas sensor reading
float gasValue = 0;

//Create an insance of VOneMqttClient
VOneMqttClient voneClient;

//last message time
unsigned long lastMsgTime = 0;


// Setup Wi-Fi Connection
void setup_wifi() {

  delay(10);
  // Connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


// Trigger Actuator Callback
void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand)
{
  Serial.print("Main received callback : ");
  Serial.print(actuatorDeviceId);
  Serial.print(" : ");
  Serial.println(actuatorCommand);

  String errorMsg = "";

  JSONVar commandObjct = JSON.parse(actuatorCommand);
  JSONVar keys = commandObjct.keys();

    String key = "";
    JSONVar commandValue = "";
    for (int i = 0; i < keys.length(); i++) {
      key = (const char* )keys[i];
      commandValue = commandObjct[keys[i]];

    }

    Serial.print("Key : ");
    Serial.println(key.c_str());
    Serial.print("value : ");
    Serial.println(commandValue);

  // Check for Relay
  if (String(actuatorDeviceId) == relay) {
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
      Serial.println("Relay ON");
      digitalWrite(relayFan, true); // Turn on fan
    } else {
      Serial.println("Relay OFF");
      digitalWrite(relayFan, false); // Turn off fan
    }

    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, true);
  
  }

  // Check for LED yellow
  if (String(actuatorDeviceId) == LEDYellow) {
    //{"LEDLight":false}
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
      Serial.println("Light ON");
      digitalWrite(ledYellowPin, true); // Turn on yellow LED
    } else {
      Serial.println("Light OFF");
      digitalWrite(ledYellowPin, false); // Turn off yellow LED
    }
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, true);
  }
}


// Main setup
void setup() {
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback);

  pinMode(irEntrance, INPUT);
  pinMode(irExit, INPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(piezoBuzzer, OUTPUT);
  pinMode(externalBuzzer, OUTPUT);
  pinMode(relayFan, OUTPUT);
  pinMode(ledYellowPin, OUTPUT);

  // Start with green LED on (room not full), fan and yellow LED off
  digitalWrite(ledGreenPin, HIGH);
  digitalWrite(ledRedPin, LOW);
  digitalWrite(piezoBuzzer, LOW);
  digitalWrite(externalBuzzer, HIGH);
  digitalWrite(relayFan, LOW);
  digitalWrite(ledYellowPin, LOW);

  Serial.begin(9600); // For debugging

  Serial.println("Gas sensor warming up!");
  delay(20000); // Allow the MQ-2 sensor to warm up
}



// Main loop
void loop() {

  if (!voneClient.connected()) {
    voneClient.reconnect();
    String errorMsg = "Sensor Fail";
    voneClient.publishDeviceStatusEvent(MQ2, true);
    voneClient.publishDeviceStatusEvent(irEnt, true);
  }
  voneClient.loop();

  if (gasValue > 1600) {  // If gas value is high
    tone(piezoBuzzer, 500);  // Trigger piezo buzzer
      delay(5000);  // Buzzer alert duration
  }

  else {
  noTone(piezoBuzzer);
  }


  // Read IR sensors
  bool entranceState = digitalRead(irEntrance);
  bool exitState = digitalRead(irExit);

  // Check entrance sensor
  if (entranceState == HIGH && !entranceTriggered) {
    entranceTriggered = true; // Mark sensor as triggered
    delay(200); // Debounce delay
    if (currentOccupancy < maxOccupancy) { // If room still have space
      currentOccupancy++;
      updateLEDs();
      Serial.print("Person entered. Current occupancy: ");
      Serial.println(currentOccupancy);
    } else {
      Serial.println("Room is full. Cannot enter.");
      digitalWrite(externalBuzzer, LOW);
      delay(300);  // Buzzer alert duration
      digitalWrite(externalBuzzer, HIGH); // Turn off external buzzer
    }
  }
  if (entranceState == LOW && entranceTriggered) {
    entranceTriggered = false; // Reset trigger after event
  }


  // Check exit sensor
  if (exitState == HIGH && !exitTriggered) {
    exitTriggered = true; // Mark sensor as triggered
    delay(200); // Debounce delay

    if (currentOccupancy > 0) {
      currentOccupancy--;
      updateLEDs();
      Serial.print("Person exited. Current occupancy: ");
      Serial.println(currentOccupancy);
    }
  }
  if (exitState == LOW && exitTriggered) {
    exitTriggered = false; // Reset trigger after event
  }


  unsigned long currentMillis = millis();

  if (currentMillis - lastMsgTime >= INTERVAL) {
    
    lastMsgTime = currentMillis;

    // Check gas Sensor 
    gasValue = analogRead(gasSensor); // Read gas sensor value
    Serial.print("Gas Level: ");
    Serial.println(gasValue);

    //Publish telemtry data
    voneClient.publishTelemetryData(MQ2, "Gas detector", gasValue);
    voneClient.publishTelemetryData(irEnt, "Obstacle", currentOccupancy);

  }

}


// Update LED Indicators
void updateLEDs() {
  if (currentOccupancy >= maxOccupancy)  { // Room has space
    digitalWrite(ledGreenPin, LOW); // Turn off green LED
    digitalWrite(ledRedPin, HIGH); // Turn on red LED
  } else { // Room is full
    digitalWrite(ledGreenPin, HIGH); // Turn on green LED
    digitalWrite(ledRedPin, LOW); // Turn off red LED
  }

  if (currentOccupancy > 0) {
    digitalWrite(relayFan, HIGH); // Turn on fan
    digitalWrite(ledYellowPin, HIGH); // Turn on yellow LED
  } else {
    digitalWrite(relayFan, LOW); // Turn off fan
    digitalWrite(ledYellowPin, LOW); // Turn off yellow LED
  }
}
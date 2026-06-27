#include "BluetoothSerial.h"
#include <DHT.h>

#define DHTPIN 4        
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Define the built-in blue LED pin
#define LED_PIN 2 

BluetoothSerial SerialBT;

unsigned long lastSendTime = 0;
const unsigned long interval = 60000;

// Variables for the non-blocking LED blink
unsigned long lastBlinkTime = 0;
bool ledState = false;
bool wasConnected = false;

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Initialize the LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with it turned off
  
  SerialBT.begin("Sensor 1"); 
  Serial.println("Bluetooth Started! Waiting for phone to connect...");
}

void loop() {
  bool isConnected = SerialBT.hasClient();

  // --- STATE 1: WAITING FOR CONNECTION ---
  if (!isConnected) {
    if (wasConnected) {
      Serial.println("Bluetooth disconnected. Waiting for connection...");
      wasConnected = false;
    }

    // Non-blocking blink every 500 milliseconds
    if (millis() - lastBlinkTime >= 500) { 
      lastBlinkTime = millis();
      ledState = !ledState;               // Flip the state
      digitalWrite(LED_PIN, ledState);    // Apply the state to the LED
    }
  } 
  
  // --- STATE 2: CONNECTED ---
  else {
    if (!wasConnected) {
      Serial.println("Phone connected!");
      digitalWrite(LED_PIN, LOW); // Instantly turn off LED upon connection
      wasConnected = true;
    }

    // Check if 5 minutes have passed (or if it's the very first connection)
    if (millis() - lastSendTime >= interval || lastSendTime == 0) {
      lastSendTime = millis();
      
      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();

      if (!isnan(humidity) && !isnan(temperature)) {
        SerialBT.print("Temp: "); SerialBT.print(temperature);
        SerialBT.print("°C | Humidity: "); SerialBT.print(humidity); 
        
        // Using the clean Android line break to prevent the ^M symbol!
        SerialBT.print("%\n"); 
        
        Serial.println("Data logged to phone!");

        // --- STATE 3: DATA TRANSMITTED ---
        // Flash the LED on for half a second as a success indicator
        digitalWrite(LED_PIN, HIGH); 
        delay(500); 
        digitalWrite(LED_PIN, LOW);  
        
      } else {
        SerialBT.print("Sensor read error.\n");
      }
    }
  }
}
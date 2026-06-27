#include "BluetoothSerial.h"
#include <DHT.h>

// --- SENSOR SETUP ---
#define DHTPIN 4        
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- BLUETOOTH SETUP ---
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

// --- DEEP SLEEP SETUP ---
#define uS_TO_S_FACTOR 1000000ULL  
#define TIME_TO_SLEEP  300         // 5 minutes (300 seconds)

void setup() {
  Serial.begin(115200);
  
  // 1. Wake up and initialize the sensor
  Serial.println("\n--- WAKING UP FROM DEEP SLEEP ---");
  dht.begin();
  delay(2000); // Give the DHT11 two seconds to stabilize

  // 2. Read Sensor 
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 3. Start Bluetooth Radio with new name
  SerialBT.begin("Sensor 1"); 
  Serial.println("Bluetooth Started! Waiting 15 seconds for phone to connect...");

  // Wait up to 15 seconds for the app to connect
  int timeout = 0;
  while (!SerialBT.hasClient() && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  // 4. Send Data (If Phone is Connected)
  if (SerialBT.hasClient()) {
    Serial.println("\nPhone connected! Sending telemetry...");
    
    // This sends the data directly to your phone app
    if (!isnan(humidity) && !isnan(temperature)) {
      SerialBT.print("Temp: "); SerialBT.print(temperature);
      SerialBT.print("°C  |  Humidity: "); SerialBT.print(humidity); SerialBT.println("%");
      delay(1000); // Give the Bluetooth radio 1 second to finish transmitting
    } else {
      SerialBT.println("Failed to read from DHT sensor.");
    }
  } else {
    Serial.println("\nNo Bluetooth connection found. Skipping data transmission.");
  }

  // 5. Go back to sleep
  Serial.println("Task complete. Going to deep sleep for 5 minutes...");
  Serial.flush(); 
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // Remains empty due to Deep Sleep
}
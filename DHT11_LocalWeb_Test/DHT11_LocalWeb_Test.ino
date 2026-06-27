#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// --- SENSOR SETUP ---
#define DHTPIN 4        
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- DEEP SLEEP SETUP ---
#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  900         // 15 minutes (900 seconds)

// --- NETWORK & CLOUD DATABASE SETUP ---
const char* ssid = "RETUYA WIFI";
const char* password = "MRR202307"; 

// Replace YOUR_CLOUD_PROJECT_ID with the one from your Supabase dashboard
String serverName = "https://YOUR_CLOUD_PROJECT_ID.supabase.co/rest/v1/climate_logs";

// Paste your new CLOUD publishable (anon) key here
String supabaseKey = "YOUR_CLOUD_PUBLISHABLE_KEY"; 

void setup() {
  Serial.begin(115200);
  
  // 1. Wake up and initialize the sensor
  Serial.println("\n--- WAKING UP FROM DEEP SLEEP ---");
  dht.begin();
  
  // Give the DHT11 two seconds to stabilize its internal readings after booting
  delay(2000); 

  // 2. Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  // 3. Read Sensor & Send Data (Only if Wi-Fi connected)
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected!");
    
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature)) {
      Serial.print("Temp: "); Serial.print(temperature);
      Serial.print("°C  |  Humidity: "); Serial.print(humidity); Serial.println("%");

      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("apikey", supabaseKey);
      http.addHeader("Authorization", "Bearer " + supabaseKey);
      http.addHeader("Prefer", "return=minimal");

      // UPDATED: Sending node_number as an integer (no quotes) instead of a text location
      String httpRequestData = "{\"node_number\":1, \"temperature_c\":" + String(temperature) + ", \"humidity\":" + String(humidity) + "}";
      
      int httpResponseCode = http.POST(httpRequestData);
      
      if (httpResponseCode > 0) {
        Serial.print("Supabase Response: "); Serial.println(httpResponseCode); // Expecting 201
      } else {
        Serial.print("HTTP Error: "); Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("Failed to read from DHT sensor.");
    }
  } else {
    Serial.println("\nFailed to connect to Wi-Fi. Skipping data send.");
  }

  // 4. Go back to sleep
  Serial.println("Task complete. Going to deep sleep for 15 minutes...");
  Serial.flush(); // Ensure all serial messages are printed before shutting down
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // Deep sleep means the ESP32 reboots on wake, so this stays empty!
}
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// --- SENSOR SETUP ---
#define DHTPIN 4        // Green wire connected to D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- NETWORK & DATABASE SETUP (FILL THESE IN) ---
const char* ssid = "RETUYA WIFI-5G";
const char* password = "MRR202307";

// Replace 192.168.1.X with your PC's actual IPv4 address
String serverName = "http://192.168.100.47/rest/v1/climate_logs";

// Paste your long Supabase Anon Key here
String supabaseKey = "sb_publishable_ACJWlzQHlZjBrEguHvfOxg_3BJgxAaH";

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting 30-second data loop...");
}

void loop() {
  // Wait 30 seconds between readings as requested
  delay(30000);

  // Read the sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if reading failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor! Check your wiring.");
    return;
  }

  // Print to Serial Monitor so you can see it working locally
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print("°C  |  Humidity: "); Serial.print(humidity); Serial.println("%");

  // Check Wi-Fi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Initialize HTTP connection
    http.begin(serverName);

    // Provide the required Supabase Headers
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabaseKey);
    http.addHeader("Authorization", "Bearer " + supabaseKey);
    http.addHeader("Prefer", "return=minimal");

    // Construct the JSON Payload string
    String httpRequestData = "{\"node_location\":\"Albuera_Test_Node\", \"temperature_c\":" + String(temperature) + ", \"humidity\":" + String(humidity) + "}";

    // Send the HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);

    // Print the response from the database
    if (httpResponseCode > 0) {
      Serial.print("Supabase Response Code: ");
      Serial.println(httpResponseCode); // A code of 201 means "Created" (Success!)
    } else {
      Serial.print("Error sending to Supabase. HTTP Code: ");
      Serial.println(httpResponseCode);
    }
    
    // Free resources
    http.end();
  } else {
    Serial.println("Wi-Fi Disconnected. Reconnecting...");
    WiFi.reconnect();
  }
}
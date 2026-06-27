#include <WiFiManager.h> 
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 4        
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

String serverName = "http://10.186.103.130:54321/rest/v1/";
String supabaseKey = "sb_publishable_ACJWlzQHlZjBrEguHvfOxg_3BJgxAaH"; 

// ==========================================

int NODE_ID = 3; // Change this for Node 2, 3, etc.

float sleepMinutes = 15.0; 
unsigned long previousMillis = 0;
bool firstRun = true;

void setup() {
  Serial.begin(115200);
  
  delay(1000); 
  Serial.println("\n\n--- WAKING UP (ALWAYS ON MODE) ---");
  
  dht.begin();
  
  WiFiManager wm;
  wm.setConnectTimeout(30);
  wm.setConfigPortalTimeout(180); 
  
  bool connected = wm.autoConnect("Weather Sensor-3  Setup"); 

  if (!connected) {
    Serial.println("Wi-Fi failed or timed out. Rebooting to try again...");
    Serial.flush();
    delay(3000);
    ESP.restart(); 
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi connection lost! Rebooting...");
      delay(1000);
      ESP.restart();
  }

  unsigned long currentMillis = millis();
  unsigned long intervalMs = sleepMinutes * 60 * 1000UL;

  if (firstRun || (currentMillis - previousMillis >= intervalMs)) {
    previousMillis = currentMillis; 
    firstRun = false;

    Serial.println("\n--- RUNNING TELEMETRY CYCLE ---");

    // --- 1. FETCH REMOTE CONFIGURATION FROM SUPABASE ---
    HTTPClient httpGet;
    String getConfigUrl = serverName + "sensor_nodes?node_number=eq." + String(NODE_ID) + "&select=sleep_interval_minutes";
    httpGet.begin(getConfigUrl);
    httpGet.addHeader("apikey", supabaseKey);
    httpGet.addHeader("Authorization", "Bearer " + supabaseKey);
    
    int getCode = httpGet.GET();
    if (getCode == 200) {
      String payload = httpGet.getString();
      JsonDocument doc; 
      deserializeJson(doc, payload);
      if(doc[0].containsKey("sleep_interval_minutes")) {
         sleepMinutes = doc[0]["sleep_interval_minutes"].as<float>();
         Serial.print("Supabase commanded interval: "); Serial.print(sleepMinutes); Serial.println(" mins");
      }
    } else {
      Serial.print("Failed to get config. HTTP Code: "); Serial.println(getCode);
    }
    httpGet.end();

    // --- 2. READ SENSOR & POST TELEMETRY ---
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature)) {
      HTTPClient httpPost;
      httpPost.begin(serverName + "climate_logs");
      httpPost.addHeader("Content-Type", "application/json");
      httpPost.addHeader("apikey", supabaseKey);
      httpPost.addHeader("Authorization", "Bearer " + supabaseKey);
      httpPost.addHeader("Prefer", "return=minimal");

      String postData = "{\"node_number\":" + String(NODE_ID) + ", \"temperature_c\":" + String(temperature) + ", \"humidity\":" + String(humidity) + "}";
      int postCode = httpPost.POST(postData);
      Serial.print("POST Response: "); Serial.println(postCode); 
      httpPost.end();
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }

    Serial.println("Cycle complete. Waiting for next interval...");
  }

  delay(50); 
}
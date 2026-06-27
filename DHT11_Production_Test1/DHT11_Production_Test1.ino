#include <WiFiManager.h> 
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#define DHTPIN 4        
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WDT Timeout (45 seconds). If the board hangs for this long, it reboots.
#define WDT_TIMEOUT 45 

// --- CLOUD SETUP ---
String serverName = "https://rxvixnoiomeldamaiweg.supabase.co/rest/v1/";
String supabaseKey = "sb_publishable_sbqE2jfERC1q252y06R_Tw_UXiQg0Qr"; 
int NODE_ID = 1; // Change this for Node 2, 3, etc.

// --- TIMING VARIABLES ---
int sleepMinutes = 15; // Default interval
unsigned long previousMillis = 0;
bool firstRun = true;

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- WAKING UP (ALWAYS ON MODE) ---");
  dht.begin();
  
  // --- 1. WI-FI SETUP (NO WATCHDOG YET) ---
  WiFiManager wm;
  
  // Give the ESP32 30 seconds to try and connect to a saved Wi-Fi network.
  wm.setConnectTimeout(30);

  // Broadcast the setup portal for 3 minutes (180 seconds)
  wm.setConfigPortalTimeout(180); 
  
  bool connected = wm.autoConnect("Weather Sensor-1 Setup"); 

  if (!connected) {
    Serial.println("Wi-Fi failed or timed out. Rebooting to try again...");
    Serial.flush();
    delay(3000);
    ESP.restart(); 
  }
  
  // --- 2. START WATCHDOG TIMER ---
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000,                
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, 
      .trigger_panic = true                            
  };
  esp_task_wdt_init(&twdt_config);
  esp_task_wdt_add(NULL); 
  esp_task_wdt_reset(); // Pet the dog!
}

void loop() {
  // Constantly tell the watchdog timer we are alive while waiting
  esp_task_wdt_reset(); 

  // Auto-reconnect fallback if the router goes offline
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi connection lost! Rebooting...");
      delay(1000);
      ESP.restart();
  }

  unsigned long currentMillis = millis();
  unsigned long intervalMs = sleepMinutes * 60 * 1000UL;

  // If it's the first run, OR if the interval time has passed...
  if (firstRun || (currentMillis - previousMillis >= intervalMs)) {
    previousMillis = currentMillis; 
    firstRun = false;

    Serial.println("\n--- RUNNING TELEMETRY CYCLE ---");

    // --- 3. FETCH REMOTE CONFIGURATION FROM SUPABASE ---
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
         sleepMinutes = doc[0]["sleep_interval_minutes"].as<int>();
         Serial.print("Supabase commanded interval: "); Serial.print(sleepMinutes); Serial.println(" mins");
      }
    }
    httpGet.end();

    esp_task_wdt_reset(); // Pet the dog during long network requests

    // --- 4. READ SENSOR & POST TELEMETRY ---
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

  // A tiny delay keeps the ESP32 stable while looping
  delay(50); 
}
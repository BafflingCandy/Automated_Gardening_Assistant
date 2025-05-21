#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi Credentials
const char* ssid = "microlab_IoT";
const char* password = "shibboleet";

// MQTT Broker Settings
const char* mqtt_server = "129.12.41.123";
const char* mqtt_user = "gardener";
const char* mqtt_password = "bafflingcandy101";

WiFiClient espClient;
PubSubClient client(espClient);

// Sensor & Pump Pins
const int moistureSensorPin = A3;
const int relayPin = D12;

// Calibration
const int dryValue = 3050;
const int wetValue = 1090;

// Default Values
int moistureThreshold = 50;
int waterTime = 5;
int waitTime = 5;

// Sun Time Variables
RTC_DATA_ATTR time_t sunsetTimestamp = 0;
RTC_DATA_ATTR time_t sunriseTimestamp = 0;

// Weather Data
float latitude = 0;
float longitude = 0;
float precipitation_prob = 0;
bool is_raining = false;

time_t convertISO8601ToTimestamp(String isoTime) {
  // Remove quotes if present
  isoTime.replace("\"", "");

  // Parse ISO 8601 with timezone offset (e.g., "2025-03-28T18:22:12+00:00")
  struct tm timeStruct = {0};
  int year, month, day, hour, minute, second;
  char timezone[6] = {0}; // For +00:00 or -05:00

  // Use sscanf to parse the components (including timezone)
  if (sscanf(isoTime.c_str(), "%d-%d-%dT%d:%d:%d%5s", 
             &year, &month, &day, &hour, &minute, &second, timezone) >= 6) {
    timeStruct.tm_year = year - 1900;
    timeStruct.tm_mon = month - 1;
    timeStruct.tm_mday = day;
    timeStruct.tm_hour = hour;
    timeStruct.tm_min = minute;
    timeStruct.tm_sec = second;
    return mktime(&timeStruct); // Convert to Unix timestamp
  }
  return 0; // Parsing failed
}

// Non-blocking delay function
void nonBlockingDelay(unsigned long duration) {
  unsigned long start = millis();
  while (millis() - start < duration) {
    client.loop();
    delay(10);
  }
}

// Generate Open-Meteo URL with dynamic location
String getWeatherUrl(float lat, float lng) {
  return String("https://api.open-meteo.com/v1/forecast?latitude=") + 
         String(lat, 4) + 
         "&longitude=" + 
         String(lng, 4) + 
         "&hourly=precipitation_probability,rain";
}

// Fetch Weather Data
void updateWeatherData() {
  if (latitude == 0 || longitude == 0) {
    Serial.println("📍 Location not set. Skipping weather update.");
    return;
  }

  String url = getWeatherUrl(latitude, longitude);
  Serial.print("🌍 Fetching weather from: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());

    precipitation_prob = doc["hourly"]["precipitation_probability"][0];
    is_raining = doc["hourly"]["rain"][0] > 0;

    Serial.print("🌧️ Precipitation Probability: ");
    Serial.print(precipitation_prob);
    Serial.println("%");

    Serial.print("🌦️ Is Raining? ");
    Serial.println(is_raining ? "Yes" : "No");
  } else {
    Serial.println("❌ Failed to fetch weather data");
  }

  http.end();
}

// MQTT Callback Function
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("📩 MQTT Message on topic: ");
  Serial.print(topic);
  Serial.print(" → ");
  Serial.println(msg);

    if (String(topic) == "home/sun_next_rising") {
    sunriseTimestamp = convertISO8601ToTimestamp(msg);
    Serial.printf("🌅 Sunrise Timestamp: %lld (Stored)\n", sunriseTimestamp);
  } else if (String(topic) == "home/sun_next_setting") {
    sunsetTimestamp = convertISO8601ToTimestamp(msg);
    Serial.printf("🌇 Sunset Timestamp: %lld (Stored)\n", sunsetTimestamp);
  } else if (String(topic) == "home/moisture_threshold") {
    moistureThreshold = msg.toInt();
  } else if (String(topic) == "home/water_time") {
    waterTime = msg.toInt();
  } else if (String(topic) == "home/wait_time") {
    waitTime = msg.toInt();
  } else if (String(topic) == "home/location") {
    if (msg.indexOf(',') != -1) {  
      int comma = msg.indexOf(',');
      latitude = msg.substring(0, comma).toFloat();
      longitude = msg.substring(comma + 1).toFloat();
      Serial.print("📍 Updated Location: ");
      Serial.print(latitude);
      Serial.print(", ");
      Serial.println(longitude);

      // 🌍 Fetch weather immediately after updating location
      updateWeatherData();
    } else {
      Serial.println("⚠️ Invalid location format received.");
    }
  }
}

// MQTT Reconnection
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println(" connected.");
      client.subscribe("home/sun_next_rising");
      client.subscribe("home/sun_next_setting");
      client.subscribe("home/moisture_threshold");
      client.subscribe("home/water_time");
      client.subscribe("home/wait_time");
      client.subscribe("home/location");
      Serial.println("Subscribed to all necessary topics.");
    } else {
      Serial.println("Failed to connect, retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void enterDeepSleepFromSunsetToSunrise(time_t sunsetTimestamp, time_t sunriseTimestamp) {
    time_t currentTime = time(nullptr);
    
    Serial.printf("[SLEEP LOGIC] Current: %lld | Sunset: %lld | Sunrise: %lld\n",
                  currentTime, sunsetTimestamp, sunriseTimestamp);

    if (currentTime >= sunsetTimestamp && sunriseTimestamp > sunsetTimestamp) {
        uint64_t sleepDuration = (sunriseTimestamp - currentTime) * 1000000ULL;
        Serial.printf("😴 Sleeping from sunset to sunrise (%d minutes)\n",
                      (int)((sunriseTimestamp - currentTime)/60));

        gpio_deep_sleep_hold_dis();
        esp_sleep_enable_timer_wakeup(sleepDuration);
        esp_deep_sleep_start();
    } else {
        Serial.println("⚠️ Invalid sleep window - check timestamps");
    }
}

// Watering Function (Updated with Your Logic)
void waterPlant() {
  while (true) {
    int sensorValue = analogRead(moistureSensorPin);
    int moistureLevel = map(sensorValue, dryValue, wetValue, 0, 100);
    moistureLevel = constrain(moistureLevel, 0, 100);

    if (moistureLevel >= moistureThreshold) {
      Serial.println("✅ Soil moisture reached threshold. No more watering needed.");
      break;  // Exit loop when moisture is sufficient
    }

    Serial.println("🚰 Activating pump...");
    digitalWrite(relayPin, HIGH);
    client.publish("home/pump_state", "ON");

    nonBlockingDelay(waterTime * 1000UL);  

    digitalWrite(relayPin, LOW);
    client.publish("home/pump_state", "OFF");
    Serial.println("🚰 Pump OFF.");

    Serial.println("⏳ Waiting 10 seconds before re-checking soil moisture...");
    nonBlockingDelay(10000UL);
  }
}

// ESP32 Setup
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\n✅ Connected to Wi-Fi");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();

  // Sync time via NTP (UTC)
  configTime(0, 0, "pool.ntp.org"); 
  Serial.println("⌛ Waiting for NTP sync...");
  while (time(nullptr) < 100000) delay(1000); // Wait until synced

  // Debug: Print current Unix time
  Serial.printf("🕒 Current Unix Time: %ld\n", time(nullptr));

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
}

// Main Loop
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int sensorValue = analogRead(moistureSensorPin);
  int moistureLevel = map(sensorValue, dryValue, wetValue, 0, 100);
  moistureLevel = constrain(moistureLevel, 0, 100);

  Serial.print("🌱 Soil Moisture: ");
  Serial.print(moistureLevel);
  Serial.println("%");

  client.publish("home/soil_moisture", String(moistureLevel).c_str());

  if (moistureLevel < moistureThreshold) {
    Serial.println("⚠️ Soil is DRY. Checking watering conditions...");

    // Smart Watering Decision
    if (precipitation_prob > 70 && !is_raining) {
      Serial.println("🌧️ Rain forecasted - Delaying watering.");
      client.publish("home/alert", "🌧️ Rain forecasted - delaying watering.");
    } else {
      Serial.println("💧 Soil is dry, starting watering cycles...");
      waterPlant();
    }
  } else {
    Serial.println("✅ Soil is moist. No need to water.");
  }

  // Check if it's time for deep sleep
  if (sunsetTimestamp > 0 && sunriseTimestamp > 0) {  // ✅ Wait until both timestamps exist
    time_t now = time(nullptr);
        Serial.printf("🕒 Checking Deep Sleep Condition: Now=%lld, Sunset=%lld, Sunrise=%lld\n", now, sunsetTimestamp, sunriseTimestamp);

        if (now >= sunsetTimestamp) {
            Serial.println("🌞 Sun has set, entering deep sleep until sunrise...");
            enterDeepSleepFromSunsetToSunrise(sunsetTimestamp, sunriseTimestamp);  // ✅ Pass global variables
        }
    }

  Serial.print("⏳ Waiting for ");
  Serial.print(waitTime);
  Serial.println(" minutes before next check...");
  nonBlockingDelay(waitTime * 60000UL);
  }


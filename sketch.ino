/*
 * ============================================================
 *  IoT Temperature & Humidity Monitor
 *  Platform  : ESP32 (Wokwi)
 *  Sensor    : DHT22 on GPIO2
 *  Cloud     : ThingSpeak every 15s
 * ============================================================
 */

#include <WiFi.h>              // ESP32 native (replaces ESP8266WiFi.h)
#include <HTTPClient.h>        // ESP32 native (replaces ESP8266HTTPClient.h)
#include <DHT.h>

// ── Configuration ────────────────────────────────────────────
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* TS_API_KEY    = "QOL0DGCZMU837JVE";
const char* TS_URL        = "http://api.thingspeak.com/update";

#define DHT_PIN     2          // GPIO2
#define DHT_TYPE    DHT22
#define TEMP_ALERT  35.0       // °C threshold for serial warning

// ── Objects ──────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ── State ────────────────────────────────────────────────────
unsigned long lastUpload  = 0;
const long    INTERVAL_MS = 1000;
int           uploadCount = 0;

// ============================================================
//  HELPERS
// ============================================================

void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int timeout = 20;
    while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi OK → " + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi FAILED");
    }
}

bool readDHT(float &temp, float &humidity) {
    humidity = dht.readHumidity();
    temp     = dht.readTemperature();

    if (isnan(humidity) || isnan(temp)) {
        Serial.println("[DHT] Read failed — sensor not ready");
        return false;
    }
    return true;
}

void uploadToThingSpeak(float temp, float humidity) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TS] No WiFi — skipping");
        return;
    }

    HTTPClient http;
    String url = String(TS_URL)
                 + "?api_key=" + TS_API_KEY
                 + "&field1="  + String(temp,     2)
                 + "&field2="  + String(humidity, 2);

    Serial.println("[TS] Sending → " + url);
    http.begin(url);           // ESP32 HTTPClient — no WiFiClient needed
    int    code     = http.GET();
    String response = http.getString();
    http.end();

    if (code == 200 && response != "0") {
        uploadCount++;
        Serial.println("[TS] ✓ OK — Entry ID: " + response
                       + "  (total: " + String(uploadCount) + ")");
    } else {
        Serial.println("[TS] ✗ Failed — HTTP " + String(code)
                       + "  response: " + response);
    }
}

void checkAlert(float temp) {
    if (temp >= TEMP_ALERT) {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("⚠  ALERT: Temp = " + String(temp, 1)
                       + "°C  exceeds "    + String(TEMP_ALERT, 0) + "°C!");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    }
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=============================================");
    Serial.println("  IoT Temperature & Humidity Monitor");
    Serial.println("  ESP32 — Wokwi Simulation");
    Serial.println("  Sensor   : DHT22 on GPIO2");
    Serial.println("  Interval : 15 seconds");
    Serial.println("=============================================\n");

    dht.begin();
    delay(2000);

    connectWiFi();
    Serial.println("\nReady — first reading in 15 seconds...\n");
}

// ============================================================
//  LOOP
// ============================================================

void loop() {
    unsigned long now = millis();

    if (now - lastUpload >= INTERVAL_MS) {
        lastUpload = now;

        float temp     = 0.0;
        float humidity = 0.0;

        if (!readDHT(temp, humidity)) {
            Serial.println("[DHT] Retrying in 15s...");
            return;
        }

        Serial.println("─────────────────────────────");
        Serial.println("  Temp     : " + String(temp,     2) + " °C");
        Serial.println("  Humidity : " + String(humidity, 2) + " %");
        Serial.println("─────────────────────────────");

        checkAlert(temp);
        uploadToThingSpeak(temp, humidity);
        Serial.println();
    }
}
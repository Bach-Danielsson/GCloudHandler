#include <Arduino.h>
#include <WiFi.h>
#include <GCloudHandler.h>

const char* ssid     = "your-ssid";
const char* password = "your-password";

const char* IOT_PROJECT_ID = "your-project_id"l
const char* IOT_LOCATION = "us-central1 ";
const char* IOT_REGISTRY_ID = "your-iot-registry";
const char* IOT_DEVICE_ID = "your-device-id";
const char* IOT_PRIVATE_KEY = "your-private-key"; // key like "98:f9:6f:1d:ae:3b:e6:ec:e0:18:bb:a2:a3:5c:9c:ef:84:60:fb:85:04:1f:67:0e:d5:cf:dc:5d:58:29:f0:56"

GCloudHandler gCloudHander(IOT_PROJECT_ID, IOT_LOCATION
, IOT_REGISTRY_ID, IOT_DEVICE_ID, IOT_PRIVATE_KEY);

const unsigned long TELEMETRY_UPDATE_PERIOD = 10000;
unsigned long lastTelemetryUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    gCloudHander.setup();
}

void loop() {
    gCloudHander.loop();
    delay(10);

    if (lastTelemetryUpdate == 0) lastTelemetryUpdate = millis();
    if (millis - lastTelemetryUpdate > TELEMETRY_UPDATE_PERIOD)
        gCloudHander.publishTelemetry("{ \"some-value\": 1024 }");	

}



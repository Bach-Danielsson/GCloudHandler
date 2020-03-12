/*
Google IoT Cloud handler for Espressif controllers (ESP32)
Created by Dennis Gubsky, March 12, 2020
Released into the public domain.
*/
#ifndef __IOT_CLOUD_HANDLER_
#define __IOT_CLOUD_HANDLER_

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <CloudIoTCore.h>

// Defince this if FreeRTOS used in your project. This will run a handler thread.
// If not defined then ::loop() function should be called in cycle
#define GCLOUD_USE_FREERTOS
// Define this if debug output required to Serial
#define __DEBUG

const unsigned long MIN_BACKOFF = 1000;
const unsigned long MAX_BACKOFF = 120000;

class GCloudHandler: public ValuesProvider {
protected:
    // These members may be changed in ::setup() function by external configuration 
    bool CLOUD_ON = true;
    String IOT_PROJECT_ID;
    String IOT_LOCATION;
    String IOT_REGISTRY_ID;
    String IOT_DEVICE_ID;
private:
    // To get the private key run (where private-key.pem is the ec private key
    // used to create the certificate uploaded to google cloud iot):
    // openssl ec -in <private-key.pem> -noout -text
    // and copy priv: part.
    // The key length should be exactly the same as the key length bellow (32 pairs
    // of hex digits). If it's bigger and it starts with "00:" delete the "00:". If
    // it's smaller add "00:" to the start. If it's too big or too small something
    // is probably wrong with your key.
    String IOT_PRIVATE_KEY;

    // To get the certificate for your region run:
    //   openssl s_client -showcerts -connect mqtt.googleapis.com:8883
    // for standard mqtt or for LTS:
    //   openssl s_client -showcerts -connect mqtt.2030.ltsapis.goog:8883
    // Copy the certificate (all lines between and including ---BEGIN CERTIFICATE---
    // and --END CERTIFICATE--) to root.cert and put here on the root_cert variable.
    const char* root_cert;

    String iotJWT = "";
 
    WiFiClientSecure *netClient = NULL;

    CloudIoTCoreDevice *iotDevice = NULL;

    bool useLts = true;

    void logError();
    void logReturnCode();

    time_t iss = 0; 

    TaskHandle_t xLoopTask = NULL;
public:
    MQTTClient *iotMqttClient;
    long lastReconnect = 0;
    unsigned long connectionBackoffTime = MIN_BACKOFF;

    virtual void setup();
#ifndef GCLOUD_USE_FREERTOS
    void loop();
#endif
    void setup();
    void reconnect();

    bool isConnected();

    GCloudHandler(const char* _IOT_PROJECT_ID, const char* _IOT_LOCATION
	, const char* _IOT_REGISTRY_ID, const char* _IOT_DEVICE_ID
	, const char* _IOT_PRIVATE_KEY, const char* _root_cert = NULL);
    virtual ~GCloudHandler();

    virtual void onConnected();
    virtual void onMessage(String &topic, String &payload);
    // Handle command from Cloud
    virtual void onCommand(String& command);
    // Handle configuration update from cloud
    virtual void onConfigUpdate(String& config);

    bool publishTelemetry(String data);
    bool publishTelemetry(const char* data, int length);
    bool publishTelemetry(String subtopic, String data);
    bool publishTelemetry(String subtopic, const char* data, int length);
    // Helper that just sends default sensor
    bool publishState(String data);
    bool publishState(const char* data, int length);

    String getDeviceJWT();
};

#endif /*__IOT_CLOUD_HANDLER_*/

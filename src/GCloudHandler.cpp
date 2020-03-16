/*
Google IoT Cloud handler for Espressif controllers (ESP32)
Created by Dennis Gubsky, March 12, 2020
Released into the public domain.
*/
#include <Arduino.h>
#include "GCloudHandler.h"

#ifdef GCLOUD_USE_FREERTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif /*GCLOUD_USE_FREERTOS*/

static GCloudHandler* __iotHandler = NULL;

// Time (seconds) to expire token += 20 minutes for drift
const int JWT_EXPIRATION_SECS = 3600; // Maximum 24H (3600*24)

// To get the certificate for your region run:
//   openssl s_client -showcerts -connect mqtt.googleapis.com:8883
// for standard mqtt or for LTS:
//   openssl s_client -showcerts -connect mqtt.2030.ltsapis.goog:8883
// Copy the certificate (all lines between and including ---BEGIN CERTIFICATE---
// and --END CERTIFICATE--) to root.cert and put here on the root_cert variable.

const char* __default_root_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIErjCCA5agAwIBAgIQW+5sTJWKajts11BgHkBRwjANBgkqhkiG9w0BAQsFADBU\n"
    "MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRydXN0IFNlcnZpY2VzMSUw\n"
    "IwYDVQQDExxHb29nbGUgSW50ZXJuZXQgQXV0aG9yaXR5IEczMB4XDTE5MDYxMTEy\n"
    "MzE1OVoXDTE5MDkwMzEyMjAwMFowbTELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNh\n"
    "bGlmb3JuaWExFjAUBgNVBAcMDU1vdW50YWluIFZpZXcxEzARBgNVBAoMCkdvb2ds\n"
    "ZSBMTEMxHDAaBgNVBAMME21xdHQuZ29vZ2xlYXBpcy5jb20wggEiMA0GCSqGSIb3\n"
    "DQEBAQUAA4IBDwAwggEKAoIBAQDHuQUoDZWl2155WvaQ9AmhTRNC+mHassokdQK7\n"
    "NxkZVZfrS8EhRkZop6SJGHdvozBP3Ko3g1MgGIZFzqb5fRohkRKB6mteHHi/W7Uo\n"
    "7d8+wuTTz3llUZ2gHF/hrXFJfztwnaZub/KB+fXwSqWgMyo1EBme4ULV0rQZGFu6\n"
    "7U38HK+mFRbeJkh1SDOureI2dxkC4ACGiqWfX/vSyzpZkWGRuxK2F5cnBHqRbcKs\n"
    "OfmYyUuxZjGah+fC5ePgDbAntLUuYNppkdgT8yt/13ae/V7+rRhKOZC4q76HBEaQ\n"
    "4Wn5UC+ShVaAGuo7BtfoIFSyZi8/DU2eTQcHWewIXU6V5InhAgMBAAGjggFhMIIB\n"
    "XTATBgNVHSUEDDAKBggrBgEFBQcDATA4BgNVHREEMTAvghNtcXR0Lmdvb2dsZWFw\n"
    "aXMuY29tghhtcXR0LW10bHMuZ29vZ2xlYXBpcy5jb20waAYIKwYBBQUHAQEEXDBa\n"
    "MC0GCCsGAQUFBzAChiFodHRwOi8vcGtpLmdvb2cvZ3NyMi9HVFNHSUFHMy5jcnQw\n"
    "KQYIKwYBBQUHMAGGHWh0dHA6Ly9vY3NwLnBraS5nb29nL0dUU0dJQUczMB0GA1Ud\n"
    "DgQWBBSKWpFfG/yH1dkkJT05y/ZnRm/M4DAMBgNVHRMBAf8EAjAAMB8GA1UdIwQY\n"
    "MBaAFHfCuFCaZ3Z2sS3ChtCDoH6mfrpLMCEGA1UdIAQaMBgwDAYKKwYBBAHWeQIF\n"
    "AzAIBgZngQwBAgIwMQYDVR0fBCowKDAmoCSgIoYgaHR0cDovL2NybC5wa2kuZ29v\n"
    "Zy9HVFNHSUFHMy5jcmwwDQYJKoZIhvcNAQELBQADggEBAKMoXHxmLI1oKnraV0tL\n"
    "NzznlVnle4ljS/pqNI8LUM4/5QqD3qGqnI4fBxX1l+WByCitbTiNvL2KRNi9xau5\n"
    "oqvsuSVkjRQxky2eesjkdrp+rrxTwFhQ6NAbUeZgUV0zfm5XZE76kInbcukwXxAx\n"
    "lneyQy2git0voUWTK4mipfCU946rcK3+ArcanV7EDSXbRxfjBSRBD6K+XGUhIPHW\n"
    "brk0v1wzED1RFEHTdzLAecU50Xwic6IniM3B9URfSOmjlBRebg2sEVQavMHbzURg\n"
    "94aDC+EkNlHh3pOmQ/V89MBiF1xDHbZZ1gB0GszYKPHec9omSwQ5HbIDV3uf3/DQ\n"
    "his=\n"
    "-----END CERTIFICATE-----\n";  

void __iotMessageReceived(String &topic, String &payload) {
    if (__iotHandler != NULL) {
        __iotHandler->onMessage(topic, payload);
    }
}

GCloudHandler::GCloudHandler(const char* _IOT_PROJECT_ID, const char* _IOT_LOCATION
    , const char* _IOT_REGISTRY_ID, const char* _IOT_DEVICE_ID
    , const char* _IOT_PRIVATE_KEY, const char* _root_cert /*= NULL*/): 
    IOT_PROJECT_ID(_IOT_PROJECT_ID)
    , IOT_LOCATION(_IOT_LOCATION)
    , IOT_REGISTRY_ID(_IOT_REGISTRY_ID)
    , IOT_DEVICE_ID(_IOT_DEVICE_ID)
    , root_cert(_root_cert == NULL ? __default_root_cert : _root_cert)
{
    __iotHandler = this;
    IOT_PRIVATE_KEY[0] = _IOT_PRIVATE_KEY;
    for (int i = 1; i < MAX_PRIVATE_KEYS; i++) IOT_PRIVATE_KEY[i] = "";
}

void GCloudHandler::setConfiguration(const char* _IOT_PROJECT_ID, const char* _IOT_LOCATION
	, const char* _IOT_REGISTRY_ID, const char* _IOT_DEVICE_ID
	, const char* _IOT_PRIVATE_KEY) {
    IOT_PROJECT_ID = _IOT_PROJECT_ID;
    IOT_LOCATION = _IOT_LOCATION;
    IOT_REGISTRY_ID = _IOT_REGISTRY_ID;
    IOT_DEVICE_ID = _IOT_DEVICE_ID;
    IOT_PRIVATE_KEY[0] = _IOT_PRIVATE_KEY;
    for (int i = 1; i < MAX_PRIVATE_KEYS; i++) IOT_PRIVATE_KEY[i] = "";
}

void GCloudHandler::setConfiguration(const char* _IOT_PROJECT_ID, const char* _IOT_LOCATION
    , const char* _IOT_REGISTRY_ID, const char* _IOT_DEVICE_ID
    , const char** _IOT_PRIVATE_KEYS) {
    IOT_PROJECT_ID = _IOT_PROJECT_ID;
    IOT_LOCATION = _IOT_LOCATION;
    IOT_REGISTRY_ID = _IOT_REGISTRY_ID;
    IOT_DEVICE_ID = _IOT_DEVICE_ID;
    int i = 0;
    for (i = 0; i < MAX_PRIVATE_KEYS && _IOT_PRIVATE_KEYS[i] != NULL; i++) {
      IOT_PRIVATE_KEY[i] = _IOT_PRIVATE_KEYS[i];
    }
    for (; i < MAX_PRIVATE_KEYS; i++) IOT_PRIVATE_KEY[i] = "";
}

void GCloudHandler::cleanup() {
  privateKeyIndex = 0;
  if (iotDevice != NULL) delete iotDevice;
  if (iotMqttClient != NULL) { iotMqttClient->disconnect(); delete iotMqttClient; }
  if (netClient != NULL) delete netClient;
  if (xLoopTask != NULL) { vTaskDelete(xLoopTask); xLoopTask = NULL; }
}

GCloudHandler::~GCloudHandler() {
  __iotHandler = NULL;
  cleanup();
}

String GCloudHandler::getDeviceJWT() {
	if (iss == 0 || iotJWT.isEmpty()) {
		iss = time(nullptr);
    tm timeinfo;
    localtime_r(&iss, &timeinfo);
    if (timeinfo.tm_year >= (2019 - 1900) && iotDevice != NULL) {
#ifdef __DEBUG      
      Serial.println("Refreshing JWT");
#endif
      iotDevice->setPrivateKey(IOT_PRIVATE_KEY[privateKeyIndex++].c_str());
      privateKeyIndex %= MAX_PRIVATE_KEYS;
      if (IOT_PRIVATE_KEY[privateKeyIndex].isEmpty()) privateKeyIndex = 0;
      iotJWT = iotDevice->createJWT(iss, JWT_EXPIRATION_SECS);
    }
#ifdef __DEBUG      
    else { Serial.println("NTP time is not updated yet"); }
#endif
	}
	return iotJWT;
}

#ifdef GCLOUD_USE_FREERTOS
void vTaskLoop( void * pvParameters )
{
    GCloudHandler* iotHandler = (GCloudHandler*)pvParameters;
 
    for( ;; ) {
      if (iotHandler->iotMqttClient != NULL) iotHandler->iotMqttClient->loop();
      if (!iotHandler->isConnected())
      {
        iotHandler->connectionBackoffTime *= 2;
        if (iotHandler->connectionBackoffTime > MAX_BACKOFF) iotHandler->connectionBackoffTime = MIN_BACKOFF;

        if (iotHandler->lastReconnect == 0 || millis() - iotHandler->lastReconnect > iotHandler->connectionBackoffTime) {          
          iotHandler->reconnect();
          iotHandler->lastReconnect = millis();
        }
      }
      else iotHandler->connectionBackoffTime = MIN_BACKOFF;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}
#else
void GCloudHandler::loop() {
    if (iotMqttClient != NULL) iotMqttClient->loop();
    if (!isConnected())
    {
        connectionBackoffTime *= 2;
        if (connectionBackoffTime > MAX_BACKOFF) connectionBackoffTime = MIN_BACKOFF;

        if (lastReconnect == 0 || millis() - lastReconnect > connectionBackoffTime) {          
            reconnect();
            lastReconnect = millis();
        }
    }
    else connectionBackoffTime = MIN_BACKOFF;
}
#endif /*GCLOUD_USE_FREERTOS*/

void GCloudHandler::setup() {
  cleanup();
  if (CLOUD_ON) {
    iotDevice = new CloudIoTCoreDevice(                                         
      IOT_PROJECT_ID.c_str(), IOT_LOCATION.c_str(), IOT_REGISTRY_ID.c_str(), IOT_DEVICE_ID.c_str(),
      IOT_PRIVATE_KEY[0].c_str());	  
    Serial.print("GCloudHandler device created: "); Serial.println(iotDevice->getDeviceId());
    netClient = new WiFiClientSecure();
    Serial.println("GCloudHandler netClient created");
    iotMqttClient = new MQTTClient(512);
    iotMqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout	 
    iotMqttClient->onMessage(__iotMessageReceived);   
#ifdef GCLOUD_USE_FREERTOS
    xTaskCreatePinnedToCore( vTaskLoop, "IOT_LOOP", 4096, (void* const) this , tskIDLE_PRIORITY, &xLoopTask, 0);
    configASSERT( xLoopTask );
#endif /*GCLOUD_USE_FREERTOS*/
  }
}

void GCloudHandler::onConnected() {
  // Set QoS to 1 (ack) for configuration messages
  iotMqttClient->subscribe(iotDevice->getConfigTopic(), 1);
  // QoS 0 (no ack) for commands
  iotMqttClient->subscribe(iotDevice->getCommandsTopic(), 0);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
#ifdef __DEBUG    
    Serial.println("Failed to obtain time");
#endif    
    return;
  }

  char timeStr[128];
  strftime(timeStr, 128, "%Y-%m-%dT%H:%M:%SZ", &timeinfo); 
	publishState("{ \"timestamp\": \"" + String(timeStr) + "\", \"connected\": true }");
}

void GCloudHandler::onCommand(String& command) {

}

void GCloudHandler::onConfigUpdate(String& config) {

}

void GCloudHandler::onMessage(String &topic, String &payload) {
  if (iotDevice->getCommandsTopic().startsWith(topic)) onCommand(payload);
  else if (iotDevice->getConfigTopic().startsWith(topic)) onConfigUpdate(payload);
#ifdef __DEBUG
  else {
    Serial.print("GCloudHandler::onMessage: ");	
    Serial.print(topic);
    Serial.print(", ");
    Serial.println(payload);
  }
#endif
}

void GCloudHandler::reconnect() {
  if (WiFi.isConnected()) {
        if (iotMqttClient != NULL && !iotMqttClient->connected()) {
#ifdef __DEBUG
            Serial.println("Attempting IOT MQTT connection...");
#endif

        if (this->useLts) {
#ifdef __DEBUG
	    Serial.println("Connect with " + String(CLOUD_IOT_CORE_MQTT_HOST_LTS) + ":" + String(CLOUD_IOT_CORE_MQTT_PORT));
#endif
	    iotMqttClient->begin(CLOUD_IOT_CORE_MQTT_HOST_LTS, CLOUD_IOT_CORE_MQTT_PORT, *netClient);
        } else {
#ifdef __DEBUG
            Serial.println("Connect with " + String(CLOUD_IOT_CORE_MQTT_HOST_LTS) + ":" + String(CLOUD_IOT_CORE_MQTT_PORT));
#endif
            iotMqttClient->begin(CLOUD_IOT_CORE_MQTT_HOST, CLOUD_IOT_CORE_MQTT_PORT, *netClient);
        }

        String jwt = getDeviceJWT();
        if (!jwt.isEmpty()) {
            iotMqttClient->connect(iotDevice->getClientId().c_str(), "unused", jwt.c_str(), false /*skip*/);
            if (iotMqttClient->lastError() != LWMQTT_SUCCESS) {
                logError();
                logReturnCode();
                // Clean up the client
                iotMqttClient->disconnect();
            } else {
                // We're now connected
#ifdef __DEBUG
                Serial.println("IOT connected");
#endif
                onConnected();
            }
        }
     }
  }
#ifdef __DEBUG
  else Serial.println("WiFi disconnected. IOT Reconnect failed.");
#endif
}

bool GCloudHandler::isConnected() {
    if (WiFi.isConnected()) {
        bool connected = false; 
        for (int i = 0; i < 10 && !connected; i++) {
            connected = connected || (iotMqttClient != NULL && iotMqttClient->connected());
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        return connected;
    }
    else return false;
}

bool GCloudHandler::publishTelemetry(String data) {  
  return CLOUD_ON ? iotMqttClient->publish(iotDevice->getEventsTopic(), data) : false;
}

bool GCloudHandler::publishTelemetry(const char* data, int length) {
  return CLOUD_ON ? iotMqttClient->publish(iotDevice->getEventsTopic().c_str(), data, length) : false;
}

bool GCloudHandler::publishTelemetry(String subtopic, String data) {
  return CLOUD_ON ? iotMqttClient->publish(iotDevice->getEventsTopic() + subtopic, data) : false;
}

bool GCloudHandler::publishTelemetry(String subtopic, const char* data, int length) {
  return CLOUD_ON ? iotMqttClient->publish(String(iotDevice->getEventsTopic() + subtopic).c_str(), data, length) : false;
}

// Helper that just sends default sensor
bool GCloudHandler::publishState(String data) {
  return CLOUD_ON ? iotMqttClient->publish(iotDevice->getStateTopic(), data) : false;
}

bool GCloudHandler::publishState(const char* data, int length) {
  return CLOUD_ON ? iotMqttClient->publish(iotDevice->getStateTopic().c_str(), data, length) : false;
}

void GCloudHandler::logError() {
  Serial.println(iotMqttClient->lastError());
  switch(iotMqttClient->lastError()) {
    case (LWMQTT_BUFFER_TOO_SHORT):
#ifdef __DEBUG
      Serial.println("LWMQTT_BUFFER_TOO_SHORT");
#endif
      break;
    case (LWMQTT_VARNUM_OVERFLOW):
#ifdef __DEBUG
      Serial.println("LWMQTT_VARNUM_OVERFLOW");
#endif
      break;
    case (LWMQTT_NETWORK_FAILED_CONNECT):
#ifdef __DEBUG
      Serial.println("LWMQTT_NETWORK_FAILED_CONNECT");
#endif
      break;
    case (LWMQTT_NETWORK_TIMEOUT):
#ifdef __DEBUG
      Serial.println("LWMQTT_NETWORK_TIMEOUT");
#endif
      break;
    case (LWMQTT_NETWORK_FAILED_READ):
#ifdef __DEBUG
      Serial.println("LWMQTT_NETWORK_FAILED_READ");
#endif
      break;
    case (LWMQTT_NETWORK_FAILED_WRITE):
#ifdef __DEBUG
      Serial.println("LWMQTT_NETWORK_FAILED_WRITE");
#endif
      break;
    case (LWMQTT_REMAINING_LENGTH_OVERFLOW):
#ifdef __DEBUG
      Serial.println("LWMQTT_REMAINING_LENGTH_OVERFLOW");
#endif
      break;
    case (LWMQTT_REMAINING_LENGTH_MISMATCH):
#ifdef __DEBUG
      Serial.println("LWMQTT_REMAINING_LENGTH_MISMATCH");
#endif
      break;
    case (LWMQTT_MISSING_OR_WRONG_PACKET):
#ifdef __DEBUG
      Serial.println("LWMQTT_MISSING_OR_WRONG_PACKET");
#endif
      break;
    case (LWMQTT_CONNECTION_DENIED):
#ifdef __DEBUG
      Serial.println("LWMQTT_CONNECTION_DENIED");
#endif
      break;
    case (LWMQTT_FAILED_SUBSCRIPTION):
#ifdef __DEBUG
      Serial.println("LWMQTT_FAILED_SUBSCRIPTION");
#endif
      break;
    case (LWMQTT_SUBACK_ARRAY_OVERFLOW):
#ifdef __DEBUG
      Serial.println("LWMQTT_SUBACK_ARRAY_OVERFLOW");
#endif
      break;
    case (LWMQTT_PONG_TIMEOUT):
#ifdef __DEBUG
      Serial.println("LWMQTT_PONG_TIMEOUT");
#endif
      break;
    default:
#ifdef __DEBUG
      Serial.println("This error code should never be reached.");
#endif
      break;
  }
}

void GCloudHandler::logReturnCode() {
  Serial.println(iotMqttClient->returnCode());
  switch(iotMqttClient->returnCode()) {
    case (LWMQTT_CONNECTION_ACCEPTED):
#ifdef __DEBUG
      Serial.println("OK");
#endif
      break;
    case (LWMQTT_UNACCEPTABLE_PROTOCOL):
#ifdef __DEBUG
      Serial.println("LWMQTT_UNACCEPTABLE_PROTOCOLL");
#endif
      break;
    case (LWMQTT_IDENTIFIER_REJECTED):
#ifdef __DEBUG
      Serial.println("LWMQTT_IDENTIFIER_REJECTED");
#endif
      break;
    case (LWMQTT_SERVER_UNAVAILABLE):
#ifdef __DEBUG
      Serial.println("LWMQTT_SERVER_UNAVAILABLE");
#endif
      break;
    case (LWMQTT_BAD_USERNAME_OR_PASSWORD):
#ifdef __DEBUG
      Serial.println("LWMQTT_BAD_USERNAME_OR_PASSWORD");
#endif
      iss = 0; // Force JWT regeneration
      break;
    case (LWMQTT_NOT_AUTHORIZED):
#ifdef __DEBUG
      Serial.println("LWMQTT_NOT_AUTHORIZED");
#endif
      iss = 0; // Force JWT regeneration
      break;
    case (LWMQTT_UNKNOWN_RETURN_CODE):
#ifdef __DEBUG
      Serial.println("LWMQTT_UNKNOWN_RETURN_CODE");
#endif
      break;
    default:
#ifdef __DEBUG
      Serial.println("This return code should never be reached.");
#endif
      break;
  }
}


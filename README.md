# Google IoT Cloud Handler

This library based on https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino experimental library
for my production devices based on Espressif32 microcontollers and tested for few month in real devices.

GCloudHandler automatically connects to Google IoT Cloud with backoff reconnection policy support. 

GCloudHandler class may run in FreeRTOS configuration if GCLOUD_USE_FREERTOS symbol defined.  

## Supported hardware targets

Currently, we support the following hardware targets:

* Espressif ESP32
* Espressif ESP8266 (not tested)

## Dependencies
Some examples use specific dependencies that need to be installed via the Arduino Library manager.

* [lwMQTT] (https://github.com/256dpi/arduino-mqtt)
* [Google Cloud IoT JWT] (https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino)

## Quickstart

First, install the library using the Arduino Library Manager.
* Open Arduino and select the **Sketch > Include Library > Library Manager**
menu item.
* In the filter box, search for "GCloudHandler".
* Install the library

Next, enable the Cloud IoT Core API by opening the [Google Cloud IoT Core console](https://console.cloud.google.com/iot/).

Next, create your device registry as described in [the Quickstart](https://cloud.google.com/iot/docs/quickstart)
or by using the [Google Cloud SDK](https://cloud.google.com/sdk).

If you're using the SDK, the following commands will setup PubSub and Cloud IoT
Core for testing on your Arduino device:

Create the PubSub topic and subscription:

    gcloud pubsub topics create atest-pub --project=YOUR_PROJECT_ID
    gcloud pubsub subscriptions create atest-sub --topic=atest-pub

Create the Cloud IoT Core registry:

    gcloud iot registries create atest-registry \
      --region=us-central1 --event-notification-config=topic=atest-pub

Generate an Eliptic Curve (EC) private / public key pair:

    openssl ecparam -genkey -name prime256v1 -noout -out ec_private.pem
    openssl ec -in ec_private.pem -pubout -out ec_public.pem

Register the device using the keys you generated:

    gcloud iot devices create atest-dev  --region=us-central1 \
        --registry=atest-registry \
        --public-key path=ec_public.pem,type=es256

At this point, your registry is created and your device has been added to the
registry so you're ready to connect it.

Select one of the available samples from the **File > Examples > GCloudHandler**
menu and replace configuration items in GCloudJandler constructor call by your values.

Find and replace the following values first:
* IOT_PROJECT_ID (get from console or `gcloud config list`)
* IOT_LOCATION (use your location. Google default is `us-central1`)
* IOT_REGISTRY_ID (created in previous steps, e.g. `atest-reg`)
* IOT_DEVICE_ID (created in previous steps, e.g. `atest-device`)

You will also need to extract your private key using the following command:

    openssl ec -in ec_private.pem -noout -text

will need to copy the output for the private key bytes into the
* IOT_PRIVATE_KEY parameter

When you run the sample, the device will connect and receive configuration
from Cloud IoT Core. When you change the configuration in the Cloud IoT Core
console, that configuration will be reflrected on the device.

Before the examples will work, you will also need to configure the root
certificate as described in the configuration headers.

After you have published telemetry data, you can read it from the PubSub topic
using the [Google Cloud SDK](https://cloud.google.com/sdk). With the SDK installed,
run the following command to create a :

```
gcloud pubsub subscriptions create <your-subscription-name> --topic=<your-iot-pubsub-topic>
```

Then read the telemetry messages:
```
gcloud pubsub subscriptions pull --limit 500 --auto-ack <your-subscription-name>
```

## Usage

Define an instance of GCloudHandler class:

GCloudHandler gCloudHandler(_IOT_PROJECT_ID, _IOT_LOCATION
	, _IOT_REGISTRY_ID, _IOT_DEVICE_ID, _IOT_PRIVATE_KEY);

Call gCloudHandler.setup() in your setup function:

void setup() {
...
	gCloudHandler.setup();
...
}

Make sure to call gCloudHandler.loop() in your loop function:

void loop() {
...
	gCloudHandler.loop();

}

Create GCloudHandler derived class and override onCommand and onConfigUpdate methods in case you need to receive Cloud commands and configuration updates.

## Notes on the certificate

The [root certificate from Google](https://pki.goog/roots.pem) is used to verify communication to
Google. Although unlikely, it's possible for the certificate to expire or rotate, requiring you to
update it.

If you're using the ESP8266 project, you need to either install the Certificate to SPIFFS
using the [SPIFFS upload utility](https://github.com/esp8266/arduino-esp8266fs-plugin) or
will need to uncomment the certificate bytes in the sample. Note that the SPIFFS utility simply 
uploads the files stored in the **data** subfolder. The sample assumes the file is called `ca.crt`:

    ├── Esp8266...
    │   ├── data
    │   │   └── ca.crt

To convert the certificate to the DER format, the following command shuold be used:

    wget pki.goog/roots.pem
    openssl x509 -outform der -in roots.pem -out ca.crt

If you're using the ESP32, you can paste the certificate bytes (don't forget the \n characters) 
into the sample. You can use any of the root certificate bytes for the certificates with Google
Trust Services (GTS) as the certificate authority (CA). This is easy to get using curl, e.g.

    curl pki.goog/roots.pem

If you're using Genuino boards like the MKR1000, you will need to add SSL certificates to your
board as [described on Hackster.io](https://www.hackster.io/arichetta/add-ssl-certificates-to-mkr1000-93c89d). 
The MQTT server address is `mqtt.googleapis.com` and the port is either `8883` for most cases or `443` in 
case your device is running in an environment where port 8883 is blocked. For long-term support,
the server is `mqtt.2030.ltsapis.goog`.

In future versions of this library, the MQTT domain and certificates will be changed for long term support (LTS) to:

* MQTT LTS Domain - mqtt.2030.ltsapis.goog
* Primary cert - [https://pki.goog/gtsltsr/gtsltsr.crt](https://pki.goog/gtsltsr/gtsltsr.crt)
* Backup cert - [https://pki.goog/gsr4/GSR4.crt](https://pki.goog/gsr4/GSR4.crt)

## For more information

* [Access Google Cloud IoT Core from Arduino](https://medium.com/@gguuss/accessing-cloud-iot-core-from-arduino-838c2138cf2b)
* [Building Google Cloud-Connected Sensors](https://medium.com/@gguuss/building-google-cloud-connected-sensors-2d46a1c58012)
* [Arduino and Google Cloud IoT](https://medium.com/@gguuss/arduino-and-google-cloud-iot-e2082e0ac000)
* [Experimenting with Robots and Cloud IoT Core](https://medium.com/@gguuss/experimenting-with-robots-and-cloud-iot-core-790ee17345ef)
* [Arduino Hexspider Revisited](https://medium.com/@gguuss/hexspider-robot-revisited-d78ff7ce9b6c)
* [TBD FAQ](https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino/issues/92)
* [As featured in Google Cloud I/O 2018](https://www.youtube.com/watch?v=7kpE44tXQak&t=1701s)


## Error codes, Debugging, and Troubleshooting

The error codes for the lwMQTT library are listed [in this header file](https://github.com/256dpi/arduino-mqtt/blob/master/src/lwmqtt/lwmqtt.h#L16-L29).
Define __DEBUG symbol if you need a debug output from the library.

If you're having trouble determining what's wrong, it may be helpful to enable more verbose debugging in Arduino by setting the debug level in the IDE under **Tools > Core Debug Level > Verbose**.

If you are using newer versions of the ESP8266 SDK, you need to set SSL support to "All SSL Cyphers" and you may need to modify the memory settings in BearSSL by modifying [Arduino/cores/esp8266/StackThunk.cpp](https://github.com/esp8266/Arduino/issues/6811).

A few things worth checking while troubleshooting:
* Is billing enabled for your project?
* Is the PubSub topic configured with your device registry valid?
* Is the [JWT valid](https://jwt.io)?
* Are the values setup in `ciotc_config.h` appearing correctly in `*_mqtt.h`?

## License

Apache 2.0; see [LICENSE](LICENSE) for details.

## Disclaimer

This project is not an official Google project. It is not supported by Google
and Google specifically disclaims all warranties as to its quality,
merchantability, or fitness for a particular purpose.


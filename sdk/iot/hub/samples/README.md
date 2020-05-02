---
page_type: sample
languages:
  - c
products:
  - azure
  - azure-iot
  - azure-iot-hub
urlFragment: iot-hub-samples
---

# Azure IoT Hub Sample

This document explains samples for the Azure SDK for Embedded C IoT Hub Client and how to use them.

## Key concepts

Key concepts are explained in detail [here][SDK_README_KEY_CONCEPTS].

## Samples for Azure IoT Hub Client APIs

This document describes how to use samples and what is done in each sample.

## Getting started

Getting started explained in detail [here][SDK_README_GETTING_STARTED].

## Samples

The following section documents various samples. All of them use the [Eclipse Paho MQTT C client][Eclipse_Paho].
For all of these samples, the following environment variables will need to be set:

- `AZ_IOT_DEVICE_ID`: Your device id.
- `AZ_IOT_HUB_HOSTNAME`: The hostname of your IoT Hub.
- `AZ_IOT_DEVICE_X509_CERT_PEM_FILE`: The full path to your device cert (in `.pem` format) concatenated
 with its private key.
- `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE`: The full path to the trusted server cert (in `.pem` format). This is usually
not needed on Mac or Linux but might be needed for Windows.

### [IoT Hub Telemetry][telemetry_sample]
Send 5 telemetry messages using the IoT Hub Client.

### [IoT Hub Twin][twin_sample]
Use twin features such as updating reported properties, receiving the twin document, and receiving desired properties using the IoT Hub Client.

### [IoT Hub Methods][methods_sample]
Invoke methods from the cloud. The sample supports a method named "ping"
which if successful will return back to you a json payload of the following:

```json
{"response": "pong"}
```

On failure, a status of `404` will be returned with an empty JSON payload.

### [IoT Hub C2D][c2d_sample]
Receive and view incoming C2D messages using the IoT Hub Client.

## Troubleshooting

When interacting with the Azure IoT Hub using this C client library, errors are documented within the [MQTT State Machine][error_codes] requests.

## Next steps

Start using the IoT Hub Client in your solutions. More SDK details can be found in [SDK README][IOT_CLIENT_README].

### Additional Documentation

For extensive documentation on Azure IoT Hub, see the [API reference documentation][iot_hub_mqtt].

<!-- LINKS -->
[IOT_CLIENT_README]: ../../README.md
[SDK_README_GETTING_STARTED]: ../../README.md#getting-started
[SDK_README_KEY_CONCEPTS]: ../../README.md#key-concepts
[c2d_sample]: src/iot_hub_c2d_sample.c
[methods_sample]: src/iot_hub_methods_sample.c
[telemetry_sample]: src/iot_hub_telemetry_sample.c
[twin_sample]: src/iot_hub_twin_sample.c
[iot_hub_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: ../../doc/mqtt_state_machine.md#IoT-Service-Errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

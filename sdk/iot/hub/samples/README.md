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

This document explains samples for the Azure Embedded C SDK IoT Hub Client and how to use them.

## Key concepts

Key concepts are explained in detail [here][SDK_README_KEY_CONCEPTS].

## Samples for Azure IoT Hub Client APIs

This document describes how to use samples and what is done in each sample.

## Getting started

Getting started explained in detail [here][SDK_README_GETTING_STARTED].

## Examples

The following section documents various examples. All of them use the [Eclipse Paho MQTT C client][Eclipse_Paho].

### [IoT Hub C2D][c2d_sample]
Receive and view incoming C2D messages using the IoT Hub client.

### [IoT Hub Methods][methods_sample]
Invoke methods from the cloud. The sample supports a method named "double"
which will return back to you the value sent as the payload. The payload must be of the form:
```json
{
  "value": 10
}
```
where 10 can be substituted for any number between `UINT64_T_MIN / 2` and `UINT64_T_MAX / 2`.
### [IoT Hub Telemetry][telemetry_sample]
Send 5 telemetry messages using the IoT Hub client.
### [IoT Hub Twin][twin_sample]
Use twin features such as updating reported properties, receiving the twin document, and receiving desired properties using the IoT Hub client.

## Troubleshooting

When interacting with the Azure IoT Hub using this C client library, errors are documented within the [MQTT State Machine][error_codes] requests.

## Next steps

Start using the IoT Hub Client in your solutions. Our SDK details can be found at [SDK README][IOT_CLIENT_README].

### Additional Documentation

For extensive documentation on Azure IoT Hub, see the [API reference documentation][iot_hub_mqtt].

## Contributing

This project welcomes contributions and suggestions. Find [more contributing][SDK_README_CONTRIBUTING] details here.

<!-- LINKS -->
[IOT_CLIENT_README]: ../../README.md
[SDK_README_CONTRIBUTING]:../../README.md#contributing
[SDK_README_GETTING_STARTED]: ../../README.md#getting-started
[SDK_README_KEY_CONCEPTS]: ../../README.md#key-concepts
[c2d_sample]: c2d/src/iot_hub_c2d_example.c
[methods_sample]: methods/src/iot_methods_example.c
[telemetry_sample]: telemetry/src/iot_hub_telemetry_example.c
[twin_sample]: twin/src/iot_hub_twin_example.c
[iot_hub_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: ../../doc/mqtt_state_machine.md#IoT-Service-Errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

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

1. [IoT Hub C2D][paho_c2d_sample]: Receive and view incoming C2D messages using the IoT Hub client.
2. [IoT Hub Telemetry][paho_telemetry_sample]: Send 5 telemetry messages using the IoT Hub client.
3. [IoT Hub Twin][paho_twin_sample]: Use twin features such as updating reported properties, 
    receiving the twin document, and receiving desired properties using the IoT Hub client.

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
[paho_c2d_sample]: c2d/src/paho_iot_hub_c2d_example.c
[paho_telemetry_sample]: telemetry/src/paho_iot_hub_telemetry_example.c
[paho_twin_sample]: twin/src/paho_iot_hub_twin_example.c
[iot_hub_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: ../../doc/mqtt_state_machine.md#IoT-Service-Errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

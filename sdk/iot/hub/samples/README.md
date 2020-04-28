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

   Following section document various examples.

1. [IoT Hub using Paho][samples_paho]: Register a device using the IoT Hub client and the [Eclipse Paho MQTT C client][Eclipse_Paho].

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
[samples_paho]: src/paho_iot_hub_example.c
[iot_hub_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: ../../doc/mqtt_state_machine.md#IoT-Service-Errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

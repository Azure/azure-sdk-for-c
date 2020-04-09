---
page_type: sample
languages:
  - c
products:
  - azure
  - azure-iot
  - azure-iot-provisioning
urlFragment: iot-provisioning-samples
---

# Azure IoT Device Provisioning Service Sample

This document explains samples for the Azure Embedded C SDK IoT Provisioning Client and how to use them.

## Key concepts

Key concepts are explained in detail [here][SDK_README_KEY_CONCEPTS].

## Samples for Azure IoT Provisioning Client APIs

This document describes how to use samples and what is done in each sample.

## Getting started

Getting started explained in detail [here][SDK_README_GETTING_STARTED].

## Examples

   Following section document various examples.

1. [IoT Provisioning using Paho][samples_paho]: Register a device using the IoT Provisioning client and the [Eclipse Paho MQTT C client][Eclipse_Paho].

## Troubleshooting

When interacting with the Azure Device Provisioning Service using this C client library, errors are documented within the [MQTT State Machine][error_codes] requests.

## Next steps

Start using the IoT Provisioning Client in your solutions. Our SDK details can be found at [SDK README][IOT_CLIENT_README].

### Additional Documentation

For extensive documentation on Azure Device Provisioning Service, see the [API reference documentation][iot_provisioning_mqtt].

## Contributing

This project welcomes contributions and suggestions. Find [more contributing][SDK_README_CONTRIBUTING] details here.

<!-- LINKS -->
[IOT_CLIENT_README]: ../../README.md
[SDK_README_CONTRIBUTING]:../../README.md#contributing
[SDK_README_GETTING_STARTED]: ../../README.md#getting-started
[SDK_README_KEY_CONCEPTS]: ../../README.md#key-concepts
[samples_paho]: src/paho_iot_provisioning_example.c
[iot_provisioning_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: ../../doc/mqtt_state_machine.md#IoT-Service-Errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

![Impressions](https://azure-sdk-impressions.azurewebsites.net/api/impressions/azure-sdk-for-c%2Fsdk%2Fiot%2Fprovisioning%2Fsamples%2FREADME.png)

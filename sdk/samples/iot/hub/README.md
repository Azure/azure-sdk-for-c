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

# Azure IoT Hub Samples

This document explains samples for the Azure SDK for Embedded C IoT Hub Client and how to use them. 

For step-by-step guides starting from scratch, you may refer to these documents:
  - Linux: [How to setup and run Azure SDK for Embedded C IoT Hub Samples on Linux](./linux/how_to_iot_hub_samples_linux.md)
  - Windows: [How to setup and run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows](./windows/how_to_iot_hub_samples_windows.md).
  - ESP8266: [How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Esp8266 NodeMCU](./aziot_esp8266/how_to_esp8266_nodemcu.md)

  **Note**: While Windows and Linux devices are not likely to be considered as constrained ones, these samples were created to make it simpler to test the Azure SDK for Embedded C libraries, even without a real device. 


## Key Concepts

Key concepts are explained in detail [here][SDK_README_KEY_CONCEPTS].

## Prerequisites

- To generate the device certificate, the provided script uses [OpenSSL 1.1.1 LTS](https://www.openssl.org/source/). Please
install the OpenSSL command line utility prior to using the script.
  - Note: for Linux based systems, manual installation from source can be risky ([details here](https://github.com/openssl/openssl/issues/11227#issuecomment-616445289)). We recommend installing for Linux via apt:  
  ```bash
  sudo apt-get install libssl-dev
  ```
- To use the samples, we use [Eclipse Paho MQTT C client][Eclipse_Paho]. You can use the directions
[here][VCPKG_DIRECTIONS] to set up VCPKG to download and manage linking the dependency.

## Getting Started

These samples use X509 authentication to connect to Azure IoT Hub. To easily run these samples, we have provided
a script to generate a self-signed device certificate used for device authentication. You can run it using the following
steps.

1. Run the script using the following form:
    ```bash
    ./generate_certificate.sh
    ```
1. Take note of the certificate fingerprint printed at the end of the output. (It is also placed in a file
named `fingerprint.txt` for your convenience).
1. Create a device in IoT Hub with `X.509 Self-Signed` authentication.
1. Paste in the certificate fingerprint printed previously.

After this is generated, make sure to set `AZ_IOT_DEVICE_X509_CERT_PEM_FILE` as an environment variable. It should be the
full path to the generated `device_cert_store.pem` file.

For more details on X509 authentication, refer to [this online documentation](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-x509ca-overview#how-to-register-the-x509-ca-certificate-to-iot-hub)

***NOTE: THESE ARE TO BE USED FOR SAMPLE USE ONLY. DO NOT USE THEM IN PRODUCTION***

## Samples

For all of these samples, the following environment variables will need to be set.

**NOTE**: These need to be set before running the cmake commands.

- `VCPKG_DEFAULT_TRIPLET`: The triplet created using the instructions [here][VCPKG_DIRECTIONS].
- `VCPKG_ROOT`: The full path to the VCPKG directory used to generate the triplet.
- `AZ_IOT_DEVICE_ID`: Your device id.
- `AZ_IOT_HUB_HOSTNAME`: The hostname of your IoT Hub.
- `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE`: The full path to the trusted server cert (in `.pem` format). This is usually
not needed on Mac or Linux but might be needed for Windows.

For samples using certificate authentication, the following environment variable needs to be set.

- `AZ_IOT_DEVICE_X509_CERT_PEM_FILE`: The full path to your device cert (in `.pem` format) concatenated
 with its private key.

For samples using SAS Key authentication, the following environment variables need to be set.

- `AZ_IOT_HUB_DEVICE_SAS_KEY`: The SAS key for your device (called "Primary Key" on the Azure Portal for your device).
- `AZ_IOT_HUB_DEVICE_SAS_KEY_DURATION`: Expiration (in hours) of the SAS key. If not set, this value will default to **2 hours.**

Once these are set, you MUST compile the SDK with the `TRANSPORT_PAHO` option turned on. For example, on the command
line, it might look like the following:
```bash
cmake -DTRANSPORT_PAHO=ON ..
```

After the SDK is built, you are free to run any of the following samples.

### [IoT Hub Telemetry (SAS Key)][telemetry_sample_sas]
Send 5 telemetry messages using the IoT Hub Client with SAS key authentication.

### [IoT Hub Telemetry (Certificates)][telemetry_sample_cert]
Send 5 telemetry messages using the IoT Hub Client with certificate authentication.

### [IoT Hub Twin (Certificates)][twin_sample]
Use device twin features such as receiving the twin document, updating reported properties, and sending desired properties using the Azure IoT Hub Client.
This sample uses a property named `device_count`, which records the number of times the device sends a reported property message to the service.

* To initiate a GET response request from the device, you will use the command `g`. 

* To initiate a reported property message from the device, you will use the command `r`.

* To send a device twin desired property message from the service to the device, open the device twin document in your Azure IoT Hub.  Add the property `device_count` along with a corresponding value to the `desired` section of the JSON.  
```json
{
  "properties": {
    "desired": {
      "device_count": 42,
    }
  }
}
```
* Select Save to send the message. The device will store the value locally and report the updated property to the service.

### [IoT Hub Methods (Certificates)][methods_sample]
Invoke methods from the cloud. The sample supports a method named "ping"
which if successful will return back to you a json payload of the following:

```json
{"response": "pong"}
```

On failure, a status of `404` will be returned with an empty JSON payload.

### [IoT Hub C2D (Certificates)][c2d_sample]
Receive and view incoming C2D messages using the IoT Hub Client.

### [IoT Hub PnP (Certificates)][pnp_sample]
Connect a PnP enabled device with the Digital Twin Model ID (DTMI) detailed [here](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json).
In short, the capabilities are listed here:
- **Methods**: Invoke a method called `getMaxMinReport` with JSON payload value `"since"` with an [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) value for start time for the report. The method sends a response containing the following JSON payload:  
```json
{
  "maxTemp": 20,
  "minTemp": 20,
  "avgTemp": 20,
  "startTime": "<ISO8601 time>",
  "endTime": "<ISO8601 time>"
}
```
with correct values substituted for each field.
- **Telemetry**: Device sends a JSON message with the field name `temperature` and the `double` value of the temperature.
- **Twin**: Desired property with the field name `targetTemperature` and the `double` value for the desired temperature. Reported property with the field name `maxTempSinceLastReboot` and the `double` value for the highest temperature.Note that part of the PnP spec is a response to a desired property update from the service. The device will send back a reported property with a similarly named property and a set of "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an optional `ad` for an ack description.

## Troubleshooting

When interacting with the Azure IoT Hub using this C client library, errors are documented within the [MQTT State Machine][error_codes] requests.

## Next Steps

Start using the IoT Hub Client in your solutions. More SDK details can be found in [SDK README][IOT_CLIENT_README].

### Additional Documentation

For extensive documentation on Azure IoT Hub, see the [API reference documentation][iot_hub_mqtt].

<!-- LINKS -->
[IOT_CLIENT_README]: https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients
[SDK_README_GETTING_STARTED]: https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#getting-started
[SDK_README_KEY_CONCEPTS]: https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients
[VCPKG_DIRECTIONS]:https://github.com/Azure/azure-sdk-for-c#development-environment
[c2d_sample]: src/paho_iot_hub_c2d_example.c
[methods_sample]: src/paho_iot_hub_methods_example.c
[telemetry_sample_sas]: src/paho_iot_hub_sas_telemetry_example.c
[telemetry_sample_cert]: src/paho_iot_hub_telemetry_example.c
[twin_sample]: src/paho_iot_hub_twin_example.c
[pnp_sample]: src/paho_iot_hub_pnp_example.c
[iot_hub_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: ../../../../sdk/docs/iot/mqtt_state_machine.md#iot-service-errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

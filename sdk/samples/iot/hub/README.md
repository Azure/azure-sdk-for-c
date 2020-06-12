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
- If you are having trouble with any of the prerequisites, please see our `how_to_iot_hub_samples_<platform>.md` documents [here](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/) for more help.

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
Use twin features such as receiving the twin document, updating reported properties, and receiving desired properties using the IoT Hub Client.

```shell
/azure-sdk-for-c/cmake/sdk/iot/hub/samples$ ./paho_iot_hub_twin_example 
AZ_IOT_DEVICE_X509_CERT_PEM_FILE = /mnt/c/Repos/azure-sdk-for-c/sdk/iot/hub/samples/src/device_cert_store.pem
AZ_IOT_DEVICE_X509_TRUST_PEM_FILE =
AZ_IOT_DEVICE_ID = testdevice-x509
AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
Posting connect semaphore for client TDS2 rc 0
Subscribed to topics.

Waiting for activity:
Press 'g' for device to request twin document from service.
Press 'r' for device to send device_count reported property to service. device_count will then locally increment.
[Press 'q' to quit]

g
Device requesting twin document from service.
Received a message from service.
Topic: $iothub/twin/res/200/?$rid=get_twin
Topic is a twin message.
A twin GET response was received.
Payload:
{"desired":{"$version":1},"reported":{"$version":1}}
Response status was 200.

r
Device sending device_count reported property to service.
Payload: {"device_count":0}
Received a message from service.
Topic: $iothub/twin/res/204/?$rid=reported_prop&$version=2
Topic is a twin message.
A twin reported properties service response was received.
No Payload upon success.
Response status was 204.

g
Device requesting twin document from service.
Received a message from service.
Topic: $iothub/twin/res/200/?$rid=get_twin
Topic is a twin message.
A twin GET response was received.
Payload:
{"desired":{"$version":1},"reported":{"device_count":0,"$version":2}}
Response status was 200.
```

To send a desired property to the device, open the device twin document in your IoT Hub and add `device_count` to the `desired` section of the JSON.

```json
"properties": {
    "desired": {
      "device_count": 42,
```

Select Save to send the message. The device will store value locally and report the property to the service.

```shell
Received a message from service.
Topic: $iothub/twin/PATCH/properties/desired/?$version=2
Topic is a twin message.
A twin desired properties message was received.
Payload:
{"device_count":42,"$version":2}
Response status was 200.
Updating device_count reported property to service.
Payload: {"device_count":42}

Received a message from service.
Topic: $iothub/twin/res/204/?$rid=reported_prop&$version=3
Topic is a twin message.
A twin reported properties service response was received.
No Payload upon success.
Response status was 204.

g
Device requesting twin document from service.
Received a message from service.
Topic: $iothub/twin/res/200/?$rid=get_twin
Topic is a twin message.
A twin GET response was received.
Payload:
{"desired":{"device_count":42,"$version":2},"reported":{"device_count":42,"$version":3}}
Response status was 200.
```



### [IoT Hub Methods (Certificates)][methods_sample]
Invoke methods from the cloud. The sample supports a method named "ping"
which if successful will return back to you a json payload of the following:

```json
{"response": "pong"}
```

On failure, a status of `404` will be returned with an empty JSON payload.

### [IoT Hub C2D (Certificates)][c2d_sample]
Receive and view incoming C2D messages using the IoT Hub Client.

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
[iot_hub_mqtt]: https://docs.microsoft.com/en-us/azure/iot-dps/iot-dps-mqtt-support
[error_codes]: https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#iot-service-errors
[Eclipse_Paho]: https://www.eclipse.org/paho/clients/c/

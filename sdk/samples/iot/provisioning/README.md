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

## Introduction
This document explains samples for the Azure Embedded C SDK IoT Provisioning Client and how to use them.

Samples are designed to be simple so that the user can understand the function calls necessary to connect with the Device Provisioning Service. These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result,  **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries, which these samples do not include. 

## Key Concepts

Further background and key concepts are explained in detail in the [Azure IoT Client README](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients).

## Prerequisites

* Have an [Azure acount](https://azure.microsoft.com/en-us/) created.
* Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
* Have an [Azure IoT Hub Device Provisioning Service (DPS)](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision) created.
* Have OpenSSL installed:

  For Linux based systems, we recommend:
  ```bash
  sudo apt-get install libssl-dev
  ```
  For non-Linux based systems, download the [OpenSSL 1.1.1 LTS](https://www.openssl.org/source/) command line utiliy.
* Have [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/clients/c/) installed.  After installation, use these directions [here](https://github.com/Azure/azure-sdk-for-c#development-environment) to both download VCPKG and use it to link the Paho MQTT dependency.

## Running the Sample

### Paho IoT Provisioning (Certificates)
This [sample]() uses x509 authentication to connect to Azure IoT Hub Device Provisioning Service (DPS).  To easily run this sample, we have provided a [script](src/generate_certificate.sh) to generate a self-signed device certification used for device authentication. This script is intended **for sample use only** and not to be used in production code.

1. Enter the directory `/azure-sdk-for-c/sdk/samples/iot/provisioning/src/` and run the script using the following form:

    ```bash
    ./generate_certificate.sh
    ```
2. Set the environment variable `AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH` noted at the bottom of the output.

3. In your Azure DPS, add a new certificate and upload the recently generated `device_ec_cert.pem` file to it.

4. Also add a new individual device enrollment using the same `device_ec_cert.pem` file.  See [here](https://docs.microsoft.com/en-us/azure/iot-dps/quick-create-simulated-device-x509#create-a-device-enrollment-entry-in-the-portal) for further directions.  After creation, the Registration ID of your device should appear as `paho-sample-device1` in the Individual Enrollments tab.

5. Set the remaining enviroment variables:
  * `VCPKG_DEFAULT_TRIPLET` and `VCPKG_ROOT`: These should already be set per these [directions](https://github.com/Azure/azure-sdk-for-c#development-environment).
  * `AZ_IOT_DEVICE_X509_CERT_PEM_FILE`: This should already be set per directions above.
  * `AZ_IOT_ID_SCOPE`: Copy this value from the Overview tab in your Azure DPS.
  * `AZ_IOT_REGISTRATION_ID`: This should be `paho-sample-device1`.

6. Compile the code:
  * Enter the directory `/azure-sdk-for-c/cmake`.  If it does not exit, please create it.
  * Build the directory structure:

    ```bash
    cmake -DTRANSPORT_PAHO=ON ..
    ```
  * Build the sample:

    Linux:
    ```bash
    make
    ```
    Windows:
    ```cmd
    cmake --build .
    ```
7. From within the cmake directory, run the sample:

    ```
    ./sdk/samples/iot/provisioning/paho_iot_provisioning_example
    ```


## Troubleshooting

The error policy in using the Embedded C SDK client library is documented [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#error-policy).

## Next Steps and Additional Documentation

Start using the IoT Provisioning Client in your solutions! 

* A general overivew of the Embedded C SDK and additional background on runing samples can be found in the [Azure SDK for Embedded C README](https://github.com/Azure/azure-sdk-for-c#azure-sdk-for-embedded-c).
* More SDK details can be found in the [Azure IoT Client README](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients). 
* The [Azure IoT Client MQTT State Machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md) provides a high-level architecture and API information. 
* For extensive documentation on the Azure IoT Hub Device Provisioning Service, see the [Microsoft reference documentation](https://docs.microsoft.com/en-us/azure/iot-dps/).


## Contributing

This project welcomes contributions and suggestions. Find more contributing details [here](https://github.com/Azure/azure-sdk-for-c/tree/master#contributing).

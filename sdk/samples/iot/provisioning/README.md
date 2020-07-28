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

This document explains samples for the Azure Embedded C SDK IoT Device Provisioning Client and how to use them.

Samples are designed to highlight the function calls required to connect with the Device Provisioning Service (DPS). These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production code on Windows or macOS**.

The samples' instructions include specifics for both Windows and Linux based systems. For Windows, the command line examples are based on the Command Prompt and not PowerShell. The Linux examples are tailored to Debian/Ubuntu environments. Samples are also designed to work on macOS systems, but the instructions do not yet include specific command line examples for this environment.

## Key Concepts

Further background on the Azure IoT Client library and key concepts are explained in detail in the [Azure IoT Client README](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients).

## Prerequisites

* Have an [Azure account](https://azure.microsoft.com/en-us/) created.
* Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
* Have an [Azure IoT Hub Device Provisioning Service (DPS)](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision) created.
* Have [git](https://git-scm.com/download) installed.
* Have [OpenSSL](https://www.openssl.org/source/) installed:
  * For Linux based systems, we recommend:

    ```bash
    sudo apt-get install libssl-dev
    ```

  * For non-Linux based systems, download the [OpenSSL 1.1.1 LTS](https://www.openssl.org/source/openssl-1.1.1g.tar.gz) command line utility and follow the downloaded INSTALL document.
* Have the following build setup:
  * For Linux based systems, have make installed:

    ```bash
    sudo apt-get install build-essential
    ```

  * For Windows systems, have [Microsoft Visual Studio](https://visualstudio.microsoft.com/downloads/) installed.
  * For all systems, have the latest version of [CMake](https://cmake.org/download) installed.
* Have Microsoft [VCPKG](https://github.com/microsoft/vcpkg) package manager and [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. Use the directions [here](https://github.com/Azure/azure-sdk-for-c#development-environment) to download VCPKG and install Paho MQTT.

## Getting Started

### Environment Variables

Samples use environment variables for a variety of purposes, including filepaths and connection parameters. Please keep in mind, every time a new terminal is opened, the environment variables will have to be reset. Setting a variable will take the following form:

Linux:

```bash
export ENV_VARIABLE_NAME=VALUE
```

Windows:

```cmd
set ENV_VARIABLE_NAME=VALUE
```

Set the following environment variables:

* `VCPKG_DEFAULT_TRIPLET` and `VCPKG_ROOT`: Refer to these [directions](https://github.com/Azure/azure-sdk-for-c#development-environment).
* `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE`: **Only for Windows or if required by OS.** Download [BaltimoreCyberTrustRoot.crt.pem](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem) to `/azure-sdk-for-c/sdk/samples/iot/`. Copy the full filepath to this downloaded .pem file, e.g. `C:/azure-sdk-for-c/sdk/samples/iot/BaltimoreCyberTrustRoot.crt.pem`.

### Paho IoT Provisioning (Certificates)

**Executable:** paho_iot_provisioning_example

This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/provisioning/src/paho_iot_provisioning_example.c) uses x509 authentication to connect to Azure IoT Hub Device Provisioning Service (DPS). To easily run this sample, we have provided a script to generate a self-signed device certification used for device authentication. **This script is intended for sample use only and not to be used in production code**.

1. Enter the directory `/azure-sdk-for-c/sdk/samples/iot/provisioning/src/` and run the script using the following form:

    Linux:

    ```bash
    ./generate_certificate.sh
    ```

    Windows:

    ```cmd
    generate_certificate.cmd
    ```

2. In your Azure DPS, add a new individual device enrollment using the recently generated `device_ec_cert.pem` file. See [here](https://docs.microsoft.com/en-us/azure/iot-dps/quick-create-simulated-device-x509#create-a-device-enrollment-entry-in-the-portal) for further instruction. After creation, the Registration ID of your device should appear as `paho-sample-device1` in the Individual Enrollments tab.
3. Set the following environment variables:

* `AZ_IOT_DEVICE_X509_CERT_PEM_FILE`: Copy the path of the generated .pem file noted at the bottom of the generate_certificate output.
* `AZ_IOT_REGISTRATION_ID`: This should be `paho-sample-device1`.
* `AZ_IOT_ID_SCOPE`: Copy this value from the Overview tab in your Azure DPS.

### Paho IoT Provisioning (SAS)

**Executable:** paho_iot_provisioning_sas_example

This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/provisioning/src/paho_iot_provisioning_sas_example.c) uses SAS symmetric key authentication to connect to Azure IoT Hub Device Provisioning Service (DPS).

1. In your Azure DPS, add a new individual device enrollment using SAS. See [here](https://docs.microsoft.com/en-us/azure/iot-dps/quick-create-simulated-device-symm-key#create-a-device-enrollment-entry-in-the-portal) for further instruction. After creation, the Registration ID of your device will appear in the Individual Enrollments tab.

2. Set the following environment variables:

* `AZ_IOT_PROVISIONING_SAS_KEY`: Select your enrolled device from the Individual Enrollments tab and copy its Primary Key.
* `AZ_IOT_REGISTRATION_ID_SAS`: Copy the Registration Id of your SAS device from the Individual Enrollments tab.
* `AZ_IOT_ID_SCOPE`: Copy this value from the Overview tab in your Azure DPS.

## Build and Run the Sample

1. Compile the code:

* Enter the directory `/azure-sdk-for-c/cmake`. If it does not exist, please create it.
* Build the directory structure and the samples:

    ```bash
    cmake -DTRANSPORT_PAHO=ON ..
    cmake --build .
    ```

2. From within the cmake directory, run the sample:

    Linux:

    ```bash
    ./sdk/samples/iot/provisioning/<sample executable here>
    ```

    Windows:

    ```cmd
    az.sln
    ```

   Once the Windows solution opens in Visual Studio:
    * Navigate on the Solution Explorer panel to the sample project you would like to run.
    * Make it the default startup project (right-click on the sample project, then click on Set as Startup Project).
    * Build and run the project (F5 on most installations).

## Troubleshooting

The error policy in using the Embedded C SDK client library is documented [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#error-policy).

## Next Steps and Additional Documentation

Start using the IoT Provisioning Client in your solutions!

* A general overview of the Embedded C SDK and additional background on running samples can be found in the [Azure SDK for Embedded C README](https://github.com/Azure/azure-sdk-for-c#azure-sdk-for-embedded-c).
* More SDK details pertaining to IoT Client can be found in the [Azure IoT Client README](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients).
* The [Azure IoT Client MQTT State Machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md) provides a high-level architecture and API information.
* For extensive documentation on the Azure IoT Hub Device Provisioning Service, see the [Microsoft reference documentation](https://docs.microsoft.com/en-us/azure/iot-dps/).

## Contributing

This project welcomes contributions and suggestions. Find more contributing details [here](https://github.com/Azure/azure-sdk-for-c/tree/master#contributing).

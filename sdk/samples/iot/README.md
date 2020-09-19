# Azure Embedded C SDK IoT Samples

- [Azure Embedded C SDK IoT Samples](#azure-embedded-c-sdk-iot-samples)
  - [Introduction](#introduction)
  - [Prerequisites](#prerequisites)
  - [Sample Descriptions](#sample-descriptions)
    - [IoT Hub C2D Sample](#iot-hub-c2d-sample)
    - [IoT Hub Methods Sample](#iot-hub-methods-sample)
    - [IoT Hub Telemetry Sample](#iot-hub-telemetry-sample)
    - [IoT Hub SAS Telemetry Sample](#iot-hub-sas-telemetry-sample)
    - [IoT Hub Twin Sample](#iot-hub-twin-sample)
    - [IoT Hub Plug and Play Sample](#iot-hub-plug-and-play-sample)
    - [IoT Hub Plug and Play Multiple Component Sample](#iot-hub-plug-and-play-multiple-component-sample)
    - [IoT Provisioning Certificate Sample](#iot-provisioning-certificate-sample)
    - [IoT Provisioning SAS Sample](#iot-provisioning-sas-sample)
  - [Getting Started](#getting-started)
    - [Set Environment Variables](#set-environment-variables)
    - [Generate Device Certificate](#generate-device-certificate)
  - [Sample Instructions](#sample-instructions)
    - [IoT Hub Certificate Samples](#iot-hub-certificate-samples)
    - [IoT Hub SAS Sample](#iot-hub-sas-sample)
    - [IoT Provisioning Certificate Sample](#iot-provisioning-certificate-sample-1)
    - [IoT Provisioning SAS Sample](#iot-provisioning-sas-sample-1)
  - [Build and Run the Sample](#build-and-run-the-sample)
  - [Next Steps and Additional Documentation](#next-steps-and-additional-documentation)
  - [Troubleshooting](#troubleshooting)
  - [Contributing](#contributing)
    - [License](#license)

## Introduction

This document explains samples for the Azure Embedded C SDK IoT Hub Client and Device Provisioning Client.

Samples are designed to highlight the function calls required to connect with the Azure IoT Hub or the Azure IoT Hub Device Provisioning Service (DPS). These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production-level code on Windows or macOS**.

The samples' instructions include specifics for both Windows and Linux based systems. For Windows, the command line examples are based on PowerShell. The Linux examples are tailored to Debian/Ubuntu environments. Samples are also designed to work on macOS systems, but the instructions do not yet include specific command line examples for this environment. While Windows and Linux devices are not likely to be considered constrained, these samples enable one to test the Azure SDK for Embedded C libraries, debug, and step through the code, even without a real device. We understand not everyone will have a real device to test and that sometimes these devices won't have debugging capabilities.

**WARNING: Samples are generic and should not be used in any production-level code.**

More detailed step-by-step guides on how to run an IoT Hub Client sample from scratch can be found below:

- Linux: [How to Setup and Run Azure SDK for Embedded C IoT Hub Certificate Samples on Linux](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_samples_linux.md)
- Windows: [How to Setup and Run Azure SDK for Embedded C IoT Hub Certificate Samples on Microsoft Windows](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_samples_windows.md)
- ESP8266: [How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Esp8266 NodeMCU](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_esp8266_nodemcu.md)

## Prerequisites

To run the samples, ensure you have the following programs and tools installed on your system:

- Have an [Azure account](https://azure.microsoft.com/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created.
- Have an [Azure IoT Hub Device Provisioning Service (DPS)](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision) created if running a DPS sample:

  *Executables:* `paho_iot_provisioning_sample`, `paho_iot_provisioning_sas_sample`

- Have the most recent version of [Azure IoT Explorer](https://github.com/Azure/azure-iot-explorer/releases) installed (more instructions can be found [here](https://docs.microsoft.com/azure/iot-pnp/howto-use-iot-explorer)) and connected to your Azure IoT Hub if running a Plug and Play sample:

  *Executables:* `paho_iot_hub_pnp_sample`, `paho_iot_hub_pnp_component_sample`

- Have the following build environment setup:

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    ```bash
    sudo apt-get update
    sudo apt-get install build-essential # make and gcc
    sudo apt-get install curl unzip tar pkg-config
    ```

    Windows (PowerShell):

    - Have [Microsoft Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) installed with [C and C++ support](https://docs.microsoft.com/cpp/build/vscpp-step-0-installation?view=vs-2019).

    </p>
    </details>

- Have [Git](https://git-scm.com/download) installed.
- Have Microsoft [vcpkg](https://github.com/microsoft/vcpkg) package manager and [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. On Linux, this installation may take an extended amount of time (~20-30 minutes).

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    ```bash
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg install --triplet x64-linux curl cmocka paho-mqtt
    ```

    Windows (PowerShell):

    ```powershell
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    .\bootstrap-vcpkg.bat
    .\vcpkg.exe install --triplet x64-windows-static curl[winssl] cmocka paho-mqtt # Update triplet per your system.
    ```

    </p>
    </details>

- Have OpenSSL installed.

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    ```bash
    sudo apt-get install openssl libssl-dev
    ```

    Windows (PowerShell):

    - OpenSSL will be installed by vcpkg as a dependency for Eclipse Paho.

      **WARNING: It is NOT recommended to use OpenSSL in production-level code on Windows or macOS.**

      ```powershell
      # NOT RECOMMENDED to use for production-level code.
      $env:PATH=$env:PATH + ';<FULL PATH to vcpkg>\installed\x64-windows-static\tools\openssl' # Update complete path as needed.
      ```

    </p>
    </details>

- Have the latest version of [CMake](https://cmake.org/download) installed. On Linux, this installation may also take an extended amount of time (~20-30 minutes).

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    ```bash
    sudo apt-get purge cmake
    sudo tar -xvzf cmake-<latest-version>.tar.gz
    cd cmake-<latest-version>
    ./bootstrap && make && sudo make install
    ```

    Windows (PowerShell):

    - Use the Windows installer.

    </p>
    </details>

- Have the Azure SDK for Embedded C IoT repository cloned.

    NOTE: On Windows, due to the length of the repository filepaths, clone near the `C:\` root.

    ```bash
    git clone https://github.com/Azure/azure-sdk-for-c.git
    ```

## Sample Descriptions

This section provides an overview of the different samples available to run and what to expect from each.

### IoT Hub C2D Sample

- *Executable:* `paho_iot_hub_c2d_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_c2d_sample.c) receives incoming cloud-to-device (C2D) messages sent from the Azure IoT Hub to the device. It will successfully receive up to 5 messages sent from the service. If a timeout occurs while waiting for a message, the sample will exit. X509 authentication is used.

  <details><summary><i>How to interact with the C2D sample:</i></summary>
  <p>

  <b>To send a C2D message:</b> Select your device's "Message to Device" tab in the Azure Portal for your IoT Hub. Enter a message in the "Message Body" and select "Send Message".

  </p>
  </details>

### IoT Hub Methods Sample

- *Executable:* `paho_iot_hub_methods_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_methods_sample.c) receives incoming method commands invoked from the the Azure IoT Hub to the device. It will successfully receive up to 5 method commands sent from the service. If a timeout occurs while waiting for a message, the sample will exit. X509 authentication is used.

  <details><summary><i>How to interact with the Methods sample:</i></summary>
  <p>

  A method named `ping` is supported for this sample.

  <b>To invoke a method:</b> Select your device's "Direct Method" tab in the Azure Portal for your IoT Hub. Enter a method name and select "Invoke Method". If successful, the sample will return a JSON payload of the following:

  ```json
  {"response": "pong"}
  ```

  No other method commands are supported. If any other methods are attempted to be invoked, the log will report the method is not found.

  </p>
  </details>

### IoT Hub Telemetry Sample

- *Executable:* `paho_iot_hub_telemetry_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_telemetry_sample.c) sends five telemetry messages to the Azure IoT Hub. X509 authentication is used.

### IoT Hub SAS Telemetry Sample

- *Executable:* `paho_iot_hub_sas_telemetry_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_sas_telemetry_sample.c) sends five telemetry messages to the Azure IoT Hub. SAS authentication is used.

### IoT Hub Twin Sample

- *Executable:* `paho_iot_hub_twin_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_twin_sample.c) utilizes the Azure IoT Hub to get the device twin document, send a reported property message, and receive up to 5 desired property messages. If a timeout occurs while waiting for a message from the Azure IoT Hub, the sample will exit. Upon receiving a desired property message, the sample will update the twin property locally and send a reported property message back to the service. X509 authentication is used.

  <details><summary><i>How to interact with the Twin sample:</i></summary>
  <p>

  A desired property named `device_count` is supported for this sample.

  <b>To send a device twin desired property message:</b> Select your device's "Device Twin" tab in the Azure Portal of your IoT Hub. Add the property `device_count` along with a corresponding value to the `desired` section of the JSON. Select "Save" to update the twin document and send the twin message to the device.

  ```json
  "properties": {
      "desired": {
          "device_count": 42,
      }
  }
  ```

  No other property names sent in a desired property message are supported. If any are sent, the log will report there is nothing to update.

  </p>
  </details>

### IoT Hub Plug and Play Sample

- *Executable:* `paho_iot_hub_pnp_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_pnp_sample.c) connects an IoT Plug and Play enabled device (a thermostat) with the Digital Twin Model ID (DTMI) detailed [here](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json). If a timeout occurs while waiting for a message from the Azure IoT Explorer, the sample will continue. If 3 timeouts occur consecutively, the sample will disconnect. X509 authentication is used.

  To interact with this sample, **you must use the Azure IoT Explorer**.

  <details><summary><i>How to interact with the Plug and Play sample:</i></summary>
  <p>

    The capabilities are listed below.

    <details><summary><b>Device Twin:</b></summary>
    <p>

    Two device twin properties are supported in this sample:
    - A desired property named `targetTemperature` with a `double` value for the desired temperature.
    - A reported property named `maxTempSinceLastReboot` with a `double` value for the highest temperature reached since device boot.
    <br>

    <b>To send a device twin desired property message:</b> Select your device's "Device Twin" tab in the Azure IoT Explorer. Add the property `targetTemperature` along with a corresponding value to the `desired` section of the device twin JSON. Select "Save" to update the document and send the twin message to the device.

    ```json
    "properties": {
        "desired": {
            "targetTemperature": 68.5,
        }
    }
    ```

    No other property names sent in a desired property message are supported. If any are sent, the log will report there is nothing to update.

    Upon receiving a desired property message, the sample will update the twin property locally and send a reported property of the same name back to the service. This message will include a set of "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an optional `ad` for an ack description. You will see the following in the device twin JSON.

    ```json
    "properties": {
        "reported": {
            "targetTemperature": {
              "value": 68.5,
              "ac": 200,
              "av": 14,
              "ad": "success"
            },
            "maxTempSinceLastReboot": 74.3,
        }
    }
    ```

    </p>
    </details>

    <details><summary><b>Direct Method (Command):</b></summary>
    <p>

    One device command is supported in this sample: `getMaxMinReport`.

    <b>To invoke a command:</b> Select your device's "Direct Method" tab in the Azure IoT Explorer. Enter the command name `getMaxMinReport` along with a payload using an [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) time format and select "Invoke method".

    ```json
    "2020-08-18T17:09:29-0700"
    ```

    The command will send back to the service a response containing the following JSON payload with updated values in each field:

    ```json
    {
      "maxTemp": 74.3,
      "minTemp": 65.2,
      "avgTemp": 68.79,
      "startTime": "2020-08-18T17:09:29-0700",
      "endTime": "2020-08-18T17:24:32-0700"
    }
    ```

    No other commands are supported. If any other commands are attempted to be invoked, the log will report the method is not found.

    </p>
    </details>

    <details><summary><b>Telemetry:</b></summary>
    <p>

    Device sends a JSON message with the property name `temperature` and a `double` value for the current temperature.

    </p>
    </details>

  </details>

### IoT Hub Plug and Play Multiple Component Sample

- *Executable:* `paho_iot_hub_pnp_component_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_pnp_component_sample.c) extends the IoT Hub Plug and Play Sample above to mimic a Temperature Controller and connects the IoT Plug and Play enabled device (the Temperature Controller) with the Digital Twin Model ID (DTMI) detailed [here](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json). If a timeout occurs while waiting for a message from the Azure IoT Explorer, the sample will continue. If 3 timeouts occur consecutively, the sample will disconnect. X509 authentication is used.

  This Temperature Controller is made up of the following components:

  - Device Info
  - [Temperature Sensor 1](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json)
  - [Temperature Sensor 2](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json)

  To interact with this sample, **you must use the Azure IoT Explorer**.

  <details><summary><i>How to interact with the Plug and Play Multiple Component sample:</i></summary>
  <p>

    The capabilities are listed below.

    <details><summary><b>Device Twin:</b></summary>
    <p>

    The following device twin properties are supported in this sample:

    Temperature Controller:
    - A reported property named `serialNumber` with a `string` value for the device serial number.

    Device Info:
    - A reported property named `manufacturer` with a `string` value for the name of the device manufacturer.
    - A reported property named `model` with a `string` value for the name of the device model.
    - A reported property named `swVersion` with a `string` value for the software version running on the device.
    - A reported property named `osName` with a `string` value for the name of the operating system running on the device.
    - A reported property named `processorArchitecture` with a `string` value for the name of the device architecture.
    - A reported property named `processorManufacturer` with a `string` value for the name of the device's processor manufacturer.
    - A reported property named `totalStorage` with a `double` value for the total storage in KiB on the device.
    - A reported property named `totalMemory` with a `double` value for the total memory in KiB on the device.

    Temperature Sensor:
    - A desired property named `targetTemperature` with a `double` value for the desired temperature.
    - A reported property named `maxTempSinceLastReboot` with a `double` value for the highest temperature reached since boot.

    On initial bootup of the device, the sample will send the Temperature Controller reported properties to the service. You will see the following in the device twin JSON.

    ```json
    "properties": {
        "reported": {
            "manufacturer": "Sample-Manufacturer",
            "model": "pnp-sample-Model-123",
            "swVersion": "1.0.0.0",
            "osName": "Contoso",
            "processorArchitecture": "Contoso-Arch-64bit",
            "processorManufacturer": "Processor Manufacturer(TM)",
            "totalStorage": 1024,
            "totalMemory": 128,
            "serialNumber": "ABCDEFG",
        }
    }
    ```

    <b>To send a device twin desired property message:</b> Select your device's Device Twin tab in the Azure IoT Explorer. Add the property targetTemperature along with a corresponding value to the desired section of the JSON. Select Save to update the twin document and send the twin message to the device.

    ```json
    "properties": {
        "desired": {
            "thermostat1": {
                "targetTemperature": 34.8
            },
            "thermostat2": {
                "targetTemperature": 68.5
            }
        }
    }
    ```

    No other property names sent in a desired property message are supported. If any are sent, the log will report there is nothing to update.

    Upon receiving a desired property message, the sample will update the twin property locally and send a reported property of the same name back to the service. This message will include a set of "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an optional `ad` for an ack description.

    ```json
    "properties": {
        "reported": {
            "thermostat1": {
                "__t": "c",
                "maxTempSinceLastReboot": 38.2,
                "targetTemperature": {
                    "value": 34.8,
                    "ac": 200,
                    "av": 27,
                    "ad": "success"
                }
            },
            "thermostat2": {
                "__t": "c",
                "maxTempSinceLastReboot": 69.1,
                "targetTemperature": {
                    "value": 68.5,
                    "ac": 200,
                    "av": 28,
                    "ad": "success"
                },
            }
        }
    }
    ```

    </p>
    </details>

    <details><summary><b>Direct Method:</b></summary>
    <p>

    Two device commands are supported in this sample: `reboot` and `getMaxMinReport`.

    <b>To invoke a command:</b> Select your device's Direct Method tab in the Azure IoT Explorer.

    - To invoke `reboot` on the Temperature Controller, enter the command name `reboot`. Select Invoke method.
    - To invoke `getMaxMinReport` on Temperature Sensor 1, enter the command name `thermostat1/getMaxMinReport` along with a payload using an [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) time format. Select Invoke method.
    - To invoke `getMaxMinReport` on Temperature Sensor 2, enter the command name `thermostat2/getMaxMinReport` along with a payload using an [ISO8601](https://en.wikipedia.org/wiki/ISO_8601) time format. Select Invoke method.

    ```json
    "2020-08-18T17:09:29-0700"
    ```

    The command will send back to the service a response containing the following JSON payload with updated values in each field:

    ```json
      {
        "maxTemp": 74.3,
        "minTemp": 65.2,
        "avgTemp": 68.79,
        "startTime": "2020-08-18T17:09:29-0700",
        "endTime": "2020-08-18T17:24:32-0700"
      }
    ```

    No other commands are supported. If any other commands are attempted to be invoked, the log will report the method is not found.

    </p>
    </details>

    <details><summary><b>Telemetry:</b></summary>
    <p>

    The Temperature Controller sends a JSON message with the property name `workingSet` and a `double` value for the current working set of the device memory in KiB. Also, each Temperature Sensor sends a JSON message with the property name `temperature` and a `double` value for the current temperature.

    </p>
    </details>

  </details>

### IoT Provisioning Certificate Sample

- *Executable:* `paho_iot_provisioning_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_provisioning_sample.c) registers a device with the Azure IoT Device Provisioning Service. It will wait to receive the registration status before disconnecting. X509 authentication is used.

### IoT Provisioning SAS Sample

- *Executable:* `paho_iot_provisioning_sas_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_provisioning_sas_sample.c) registers a device with the Azure IoT Device Provisioning Service. It will wait to receive the registration status before disconnecting. SAS authentication is used.

## Getting Started

### Set Environment Variables

Samples use environment variables for a variety of purposes, including filepaths and connection parameters. Please keep in mind, **every time a new terminal is opened, the environment variables will have to be reset**. Setting a variable will take the following form:

**Linux:**

```bash
export ENV_VARIABLE_NAME=VALUE
```

**Windows (PowerShell):**

```powershell
$env:ENV_VARIABLE_NAME='VALUE'
```

Set the following environment variables for all samples:

  1. Set the vcpkg environment variables.

      Refer to these [directions](https://github.com/Azure/azure-sdk-for-c#development-environment) for more detail.

      Linux:

      ```bash
      export VCPKG_DEFAULT_TRIPLET=x64-linux
      export VCPKG_ROOT=<FULL PATH to vcpkg>
      ```

      Windows (PowerShell):

      ```powershell
      $env:VCPKG_DEFAULT_TRIPLET='x64-windows-static' # Update triplet to match what was used during vcpkg install.
      $env:VCPKG_ROOT='<FULL PATH to vcpkg>'
      ```

  2. Set the trust pem filepath. **Only for Windows or if required by OS.**

      Download [BaltimoreCyberTrustRoot.crt.pem](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem) to `<FULL PATH TO azure-sdk-for-c>\sdk\samples\iot\`. Confirm the downloaded certificate uses the correct file name and file extension.

      Windows (PowerShell):

      ```powershell
      $env:AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH='<FULL PATH TO azure-sdk-for-c>\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem'
      ```

### Generate Device Certificate

For samples using certificates, x509 authentication is used to connect to Azure IoT Hub or Azure IoT Hub DPS.

**WARNING: Certificates created by these commands MUST NOT be used in production-level code on Windows or macOS.** These certificates expire after 365 days and are provided ONLY to help you easily understand CA Certificates. When productizing against CA Certificates, you will need to use your own security best practices for certificate creation and lifetime management.

  - *Executables:* `paho_iot_hub_c2d_sample`, `paho_iot_hub_methods_sample`, `paho_iot_hub_telemetry_sample`, `paho_iot_hub_twin_sample`, `paho_iot_hub_pnp_sample`, `paho_iot_hub_pnp_component_sample`, `paho_iot_provisioning_sample`

The resulting thumbprint will be placed in `fingerprint.txt` and the generated pem file is named `device_ec_cert.pem`.

**Linux:**

  1. Enter the directory `azure-sdk-for-c/sdk/samples/iot/`.
  2. Run the following commands:

      ```bash
      openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
      openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
      openssl x509 -noout -text -in device_ec_cert.pem

      rm -f device_cert_store.pem
      cat device_ec_cert.pem device_ec_key.pem > device_cert_store.pem

      openssl x509 -noout -fingerprint -in device_ec_cert.pem | sed 's/://g'| sed 's/\(SHA1 Fingerprint=\)//g' | tee fingerprint.txt

      export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=$(pwd)/device_cert_store.pem
      ```

**Windows (PowerShell):**

  1. Enter the directory `azure-sdk-for-c\sdk\samples\iot\`.
  2. Run the following commands:

      ```powershell
      openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
      openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
      openssl x509 -noout -text -in device_ec_cert.pem

      Get-Content device_ec_cert.pem, device_ec_key.pem | Set-Content device_cert_store.pem

      openssl x509 -noout -fingerprint -in device_ec_cert.pem | % {$_.replace(":", "")} | % {$_.replace("SHA1 Fingerprint=", "")} | Tee-Object fingerprint.txt

      $env:AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=$(Resolve-Path device_cert_store.pem)
      ```

## Sample Instructions

### IoT Hub Certificate Samples

*Executables:*
- `paho_iot_hub_c2d_sample`
- `paho_iot_hub_methods_sample`
- `paho_iot_hub_telemetry_sample`
- `paho_iot_hub_twin_sample`
- `paho_iot_hub_pnp_sample`
- `paho_iot_hub_pnp_component_sample`

<details><summary><i>Instructions to run a Hub Certificate sample:</i></summary>
<p>

1. In your Azure IoT Hub, add a new device using a self-signed certificate. See [here](https://docs.microsoft.com/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**DO NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**.

    For the Thumbprint, use the recently generated fingerprint, which has been placed in the file `fingerprint.txt`.

2. Set the following environment variables:

    - `AZ_IOT_HUB_DEVICE_ID`: Select your device from the IoT Devices page and copy its Device Id.
    - `AZ_IOT_HUB_HOSTNAME`: Copy the Hostname from the Overview tab in your Azure IoT Hub.

      Linux:

      ```bash
      export AZ_IOT_HUB_DEVICE_ID=<device-id>
      export AZ_IOT_HUB_HOSTNAME=<hostname>
      ```

      Windows (PowerShell):

      ```powershell
      $env:AZ_IOT_HUB_DEVICE_ID='<device-id>'
      $env:AZ_IOT_HUB_HOSTNAME='<hostname>'
      ```

3. [Build and run the sample](#build-and-run-the-sample).

4. See the sample description for interaction instructions:

    - [`paho_iot_hub_c2d_sample`](#iot-hub-c2d-sample)
    - [`paho_iot_hub_methods_sample`](#iot-hub-methods-sample)
    - [`paho_iot_hub_telemetry_sample`](#iot-hub-telemetry-sample)
    - [`paho_iot_hub_twin_sample`](#iot-hub-twin-sample)
    - [`paho_iot_hub_pnp_sample`](#iot-hub-plug-and-play-sample)
    - [`paho_iot_hub_pnp_component_sample`](#iot-hub-plug-and-play-muiltiple-component-sample)

</p>
</details>

### IoT Hub SAS Sample

*Executable:* `paho_iot_hub_sas_telemetry_sample`

<details><summary><i>Instructions to run a Hub SAS sample:</i></summary>
<p>

1. In your Azure IoT Hub, add a new device using a symmetric key. See [here](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) for further instruction.

2. Set the following environment variables:

    - `AZ_IOT_HUB_SAS_DEVICE_ID`: Select your device from the IoT Devices page and copy its Device Id.
    - `AZ_IOT_HUB_SAS_KEY`: Copy its Primary Key from the same page.
    - `AZ_IOT_HUB_HOSTNAME`: Copy the Hostname from the Overview tab in your Azure IoT Hub.

      Linux:

      ```bash
      export AZ_IOT_HUB_SAS_DEVICE_ID=<sas-device-id>
      export AZ_IOT_HUB_SAS_KEY=<sas-key>
      export AZ_IOT_HUB_HOSTNAME=<hostname>
      ```

      Windows (PowerShell):

      ```powershell
      $env:AZ_IOT_HUB_SAS_DEVICE_ID='<sas-device-id>'
      $env:AZ_IOT_HUB_SAS_KEY='<sas-key>'
      $env:AZ_IOT_HUB_HOSTNAME='<hostname>'
      ```

3. [Build and run the sample](#build-and-run-the-sample).

4. See the sample description for interaction instructions:

    - [`paho_iot_hub_sas_telemetry_sample`](#iot-hub-sas-telemetry-sample)

</p>
</details>

### IoT Provisioning Certificate Sample

*Executable:* `paho_iot_provisioning_sample`

<details><summary><i>Instructions to run a Provisioning Certificate sample:</i></summary>
<p>

1. In your Azure IoT Hub DPS, add a new individual device enrollment using the recently generated `device_ec_cert.pem` file. See [here](https://docs.microsoft.com/azure/iot-dps/quick-create-simulated-device-x509#create-a-device-enrollment-entry-in-the-portal) for further instruction. After creation, the Registration ID of your device should appear as `paho-sample-device1` in the Individual Enrollments tab.

2. Set the following environment variables:

    - `AZ_IOT_PROVISIONING_REGISTRATION_ID`: This should be `paho-sample-device1`.
    - `AZ_IOT_PROVISIONING_ID_SCOPE`: Copy the Id Scope from the Overview tab in your Azure IoT Hub DPS.

      Linux:

      ```bash
      export AZ_IOT_PROVISIONING_REGISTRATION_ID=<registration-id>
      export AZ_IOT_PROVISIONING_ID_SCOPE=<id-scope>
      ```

      Windows (PowerShell):

      ```powershell
      $env:AZ_IOT_PROVISIONING_REGISTRATION_ID='<registration-id>'
      $env:AZ_IOT_PROVISIONING_ID_SCOPE='<id-scope>'
      ```

3. [Build and run the sample](#build-and-run-the-sample).

4. See the sample description for interaction instructions:

    - [`paho_iot_provisioning_sample`](#iot-provisioning-sample)

</p>
</details>

### IoT Provisioning SAS Sample

*Executable:* `paho_iot_provisioning_sas_sample`

<details><summary><i>Instructions to run a Provisioning SAS sample:</i></summary>
<p>

1. In your Azure IoT Hub DPS, add a new individual device enrollment using a symmetric key. See [here](https://docs.microsoft.com/azure/iot-dps/quick-create-simulated-device-symm-key#create-a-device-enrollment-entry-in-the-portal) for further instruction. After creation, the Registration ID of your device will appear in the Individual Enrollments tab.

2. Set the following environment variables:

    - `AZ_IOT_PROVISIONING_SAS_REGISTRATION_ID`: Copy the Registration Id of your SAS device from the Individual Enrollments tab.
    - `AZ_IOT_PROVISIONING_SAS_KEY`: Select your SAS device from the Individual Enrollments tab and copy its Primary Key.
    - `AZ_IOT_PROVISIONING_ID_SCOPE`: Copy the Id Scope from the Overview tab in your Azure IoT Hub DPS.

      Linux:

      ```bash
      export AZ_IOT_PROVISIONING_SAS_REGISTRATION_ID=<sas-registration-id>
      export AZ_IOT_PROVISIONING_SAS_KEY=<sas-key>
      export AZ_IOT_PROVISIONING_ID_SCOPE=<id-scope>
      ```

      Windows (PowerShell):

      ```powershell
      $env:AZ_IOT_PROVISIONING_SAS_REGISTRATION_ID='<sas-registration-id>'
      $env:AZ_IOT_PROVISIONING_SAS_KEY='<sas-key>'
      $env:AZ_IOT_PROVISIONING_ID_SCOPE='<id-scope>'
      ```

3. [Build and run the sample](#build-and-run-the-sample).

4. See the sample description for interaction instructions:

    - [`paho_iot_provisioning_sas_sample`](#iot-provisioning-sas-sample)

</p>
</details>

## Build and Run the Sample

1. Build the Azure SDK for Embedded C directory structure.

    From the root of the SDK directory `azure-sdk-for-c`:

    Linux:

    ```bash
    mkdir build
    cd build
    cmake -DTRANSPORT_PAHO=ON ..
    ```

    Windows (PowerShell):

    ```powershell
    mkdir build
    cd build
    cmake -DTRANSPORT_PAHO=ON ..
    ```

2. Compile and run the sample.

    Linux:

    ```bash
    cmake --build .
    ./sdk/samples/iot/<sample executable here>
    ```

    Windows (PowerShell):

    ```powershell
    .\az.sln
    ```

    Once the Windows solution opens in Visual Studio:
    - Navigate on the "Solution Explorer" panel to the sample project you would like to run.
    - Right-click on the sample project, then click on "Set as Startup Project". (This makes it the default startup project.)
    - Build and run the project (`F5` on most installations).

## Next Steps and Additional Documentation

Start using the Azure Embedded C SDK IoT Clients in your solutions!

- A general overview of the Embedded C SDK and additional background on running samples can be found in the [Azure SDK for Embedded C README](https://github.com/Azure/azure-sdk-for-c#azure-sdk-for-embedded-c).
- More SDK details pertaining to the Azure IoT Client library can be found in the [Azure IoT Client README](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients).
- The [Azure IoT Client MQTT State Machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md) provides a high-level architecture and API information.
- For extensive documentation on Azure IoT Hub, see the [Microsoft API reference documentation](https://docs.microsoft.com/azure/iot-hub/about-iot-hub).
- For extensive documentation on Azure IoT Hub Device Provisioning Service, see the [Microsoft API reference documentation](https://docs.microsoft.com/azure/iot-dps/).

## Troubleshooting

- The error policy for the Embedded C SDK client library is documented [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#error-policy).
- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using the `azure` and `c` tags.

## Contributing

This project welcomes contributions and suggestions. Find more contributing details [here](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

### License

Azure SDK for Embedded C is licensed under the [MIT](https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE) license.

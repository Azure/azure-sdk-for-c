# Azure Embedded C SDK IoT Samples

## Introduction

This document explains samples for the Azure Embedded C SDK IoT Hub Client and Device Provisioning Client.

Samples are designed to highlight the function calls required to connect with the Azure IoT Hub or the Azure IoT Hub Device Provisioning Service (DPS). These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production code on Windows or macOS**.

The samples' instructions include specifics for both Windows and Linux based systems. For Windows, the command line examples are based on the Command Prompt and not PowerShell. The Linux examples are tailored to Debian/Ubuntu environments. Samples are also designed to work on macOS systems, but the instructions do not yet include specific command line examples for this environment. While Windows and Linux devices are not likely to be considered constrained, these samples enable one to test the Azure SDK for Embedded C libraries, even without a real device.

More detailed step-by-step guides on how to run an IoT Hub Client sample from scratch can be found below:

- Linux: [How to setup and run Azure SDK for Embedded C IoT Hub Samples on Linux](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_samples_linux.md)
- Windows: [How to setup and run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_samples_windows.md)
- ESP8266: [How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Esp8266 NodeMCU](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/docs/how_to_iot_hub_esp8266_nodemcu.md)

## Sample Descriptions

This section provides an overview of the different samples available to run and what to expect from each.

### IoT Hub C2D Sample

- *Executable:* `paho_iot_hub_c2d_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_c2d_sample.c) receives incoming cloud-to-device (C2D) messages invoked from the Azure IoT Hub. It will successfully receive up to 5 messages sent from the service. If a timeout occurs while waiting for a message, the sample will exit. X509 self-certification is used.

  To send C2D messages, select your device's Message to Device tab in your Azure IoT Hub. Enter a message in the Message Body and select Send Message.

### IoT Hub Methods Sample

- *Executable:* `paho_iot_hub_methods_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_methods_sample.c) receives incoming method commands invoked from the the Azure IoT Hub. It will successfully receive up to 5 method commands sent from the service. If a timeout occurs while waiting for a message, the sample will exit. X509 self-certification is used.

  To send a method command, select your device's Direct Method tab in your Azure IoT Hub. Enter a method name and select Invoke Method. A method named `ping` is supported, which if successful will return a json payload of the following:

  ```json
  {"response": "pong"}
  ```

  No other method commands are supported. If any are attempted to be invoked, the log will report the method is not found.

### IoT Hub Telemetry Sample

- *Executable:* `paho_iot_hub_telemetry_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_telemetry_sample.c) sends five telemetry messages to the Azure IoT Hub. X509 self-certification is used.

### IoT Hub SAS Telemetry Sample

- *Executable:* `paho_iot_hub_sas_telemetry_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_sas_telemetry_sample.c) sends five telemetry messages to the Azure IoT Hub. SAS certification is used.

### IoT Hub Twin Sample

- *Executable:* `paho_iot_hub_twin_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_twin_sample.c) utilizes the Azure IoT Hub to get the twin document, send a reported property message, and receive up to 5 desired property messages. If a timeout occurs while waiting for a message from the Azure IoT Hub, the sample will exit. Upon receiving a desired property message, the sample will update the property locally and send a reported property message back to the service. X509 self-certification is used.

  A property named `device_count` is supported for this sample. To send a device twin desired property message, select your device's Device Twin tab in your Azure IoT Hub. Add the property `device_count` along with a corresponding value to the `desired` section of the JSON. Select Save to send the message.

  ```json
  {
    "properties": {
      "desired": {
        "device_count": 42,
      }
    }
  }
  ```

  No other property names sent in a desired property message are supported. If any are sent, the log will report there is nothing to update.

### IoT Hub Plug and Play Sample

- *Executable:* `paho_iot_hub_pnp_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_pnp_sample.c) connects an IoT Plug and Play enabled device with the Digital Twin Model ID (DTMI) detailed [here](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json). X509 self-certification is used.

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
- **Twin**: Desired property with the field name `targetTemperature` and the `double` value for the desired temperature. Reported property with the field name `maxTempSinceLastReboot` and the `double` value for the highest temperature. Note that part of the IoT Plug and Play spec is a response to a desired property update from the service. The device will send back a reported property with a similarly named property and a set of "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an optional `ad` for an ack description.

### IoT Hub Plug and Play Multiple Component

- *Executable:* `paho_iot_hub_pnp_component_example`

This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_hub_pnp_component_example.c) connects an IoT Plug and Play enabled device with the Digital Twin Model ID (DTMI) detailed [here](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json). X509 self-certification is used.

This temperature controller is made up of the following sub-components

- Temperature Sensor 1
- Temperature Sensor 2
- Device Info

Link to the component DTMI can be found [here](https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/TemperatureController.json).

### IoT Provisioning Sample

- *Executable:* `paho_iot_provisioning_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_provisioning_sample.c) registers a device with the Azure IoT Hub Device Provisioning Service. It will wait to receive the registration status before disconnecting. X509 self-certification is used.

### IoT Provisioning SAS Sample

- *Executable:* `paho_iot_provisioning_sas_sample`

  This [sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/paho_iot_provisioning_sas_sample.c) registers a device with the Azure IoT Hub Device Provisioning Service. It will wait to receive the registration status before disconnecting. SAS certification is used.

## Prerequisites

To run the samples, ensure you have the following programs or tools installed on your system:

- Have an [Azure account](https://azure.microsoft.com/en-us/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
- Have an [Azure IoT Hub Device Provisioning Service (DPS)](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision) created if using a DPS sample.
- Have [git](https://git-scm.com/download) installed.
- Have [OpenSSL](https://www.openssl.org/source/) installed:
  - For Linux based systems, we recommend:

    ```bash
    sudo apt-get install libssl-dev
    ```

  - For non-Linux based systems, download the [OpenSSL 1.1.1 LTS](https://www.openssl.org/source/openssl-1.1.1g.tar.gz) command line utility and follow the downloaded INSTALL document.
- Have the following build setup:
  - For Linux based systems, have make installed:

    ```bash
    sudo apt-get install build-essential
    ```

  - For Windows systems, have [Microsoft Visual Studio](https://visualstudio.microsoft.com/downloads/) installed.
  - For all systems, have the latest version of [CMake](https://cmake.org/download) installed.
- Have Microsoft [VCPKG](https://github.com/microsoft/vcpkg) package manager and [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. Use the directions [here](https://github.com/Azure/azure-sdk-for-c#development-environment) to download VCPKG and install Paho MQTT.

## Getting Started

### Environment Variables

Samples use environment variables for a variety of purposes, including filepaths and connection parameters. Please keep in mind, **every time a new terminal is opened, the environment variables will have to be reset**. Setting a variable will take the following form:

Linux:

```bash
export ENV_VARIABLE_NAME=VALUE
```

Windows:

```cmd
set ENV_VARIABLE_NAME=VALUE
```

Set the following environment variables for all samples:

- `VCPKG_DEFAULT_TRIPLET` and `VCPKG_ROOT`: Refer to these [directions](https://github.com/Azure/azure-sdk-for-c#development-environment).
- `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE`: **Only for Windows or if required by OS.** Download [BaltimoreCyberTrustRoot.crt.pem](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem) to `\azure-sdk-for-c\sdk\samples\iot\`. Copy the full filepath to this downloaded .pem file, e.g. `C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem`.

### Certificate Samples

The following samples use x509 authentication to connect to Azure IoT Hub or Azure IoT Hub DPS. To easily run these samples, we have provided a script to generate a self-signed device certification used for device authentication. **This script is intended for sample use only and not to be used in production code**.

1. Enter the directory `/azure-sdk-for-c/sdk/samples/iot/` and run the script using the following form:

    Linux:

    ```bash
    ./generate_certificate.sh
    ```

    Windows:

    ```cmd
    generate_certificate.cmd
    ```

2. Set the following environment variable:

    - `AZ_IOT_DEVICE_X509_CERT_PEM_FILE`: Copy the path of the generated .pem file noted in the generate_certificate output.

#### IoT Hub Certificate Samples

*Executables:* `paho_iot_hub_c2d_sample`, `paho_iot_hub_methods_sample`, `paho_iot_hub_telemetry_sample`, `paho_iot_hub_twin_sample`, `paho_iot_hub_pnp_sample`

1. In your Azure IoT Hub, add a new device using a self-signed certificate.  See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**do NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**. For the Thumbprint, use the recently generated fingerprint noted at the bottom of the generate_certificate output. (It is also placed in a file named `fingerprint.txt` for your convenience).

2. Set the following environment variables:

    - `AZ_IOT_DEVICE_ID`: Select your device from the IoT Devices page and copy its Device Id.
    - `AZ_IOT_HUB_HOSTNAME`: Copy the Hostname from the Overview tab in your Azure IoT Hub.

#### IoT Hub DPS Certificate Sample

*Executables:* `paho_iot_provisioning_sample`

1. In your Azure IoT Hub DPS, add a new individual device enrollment using the recently generated `device_ec_cert.pem` file. See [here](https://docs.microsoft.com/en-us/azure/iot-dps/quick-create-simulated-device-x509#create-a-device-enrollment-entry-in-the-portal) for further instruction. After creation, the Registration ID of your device should appear as `paho-sample-device1` in the Individual Enrollments tab.

2. Set the following environment variables:

    - `AZ_IOT_REGISTRATION_ID`: This should be `paho-sample-device1`.
    - `AZ_IOT_ID_SCOPE`: Copy the Id Scope from the Overview tab in your Azure IoT Hub DPS.

### SAS Samples

The following samples use SAS authentication to connect to Azure IoT Hub or Azure IoT Hub DPS.

#### IoT Hub SAS Sample

*Executables:* `paho_iot_hub_sas_telemetry_sample`

1. In your Azure IoT Hub, add a new device using a symmetric key. See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) for further instruction.

2. Set the following environment variables:

    - `AZ_IOT_DEVICE_ID_SAS`: Select your device from the IoT Devices page and copy its Device Id.
    - `AZ_IOT_HUB_DEVICE_SAS_KEY`: Copy its Primary Key from the same page.
    - `AZ_IOT_HUB_HOSTNAME`: Copy the Hostname from the Overview tab in your Azure IoT Hub.

#### IoT Hub DPS SAS Sample

*Executables:* `paho_iot_provisioning_sas_sample`

1. In your Azure IoT Hub DPS, add a new individual device enrollment using a symmetric key. See [here](https://docs.microsoft.com/en-us/azure/iot-dps/quick-create-simulated-device-symm-key#create-a-device-enrollment-entry-in-the-portal) for further instruction. After creation, the Registration ID of your device will appear in the Individual Enrollments tab.

2. Set the following environment variables:

    - `AZ_IOT_REGISTRATION_ID_SAS`: Copy the Registration Id of your SAS device from the Individual Enrollments tab.
    - `AZ_IOT_PROVISIONING_SAS_KEY`: Select your SAS device from the Individual Enrollments tab and copy its Primary Key.
    - `AZ_IOT_ID_SCOPE`: Copy the Id Scope from the Overview tab in your Azure IoT Hub DPS.

## Build and Run the Sample

1. Compile the code:

    - Enter the directory `/azure-sdk-for-c/cmake`. If it does not exist, please create it.
    - Build the directory structure and the samples:

      ```bash
      cmake -DTRANSPORT_PAHO=ON ..
      cmake --build .
      ```

2. From within the cmake directory, run the sample:

    Linux:

    ```bash
    ./sdk/samples/iot/<sample executable here>
    ```

    Windows:

    ```cmd
    az.sln
    ```

    Once the Windows solution opens in Visual Studio:
    - Navigate on the Solution Explorer panel to the sample project you would like to run.
    - Make it the default startup project (right-click on the sample project, then click on Set as Startup Project).
    - Build and run the project (F5 on most installations).

## Next Steps and Additional Documentation

Start using the IoT Provisioning Client in your solutions!

- A general overview of the Embedded C SDK and additional background on running samples can be found in the [Azure SDK for Embedded C README](https://github.com/Azure/azure-sdk-for-c#azure-sdk-for-embedded-c).
- More SDK details pertaining to the Azure IoT Client library can be found in the [Azure IoT Client README](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot#azure-iot-clients).
- The [Azure IoT Client MQTT State Machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md) provides a high-level architecture and API information.
- For extensive documentation on Azure IoT Hub, see the [Microsoft API reference documentation](https://docs.microsoft.com/en-us/azure/iot-hub/about-iot-hub).
- For extensive documentation on Azure IoT Hub Device Provisioning Service, see the [Microsoft API reference documentation](https://docs.microsoft.com/en-us/azure/iot-dps/).

## Troubleshooting

- The error policy for the Embedded C SDK client library is documented [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#error-policy).
- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Contributing

This project welcomes contributions and suggestions. Find more contributing details [here](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

### License

Azure SDK for Embedded C is licensed under the [MIT](https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE) license.

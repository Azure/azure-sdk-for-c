# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows

- [How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows](#how-to-setup-and-run-azure-sdk-for-embedded-c-iot-hub-samples-on-microsoft-windows)
  - [Introduction](#introduction)
    - [What is Covered](#what-is-covered)
  - [Prerequisites](#prerequisites)
  - [Azure SDK for Embedded C Setup Instructions](#azure-sdk-for-embedded-c-setup-instructions)
  - [Configure and Run the IoT Hub Client Certificate Samples](#configure-and-run-the-iot-hub-client-certificate-samples)
  - [Sample Instructions](#sample-instructions)
    - [IoT Hub C2D Sample](#iot-hub-c2d-sample)
    - [IoT Hub Methods Sample](#iot-hub-methods-sample)
    - [Telemetry (device-to-cloud messages)](#telemetry-device-to-cloud-messages)
    - [IoT Hub Twin Sample](#iot-hub-twin-sample)
  - [Troubleshooting](#troubleshooting)
  - [Contributing](#contributing)
    - [License](#license)

## Introduction

This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Certificate Samples running on Microsoft Windows.

Samples are designed to highlight the function calls required to connect with the Azure IoT Hub. These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production-level code on Windows or macOS**.

For Windows, the command line examples are based on PowerShell. While Windows devices are not likely to be considered constrained, these samples enable one to test the Azure SDK for Embedded C libraries, even without a real device.

**WARNING: Samples are generic and should not be used in any production-level code.**

### What is Covered

- Setup instructions for the Azure SDK for Embedded C suite.
- Configuration, build, and run instructions for the IoT Hub Client Certificate Samples.

    _The following was run on Microsoft Windows 10.0.18363.836._

## Prerequisites

To run the samples, ensure you have the following programs and tools installed on your system:

- Have an [Azure account](https://azure.microsoft.com/en-us/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
- Have [Microsoft Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) installed with [C and C++ support](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2019).
- Have [Git](https://git-scm.com/download/win) for Windows installed.
- Have Microsoft [vcpkg](https://github.com/microsoft/vcpkg) package manager and [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. This installation may take an extended amount of time (~20-30 minutes).

    ```powershell
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    .\bootstrap-vcpkg.bat
    .\vcpkg.exe install --triplet x64-windows-static curl[winssl] cmocka paho-mqtt # Update triplet per your system.
    ```

- Have OpenSSL installed.

    OpenSSL will be installed by vcpkg as a dependency for Eclipse Paho. **WARNING: It is NOT recommended to use OpenSSL in production-level code on Windows or macOS.**

    ```powershell
    # NOT RECOMMENDED to use for production-level code.
    $env:PATH=$env:PATH + ';<FULL PATH to vcpkg>\installed\x64-windows-static\tools\openssl' # Update complete path as needed.
    ```

- Have the latest version of [CMake](https://cmake.org/download) installed.
- Have the Azure SDK for Embedded C IoT repository cloned.

    NOTE: Due to the length of the repository filepaths, clone near the `C:\` root.

    ```powershell
    git clone https://github.com/Azure/azure-sdk-for-c.git
    ```

## Azure SDK for Embedded C Setup Instructions

1. Set the vcpkg environment variables.

    ```powershell
    C:\vcpkg> $env:VCPKG_DEFAULT_TRIPLET='x64-windows-static'
    C:\vcpkg> $env:VCPKG_ROOT='C:\vcpkg'
    ```

    NOTE: Please keep in mind, **every time a new terminal is opened, the environment variables will have to be reset**.

2. Set the trust pem file path.

    Download [BaltimoreCyberTrustRoot.crt.pem](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem) to `<FULL PATH TO azure-sdk-for-c>\sdk\samples\iot\`. Confirm the downloaded certificate uses the correct file name and file extension.

    Windows (PowerShell):

    ```powershell
    $env:AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH='<FULL PATH TO azure-sdk-for-c>\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem'
    ```

## Configure and Run the IoT Hub Client Certificate Samples

1. Generate a self-signed certificate.

    **WARNING: Certificates created by these commands MUST NOT be used in production-level code on Windows or macOS.** These certificates expire after 365 days and are provided ONLY to help you easily understand CA Certificates. When productizing against CA Certificates, you will need to use your own security best practices for certificate creation and lifetime management.

    The resulting thumbprint will be placed in `fingerprint.txt` and the generated pem file is named `device_ec_cert.pem`.

    Enter the directory `azure-sdk-for-c\sdk\samples\iot\`.

    ```powershell
    openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
    openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
    openssl x509 -noout -text -in device_ec_cert.pem

    Get-Content device_ec_cert.pem, device_ec_key.pem | Set-Content device_cert_store.pem

    openssl x509 -noout -fingerprint -in device_ec_cert.pem | % {$_.replace(":", "")} | % {$_.replace("SHA1 Fingerprint=", "")} | Tee-Object fingerprint.txt

    $env:AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=$(Resolve-Path device_cert_store.pem)
    ```

2. Create a logical device.

    In your Azure IoT Hub, add a new device using a self-signed certificate. See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**DO NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**.

    For the Thumbprint, use the recently generated fingerprint, which has been placed in the file `fingerprint.txt`.

3. Set the remaining environment variables needed for the samples.

    - `AZ_IOT_HUB_DEVICE_ID`: Select your device from the IoT Devices page and copy its Device Id. (In this example it is "paho-sample-device1".)
    - `AZ_IOT_HUB_HOSTNAME`: Copy the Hostname from the Overview tab in your Azure IoT Hub. (In this example it is "myiothub.azure-devices.net".)

    ```powershell
    C:\azure-sdk-for-c\sdk\samples\iot> env:AZ_IOT_HUB_DEVICE_ID='paho-sample-device1'
    C:\azure-sdk-for-c\sdk\samples\iot> $env:AZ_IOT_HUB_HOSTNAME='myiothub.azure-devices.net'
    ```

4. Build the Azure SDK for Embedded C directory structure:

    From the sdk root directory `azure-sdk-for-c`:

    ```powershell
    C:\azure-sdk-for-c> mkdir build
    C:\azure-sdk-for-c> cd build
    C:\azure-sdk-for-c\build> cmake -DTRANSPORT_PAHO=ON ..
    ```

 5. Compile and run the sample from within the `build` directory:

    ```powershell
    C:\azure-sdk-for-c\build> .\az.sln
    ```

    Once the Windows solution opens in Visual Studio:
    - Navigate on the "Solution Explorer" panel to the sample project you would like to run.
    - Right-click on the sample project, then click on "Set as Startup Project". (This makes it the default startup project.)
    - Build and run the project (`F5` on most installations).

## Sample Instructions

### IoT Hub C2D Sample

- *Executable:* `paho_iot_hub_c2d_sample`

For the sample description and interaction instructions, please go [here](https://github.com/momuno/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md#iot-hub-c2d-sample).

### IoT Hub Methods Sample

- *Executable:* `paho_iot_hub_methods_sample`

For the sample description and interaction instructions, please go [here](https://github.com/momuno/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md#iot-hub-methods-sample).

### Telemetry (device-to-cloud messages)

- *Executable:* `paho_iot_hub_telemetry_sample`

For the sample description and interaction instructions, please go [here](https://github.com/momuno/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md#iot-hub-telemetry-sample).

### IoT Hub Twin Sample

- *Executable:* `paho_iot_hub_twin_sample`

For the sample description and interaction instructions, please go [here](https://github.com/momuno/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md#iot-hub-twin-sample).

## Troubleshooting

- The error policy for the Embedded C SDK client library is documented [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#error-policy).
- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using the `azure` and `c` tags.

## Contributing

This project welcomes contributions and suggestions. Find more contributing details [here](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

### License

Azure SDK for Embedded C is licensed under the [MIT](https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE) license.

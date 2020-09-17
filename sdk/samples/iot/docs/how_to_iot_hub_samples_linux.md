# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux

- [How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux](#how-to-setup-and-run-azure-sdk-for-embedded-c-iot-hub-samples-on-linux)
  - [Introduction](#introduction)
    - [What is Covered](#what-is-covered)
  - [Prerequisites](#prerequisites)
  - [Azure SDK for Embedded C Setup Instructions](#azure-sdk-for-embedded-c-setup-instructions)
  - [Configure and Run the IoT Hub Client Certificate Samples](#configure-and-run-the-iot-hub-client-certificate-samples)
  - [Sample Instructions](#sample-instructions)
    - [IoT Hub C2D Sample](#iot-hub-c2d-sample)
    - [IoT Hub Methods Sample](#iot-hub-methods-sample)
    - [IoT Hub Telemetry Sample](#iot-hub-telemetry-sample)
    - [IoT Hub Twin Sample](#iot-hub-twin-sample)
  - [Troubleshooting](#troubleshooting)
  - [Contributing](#contributing)
    - [License](#license)

## Introduction

This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Certificate Samples running on Linux.

Samples are designed to highlight the function calls required to connect with the Azure IoT Hub. These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production-level code on Windows or macOS**.

For Linux, the examples are tailored to Debian/Ubuntu environments. While Linux devices are not likely to be considered constrained, these samples enable one to test the Azure SDK for Embedded C libraries, even without a real device.

**WARNING: Samples are generic and should not be used in any production-level code.**

### What is Covered

- Setup instructions for the Azure SDK for Embedded C suite.
- Configuration, build, and run instructions for the IoT Hub Client Certificate Samples.

_The following was run on an Ubuntu Desktop 18.04 environment, but it also works on WSL 1 and 2 (Windows Subsystem for Linux)._

## Prerequisites

To run the samples, ensure you have the following programs and tools installed on your system:

- Have an [Azure account](https://azure.microsoft.com/en-us/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
- Have the following build environment setup:

    ```bash
    sudo apt-get update
    sudo apt-get install build-essential # make and gcc
    sudo apt-get install curl unzip tar pkg-config
    ```

- Have [Git](https://git-scm.com/download/linux) for Linux installed.

    ```bash
    sudo apt-get install git
    ```

- Have Microsoft [vcpkg](https://github.com/microsoft/vcpkg) package manager and [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. This installation may take an extended amount of time (~20-30 minutes).

    ```bash
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg install --triplet x64-linux curl cmocka paho-mqtt
    ```

- Have OpenSSL installed.

    ```bash
    sudo apt-get install openssl
    sudo apt-get install libssl-dev
    ```

- Have the latest version of [CMake](https://cmake.org/download) installed.

    ```bash
    sudo apt-get purge cmake
    sudo tar -xvzf cmake-<latest-version>.tar.gz
    cd cmake-<latest-version>
    ./bootstrap && make && sudo make install
    ```

- Have the Azure SDK for Embedded C IoT repository cloned.

    ```bash
    git clone https://github.com/Azure/azure-sdk-for-c.git
    ```

## Azure SDK for Embedded C Setup Instructions

1. Set the vcpkg environment variables.

    ```bash
    export VCPKG_DEFAULT_TRIPLET=x64-linux
    export VCPKG_ROOT=<FULL PATH to vcpkg>
    ```

    NOTE: Please keep in mind, **every time a new terminal is opened, the environment variables will have to be reset**.

## Configure and Run the IoT Hub Client Certificate Samples

1. Generate a self-signed certificate.

    **WARNING: Certificates created by these commands MUST NOT be used in production-level code on Windows or macOS.** These certificates expire after 365 days and are provided ONLY to help you easily understand CA Certificates. When productizing against CA Certificates, you will need to use your own security best practices for certificate creation and lifetime management.

    The resulting thumbprint will be placed in `fingerprint.txt` and the generated pem file is named `device_ec_cert.pem`.

    Enter the directory `azure-sdk-for-c/sdk/samples/iot/`.

    ```bash
    openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
    openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
    openssl x509 -noout -text -in device_ec_cert.pem

    rm -f device_cert_store.pem
    cat device_ec_cert.pem device_ec_key.pem > device_cert_store.pem

    openssl x509 -noout -fingerprint -in device_ec_cert.pem | sed 's/://g'| sed 's/\(SHA1 Fingerprint=\)//g' | tee fingerprint.txt

    export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=$(pwd)/device_cert_store.pem
    ```

2. Create a logical device

    In your Azure IoT Hub, add a new device using a self-signed certificate. See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**DO NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**.

    For the Thumbprint, use the recently generated fingerprint, which has been placed in the file `fingerprint.txt`.

3. Set the remaining environment variables needed for the samples.

    - `AZ_IOT_HUB_DEVICE_ID`: Select your device from the IoT Devices page and copy its Device Id. (In this example it is "testdevice-x509".)
    - `AZ_IOT_HUB_HOSTNAME`: Copy the Hostname from the Overview tab in your Azure IoT Hub. (In this example it is "myiothub.azure-devices.net".)

    ```bash
    export AZ_IOT_HUB_DEVICE_ID=testdevice-x509
    export AZ_IOT_HUB_HOSTNAME=myiothub.azure-devices.net
    ```

4. Build the Azure SDK for Embedded C directory structure:

    From the sdk root directory `azure-sdk-for-c`:

    ```bash
    mkdir build
    cd build
    cmake -DTRANSPORT_PAHO=ON ..
    ```

5. Compile and run the sample from within the `build` directory:

    ```bash
    make
    ./sdk/samples/iot/<sample-executable-here>
    ```

## Sample Instructions

### IoT Hub C2D Sample

- *Executable:* `paho_iot_hub_c2d_sample`

For the sample description and interaction instructions, please go [here](https://github.com/momuno/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md#iot-hub-c2d-sample).

### IoT Hub Methods Sample

- *Executable:* `paho_iot_hub_methods_sample`

For the sample description and interaction instructions, please go [here](https://github.com/momuno/azure-sdk-for-c/blob/master/sdk/samples/iot/README.md#iot-hub-methods-sample).

### IoT Hub Telemetry Sample

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

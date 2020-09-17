# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux

## Introduction

Prerequisites:
=======
This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Certificate Samples running on Linux.

Samples are designed to highlight the function calls required to connect with the Azure IoT Hub. These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production-level code on Windows or macOS**.

For Linux, the examples are tailored to Debian/Ubuntu environments. While Linux devices are not likely to be considered constrained, these samples enable one to test the Azure SDK for Embedded C libraries, even without a real device.

### WARNING: Samples are generic and should not be used in any production-level code.

### Prerequisites

- Have an [Azure account](https://azure.microsoft.com/en-us/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
- Have `make` and `gcc` installed. Have tools and `libssl-dev` installed:
    <details><summary> Click for further information:</summary>
    <p>

    ```bash
    sudo apt-get update
    sudo apt-get install build-essential
    sudo apt-get install curl unzip tar pkg-config
    sudo apt-get install libssl-dev
    ```

    Note: `libssl-dev` must be installed as a prerequisite to installing CMake.

    </p>
    </details>

- Have [Git](https://git-scm.com/download/linux) for Linux installed:

    ```bash
    sudo apt-get install git
    ```

- Have the latest version of [CMake](https://cmake.org/download) installed.
    <details><summary> Click for further information:</summary>
    <p>

    Purge any apt-get installed cmake:

    ```bash
    sudo apt-get purge cmake
    ```

    Once you have downloaded the most recent tar.gz cmake file, untar it and install:

    ```bash
    sudo tar -xvzf cmake-3.18.2.tar.gz
    cd cmake-3.18.2/
    sudo ./bootstrap && make && sudo make install
    ```

    </p>
    </details>

### What is Covered

- Setup instructions for the Azure SDK for Embedded C suite.
- Configuration, build, and run instructions for the IoT Hub Client Certificate Samples.

_The following was run on an Ubuntu Desktop 18.04 environment, but it also works on WSL 1 and 2 (Windows Subsystem for Linux)._

## Azure SDK for Embedded C Setup Instructions

1. Install Paho using vcpkg.

    The Azure IoT SDK for C uses Eclipse Paho for C installed via [vcpkg](https://github.com/Microsoft/vcpkg) (for the CMake integration).  This installation may take an extended amount of time.

    ```bash
    git clone https://github.com/Microsoft/vcpkg
    cd vcpkg/
    ./bootstrap-vcpkg.sh
    ./vcpkg install --triplet x64-linux curl cmocka paho-mqtt
    ```

2.  Set the vcpkg environment variables.

    Confirm `VCPKG_ROOT` is the path where vcpkg was cloned.

    ```bash
    export VCPKG_DEFAULT_TRIPLET=x64-linux
    export VCPKG_ROOT=PATH_TO_VCPKG #replace PATH_TO_VCPKG with full path to vcpkg/
    ```

    NOTE: Setting an environment variable only applies to the current session.  If you open a new window, the variable will have to be reset.

3. Clone the Azure Embedded SDK for C.

    ```bash
    cd ..
    git clone https://github.com/azure/azure-sdk-for-c
    ```

## Configure and Run the IoT Hub Client Certificate Samples

1. Generate a self-signed certificate.

    **WARNING: This script is intended for sample use only and should not be used in any production-level code.**

    The Azure Embedded SDK for C IoT Client Certificate Samples use a self-signed certificate. A script is provided for creating that certificate. You must use PowerShell Core to execute this script.

    ```powershell
    ./azure-sdk-for-c/sdk/samples/iot/generate_certificate.ps1
    ```

    <details><summary>Complete output of the `generate_certificate.ps1` script.</summary>
    <p>

    ```powershell
    WARNING: Certificates created by this script MUST NOT be used for production.
    WARNING: They expire after 365 days, and most importantly are provided for demonstration purposes to help you quickly understand CA Certificates.
    WARNING: When productizing against CA Certificates, you'll need to use your own security best practices for certification creation and lifetime management.
    Certificate:
        Data:
            Version: 1 (0x0)
            Serial Number:
                48:36:a4:16:dd:a5:59:bb:3d:f4:cc:30:91:75:a5:d2:82:f0:c0:4b
            Signature Algorithm: ecdsa-with-SHA256
            Issuer: CN = paho-sample-device1
            Validity
                Not Before: Sep 15 12:58:01 2020 GMT
                Not After : Sep 15 12:58:01 2021 GMT
            Subject: CN = paho-sample-device1
            Subject Public Key Info:
                Public Key Algorithm: id-ecPublicKey
                    Public-Key: (256 bit)
                    pub:
                        04:6d:f4:06:9f:5d:81:dd:64:39:ee:00:3a:d4:36:
                        c0:6f:6d:9e:66:9a:ec:0d:1f:1c:10:6a:0c:81:d1:
                        9a:d7:80:f9:b2:ba:d5:b1:ed:0a:8c:7a:91:5c:04:
                        2e:07:31:33:d5:38:83:84:f3:95:6e:58:5d:93:da:
                        6e:84:2e:a6:26
                    ASN1 OID: prime256v1
                    NIST CURVE: P-256
        Signature Algorithm: ecdsa-with-SHA256
            30:45:02:21:00:af:0f:ae:15:82:c4:0f:45:8b:18:47:3f:7d:
            9b:33:15:e6:3d:90:59:77:c6:5f:a1:f2:45:35:38:b8:36:f8:
            14:02:20:05:d1:1c:d5:1a:c0:98:af:e7:2f:33:cc:b1:3e:55:
            8d:da:bd:dd:3a:87:08:c1:7d:95:a6:14:8f:7a:a0:7f:38

    SAMPLE CERTIFICATE GENERATED:
    Use the following command to set the environment variable for the samples:

            export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=/azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem

    DPS SAMPLE:
    Upload device_ec_cert.pem when enrolling your device with the Device Provisioning Service.

    IOT HUB SAMPLES:
    Use the following fingerprint when creating your device in IoT Hub.
    (The fingerprint has also been placed in fingerprint.txt for future reference.)
    SHA1 Fingerprint=4086E30F95586BF6AC7C37C6C71D94F7167394D6
    ```

    </p>
    </details>

2.  Set the environment variable from the `generate_certificate.ps1` script output.

    NOTE: Do not copy this file path. Please use the output generated from running the script on your system.

    ```bash
    export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=/azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    ```

3.  Save the certificate Fingerprint from the `generate_certificate.ps1` script output.

    In this example, it is "4086E30F95586BF6AC7C37C6C71D94F7167394D6".
    It will be used to create the logical device on your Azure IoT Hub.

4. Create a logical device

    In your Azure IoT Hub, add a new device using a self-signed certificate.  See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**DO NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**.

    For the Thumbprint, use the recently generated fingerprint noted at the bottom of the `generate_certificate.ps1` output. (It is also placed in a file named `fingerprint.txt` for your convenience).

5. Set the remaining environment variables needed for the samples.

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Select your device from the IoT Devices page and copy its Device Id. (In this example it is "testdevice-x509".)
    - Set the associated environment variable:

    ```bash
    export AZ_IOT_HUB_DEVICE_ID=testdevice-x509
    ```

    - Copy the Hostname from the Overview tab in your Azure IoT Hub. (In this example it is "myiothub.azure-devices.net".)
    - Set the associated environment variable:

    ```bash
    export AZ_IOT_HUB_HOSTNAME=myiothub.azure-devices.net
    ```


6. Build the Azure SDK for Embedded Cc

    From the root of the cloned repository `azure-sdk-for-c/`:

    ```bash
    mkdir build
    cd build
    cmake -DTRANSPORT_PAHO=ON ..
    make
    ```

7. Run the samples.

    ```bash
    cd sdk/samples/iot/
    ./<sample-executable-here>
    ```

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

## Need Help?

- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.
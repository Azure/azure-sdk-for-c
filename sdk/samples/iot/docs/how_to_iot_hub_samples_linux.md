# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux

## Introduction

This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Certificate Samples running on Linux.

Samples are designed to highlight the function calls required to connect with the Azure IoT Hub. These calls illustrate the happy path of the [mqtt state machine](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md). As a result, **these samples are NOT designed to be used as production-level code**. Production code needs to incorporate other elements, such as connection retries and more extensive error-handling, which these samples do not include. These samples also utilize OpenSSL, which is **NOT recommended to use in production-level code on Windows or macOS**.

For Linux, the examples are tailored to Debian/Ubuntu environments. While Linux devices are not likely to be considered constrained, these samples enable one to test the Azure SDK for Embedded C libraries, even without a real device.

### WARNING: Samples are generic and should not be used in any production-level code.

### Prerequisites

- Have an [Azure account](https://azure.microsoft.com/en-us/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
- Have [PowerShell Core](https://github.com/PowerShell/PowerShell/tree/v7.0.3#get-powershell) installed. This is required to run the certificate generation script `generate_certificate.ps1`.
- <detail><summary>Have `make` and `gcc` installed. Have tools and `libssl-dev` installed:</summary>
<p>

    ```bash
    sudo apt-get update
    sudo apt-get install build-essential
    sudo apt-get install curl unzip tar pkg-config
    sudo apt-get install libssl-dev
    ```

    Note: `libssl-dev` must be installed as a prerequisite to installing CMake.

</p>
</detail>

- Have [Git](https://git-scm.com/download/linux) for Linux installed:

    ```bash
    sudo apt-get install git
    ```

- Have the latest version of [CMake](https://cmake.org/download) installed.

  Purge any apt-get installed cmake:

    ```shell
    sudo apt-get purge cmake
    ```

  Once you have downloaded the most recent tar.gz cmake file, untar it and install:

    ```shell
    sudo tar -xvzf cmake-3.18.2.tar.gz
    cd cmake-3.18.2/
    sudo ./bootstrap && make && sudo make install
    ```

  Confirm the correct version is installed:

    ```shell
    cmake-3.18.2$ cmake --version
    cmake version 3.18.2

    CMake suite maintained and supported by Kitware (kitware.com/cmake).
    ```

### What is Covered

- Setup instructions for the Azure SDK for Embedded C suite.
- Configuration, build, and run instructions for the IoT Hub Client Certificate Samples.

_The following was run on an Ubuntu Desktop 18.04 environment, but it also works on WSL 1 and 2 (Windows Subsystem for Linux)._

## Azure SDK for Embedded C Setup Instructions

1. Install Paho using vcpkg.

    The Azure IoT SDK for C uses Eclipse Paho for C installed via [vcpkg](https://github.com/Microsoft/vcpkg) (for the CMake integration).  This installation may take an extended amount of time.

    ```shell
    git clone https://github.com/Microsoft/vcpkg
    cd vcpkg/
    ./bootstrap-vcpkg.sh
    ./vcpkg install --triplet x64-linux curl cmocka paho-mqtt
    ```

2.  Set the vcpkg environment variables.

    Confirm `VCPKG_ROOT` is the path where vcpkg was cloned.

    ```shell
    export VCPKG_DEFAULT_TRIPLET=x64-linux
    export VCPKG_ROOT=PATH_TO_VCPKG #replace PATH_TO_VCPKG with full path to vcpkg/
    ```

   NOTE: Setting an environment variable only applies to the current session.  If you open a new window, the variable will have to be reset.

3. Clone the Azure Embedded SDK for C.

    ```shell
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

    ```shell
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

    NOTE: Do not copy this filepath.  Please use the output generated from running the script on your system.

    ```shell
    export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=/azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    ```

3.  Save the certificate Fingerprint from the from the `generate_certificate.ps1` script output.

    In this example, it is "4086E30F95586BF6AC7C37C6C71D94F7167394D6".
    It will be used to create the logical device on your Azure IoT Hub.

4. Create a logical device

    In your Azure IoT Hub, add a new device using a self-signed certificate.  See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**DO NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**.

    For the Thumbprint, use the recently generated fingerprint noted at the bottom of the `generate_certificate.ps1` output. (It is also placed in a file named `fingerprint.txt` for your convenience).

5. Set the remaining environment variables needed for the samples.

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Copy the Hostname from the Overview tab in your Azure IoT Hub. (In this example it is "myiothub.azure-devices.net".)
    - Set the associated environment variable:

    ```shell
    export AZ_IOT_HUB_HOSTNAME=`myiothub.azure-devices.net`
    ```

    - Select your device from the IoT Devices page and copy its Device Id. (In this example it is "testdevice-x509".)
    - Set the associated environment variable:

    ```shell
    export AZ_IOT_HUB_DEVICE_ID=`testdevice-x509`
    ```

6. Build the Azure SDK for Embedded Cc

    From the root of the cloned repository `azure-sdk-for-c/`:

    ```shell
    mkdir build
    cd build
    cmake -DTRANSPORT_PAHO=ON ..
    make
    ```

7. Run the samples.

    This is a similar list of files you should see in your computer:

    ```shell
    /azure-sdk-for-c/cmake$ cd sdk/samples/iot/
    /azure-sdk-for-c/cmake/sdk/samples/iot/$ ll
    total 248
    drwxrwxrwx 1 root root  4096 Sep 15 06:12 ./
    drwxrwxrwx 1 root root  4096 Jul 24 14:00 ../
    -rwxrwxrwx 1 root root  4188 Aug 29 00:38 CMakeLists.txt*
    -rwxrwxrwx 1 root root 30779 Sep 15 05:38 README.md*
    drwxrwxrwx 1 root root  4096 Aug 31 14:44 aziot_esp8266/
    -rwxrwxrwx 1 root root   783 Sep 15 05:58 device_cert_store.pem*
    -rwxrwxrwx 1 root root   481 Sep 15 05:58 device_ec_cert.pem*
    -rwxrwxrwx 1 root root   302 Sep 15 05:58 device_ec_key.pem*
    drwxrwxrwx 1 root root  4096 Aug 17 12:07 docs/
    -rwxrwxrwx 1 root root    58 Sep 15 05:58 fingerprint.txt*
    -rwxrwxrwx 1 root root  3145 Sep 15 00:51 generate_certificate.ps1*
    -rwxrwxrwx 1 root root 16633 Sep 14 16:17 iot_sample_common.c*
    -rwxrwxrwx 1 root root  8852 Sep 14 16:17 iot_sample_common.h*
    -rwxrwxrwx 1 root root  9060 Sep 14 16:17 paho_iot_hub_c2d_sample.c*
    -rwxrwxrwx 1 root root 11881 Sep 14 16:17 paho_iot_hub_methods_sample.c*
    -rwxrwxrwx 1 root root 43965 Sep 14 16:17 paho_iot_hub_pnp_component_sample.c*
    -rwxrwxrwx 1 root root 35923 Sep 14 16:17 paho_iot_hub_pnp_sample.c*
    -rwxrwxrwx 1 root root  8656 Sep 14 16:17 paho_iot_hub_sas_telemetry_sample.c*
    -rwxrwxrwx 1 root root  6942 Sep 14 16:17 paho_iot_hub_telemetry_sample.c*
    -rwxrwxrwx 1 root root 17908 Sep 14 16:17 paho_iot_hub_twin_sample.c*
    -rwxrwxrwx 1 root root 14211 Sep 14 16:17 paho_iot_provisioning_sample.c*
    -rwxrwxrwx 1 root root 15974 Sep 14 16:17 paho_iot_provisioning_sas_sample.c*
    drwxrwxrwx 1 root root  4096 Sep 14 16:17 pnp/
    -rwxrwxrwx 1 root root   226 Aug  8 17:12 x509_config.cfg*
    ```

    ### Telemetry (device-to-cloud messages)

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/$ ./paho_iot_hub_telemetry_sample
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID = testdevice-x509
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = /azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH =

    SUCCESS:        MQTT endpoint created at "ssl://myiothub.azure-devices.net:8883".
    SUCCESS:        Client created and configured.
    SUCCESS:        Client connected to IoT Hub.
                    Sending Message 1
                    Sending Message 2
                    Sending Message 3
                    Sending Message 4
                    Sending Message 5
    SUCCESS:        Client sent telemetry messages to IoT Hub.
    SUCCESS:        Client disconnected from IoT Hub.
    /azure-sdk-for-c/cmake/sdk/samples/iot/$
    ```

    ### Cloud-to-Device (c2d) messages

    This sample requires two actions:
    - connecting the Azure IoT SDK device client to the hub, and
    - sending a c2d message through the Azure Portal.

    First, run the sample:

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/$ ./paho_iot_hub_c2d_sample
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID = testdevice-x509
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = /azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH =

    SUCCESS:        MQTT endpoint created at "ssl://myiothub.azure-devices.net:8883".
    SUCCESS:        Client created and configured.
    SUCCESS:        Client connected to IoT Hub.
    SUCCESS:        Client subscribed to IoT Hub topics.
                    Waiting for message.
    ```

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 6).
      ![Device page](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_01_device.png)
    - Click on "Message to Device".
    - On "Message Body", type "Hello world!" (too cheesy? how about "Lorem Ipsum"?)
      ![Send message](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_02_c2d.png)
    - Click on "Send Message".
      ![Success](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_03_c2d_success.png)

    Back to the shell, verify that the message has been received by the sample:

    ```shell
    SUCCESS:        Message #1: Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: devices/testdevice-x509/messages/devicebound/%24.to=%2Fdevices%2Ftestdevice-x509%2Fmessages%2FdeviceBound
                    Payload: Hello world!
    SUCCESS:        Client parsed message.

                    Waiting for message.
    ```

    Note: The sample will terminate after 5 messages have been sent or there is a timeout.

    ### Direct Methods

    This sample requires two actions: connecting the Azure IoT SDK device client to the hub, and triggering the direct method through the Azure Portal.

    First, run the sample:

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/$ ./paho_iot_hub_methods_sample
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID = testdevice-x509
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = /azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH =

    SUCCESS:        MQTT endpoint created at "ssl://myiothub.azure-devices.net:8883".
    SUCCESS:        Client created and configured.
    SUCCESS:        Client connected to IoT Hub.
    SUCCESS:        Client subscribed to IoT Hub topics.
                    Waiting for message.
    ```

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 6).
    - Click on "Direct Method".
      ![Methods](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_04_methods.png)
    - On "Method Name", type "ping" (the sample expects the name "ping").
    - On "Payload", type '{ "somevalue": 1234 }' (the payload can be empty, but MUST be a valid Json).
    - Click on "Invoke Method".
    - See the reply from the sample on "Result" (bottom of the page).
      ![Response](../../../../docs/iot/resources/embc_samples_05_methods_response.png)

    Back to the shell, verify that the message has been received by the sample:

    ```shell
    SUCCESS:        Message #1: Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: $iothub/methods/POST/ping/?$rid=1
                    Payload: null
    SUCCESS:        Client parsed message.
                    PING!
    SUCCESS:        Client invoked ping method.
    SUCCESS:        Client published method response:
                    Status: 200
                    Payload: {"response": "pong"}

                    Waiting for message.
    ```

    Note: The sample will terminate after 5 messages have been sent or there is a timeout.

    ### Device Twin

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/$ ./paho_iot_hub_twin_sample
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID = testdevice-x509
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = /azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH =

    SUCCESS:        MQTT endpoint created at "ssl://myiothub.azure-devices.net:8883".
    SUCCESS:        Client created and configured.
    SUCCESS:        Client connected to IoT Hub.
    SUCCESS:        Client subscribed to IoT Hub topics.
                    Client requesting twin document from service.
                    Waiting for message.
    SUCCESS:        Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: $iothub/twin/res/200/?$rid=get_twin
                    Payload: {"desired":{"$version":1},"reported":{"$version":1}}
                    Status: 200
                    Type: GET
    SUCCESS:        Client parsed message.

    SUCCESS:        Client got twin document.
                    Client sending reported property to service.
    SUCCESS:        Client sent reported property message:
                    Payload: {"device_count":0}
                    Waiting for message.
    SUCCESS:        Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: $iothub/twin/res/204/?$rid=reported_prop&$version=2
                    Payload:
                    Status: 204
                    Type: Reported Properties
    SUCCESS:        Client parsed message.

    SUCCESS:        Client sent reported property.
                    Waiting for message.
    ```

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 6).
    - Click on "Device Twin".
    - Update the desired properties section of the JSON to include `device_count` and a value:

    ```json
    "properties": {
        "desired": {
            "device_count": 42,
        }
    }
    ```

    - Click on "Save".
    - See the reply from the sample on "Result" (bottom of the page).

    Back to the shell, verify that the message has been received by the sample:

    ```shell
    SUCCESS:        Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: $iothub/twin/PATCH/properties/desired/?$version=2
                    Payload: {"device_count":42,"$version":2}
                    Status: 200
                    Type: Desired Properties
    SUCCESS:        Client updated "device_count" locally to 42.
    SUCCESS:        Client parsed message.

                    Client sending reported property to service.
    SUCCESS:        Client sent reported property message:
                    Payload: {"device_count":42}
                    Waiting for message.
    SUCCESS:        Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: $iothub/twin/res/204/?$rid=reported_prop&$version=3
                    Payload:
                    Status: 204
                    Type: Reported Properties
    SUCCESS:        Client parsed message.

                    Waiting for message.
    ```

    Note: The sample will terminate after 5 twin device updates have been sent or there is a timeout.

## Need Help?

- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE
[azure_sdk_for_c_contributing_developer_guide]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#developer-guide
[azure_sdk_for_c_contributing_pull_requests]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#pull-requests
[azure_cli]: https://docs.microsoft.com/cli/azure
[azure_pattern_circuit_breaker]: https://docs.microsoft.com/azure/architecture/patterns/circuit-breaker
[azure_pattern_retry]: https://docs.microsoft.com/azure/architecture/patterns/retry
[azure_portal]: https://portal.azure.com
[azure_sub]: https://azure.microsoft.com/free/
[c_compiler]: https://visualstudio.microsoft.com/vs/features/cplusplus/
[cloud_shell]: https://docs.microsoft.com/azure/cloud-shell/overview
[cloud_shell_bash]: https://shell.azure.com/bash

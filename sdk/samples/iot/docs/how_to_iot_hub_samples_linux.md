# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux

This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Samples running.

Prerequisites:

- [Having created an Azure account](https://azure.microsoft.com/en-us/)
- [Having created an Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal)

What is covered:

- Downloading and building the Azure SDK for Embedded C suite
- Configuring and running the IoT Hub client samples.

    _The following was run on an Ubuntu Desktop 18.04 environment, but it also works on WSL 1 and 2 (Windows Subsystem for Linux)_

1. Install library dependencies

    ```shell
    sudo apt-get install unzip git build-essential pkg-config libssl-dev
    ```

2. Install the latest cmake

    Check the latest version available on https://cmake.org/download/.

    Currently, the latest version of cmake is 3.17.2.

    ```shell
    $ sudo apt-get purge cmake
    $ wget https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2.tar.gz
    $ tar -xvzf cmake-3.17.2.tar.gz
    $ cd cmake-3.17.2/
    /cmake-3.17.2$ ./bootstrap
    /cmake-3.17.2$ sudo make install
    ```

    Now we need to check if Cmake was correctly installed:

    ```shell
    cmake-3.17.2$ cmake --version
    cmake version 3.17.2

    CMake suite maintained and supported by Kitware (kitware.com/cmake).
    /cmake-3.17.2$
    ```

3. Install Paho using vcpkg

    The Azure IoT SDK for C uses Eclipse Paho for C installed via vcpkg (for the cmake integration).

    ```shell
    $ cd
    $ git clone https://github.com/Microsoft/vcpkg
    $ cd vcpkg/
    /vcpkg$ ./bootstrap-vcpkg.sh
    /vcpkg$ ./vcpkg install paho-mqtt
    ...
    Total elapsed time: 2.835 min

    The package paho-mqtt:x64-linux provides CMake targets:

        find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
        target_link_libraries(main PRIVATE eclipse-paho-mqtt-c::paho-mqtt3as-static eclipse-paho-mqtt-c::paho-mqtt3cs-static)

    /vcpkg$
    /vcpkg$ ./vcpkg integrate install
    Applied user-wide integration for this vcpkg root.

    CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake"
    ```

4. Clone the Azure SDK for C

    ```shell
    $ cd
    $ git clone https://github.com/azure/azure-sdk-for-c
    ```

5. Generate a self-signed certificate

    The Azure Embedded SDK for C IoT Client samples use a self-signed certificate.

    A script is provided for creating that certificate.

    ```shell
    $ cd azure-sdk-for-c/sdk/samples/iot
    azure-sdk-for-c/sdk/samples/iot$ ./generate_certificate.sh
    ```

    <details allowed_elements>
    <summary>
    Complete output of the `generate_certificate.sh` script.
    </summary>

    ```shell
    azure-sdk-for-c/sdk/samples/iot$ ./generate_certificate.sh
    Certificate:
        Data:
            Version: 1 (0x0)
            Serial Number:
                5d:29:5b:fd:d7:6c:e0:a0:c3:a5:a9:ee:4d:92:4c:56:fc:bd:4e:f5
            Signature Algorithm: ecdsa-with-SHA256
            Issuer: CN = paho-sample-device1
            Validity
                Not Before: Aug  8 21:07:29 2020 GMT
                Not After : Aug  8 21:07:29 2021 GMT
            Subject: CN = paho-sample-device1
            Subject Public Key Info:
                Public Key Algorithm: id-ecPublicKey
                    Public-Key: (256 bit)
                    pub:
                        04:6d:fd:c8:5d:d4:7e:99:b0:44:81:bb:66:20:52:
                        d6:22:b6:32:81:91:5d:e3:99:8f:93:bb:35:c8:ac:
                        47:48:d7:76:89:7b:02:f4:00:5a:1f:8d:e9:62:5b:
                        a7:b0:12:a9:b8:51:ca:24:0d:72:17:81:f8:9f:ae:
                        09:26:68:c6:36
                    ASN1 OID: prime256v1
                    NIST CURVE: P-256
        Signature Algorithm: ecdsa-with-SHA256
            30:45:02:20:15:4f:49:cd:ff:08:3e:0e:7d:5d:8f:a6:3a:b3:
            87:62:01:b2:ad:17:6a:bc:cf:b0:d6:2d:e1:85:25:d6:65:0e:
            02:21:00:8a:2e:e5:d4:33:8e:91:9d:71:b1:ee:78:cf:c5:ff:
            06:7e:92:9c:66:71:38:95:06:82:82:8c:5a:7c:90:a3:9d

    IMPORTANT:
    It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:
    Microsoft Windows SChannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel
    OR
    Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport
    If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system.

    SAMPLE CERTIFICATE GENERATED:
    Use the following command to set the environment variable for the samples:

            export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=/azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem

    DPS SAMPLE:
    Upload device_ec_cert.pem when enrolling your device with the Device Provisioning Service.

    IOT HUB SAMPLES:
    Use the following fingerprint when creating your device in IoT Hub.
    (The fingerprint has also been placed in fingerprint.txt for future reference.)

            SHA1 Fingerprint=D0A4AB23D52BB6FE5EF06FC0718D6F3743F00364

    /azure-sdk-for-c/sdk/samples/iot$
    ```

    </details>

    Do not forget to set the environment variable above, making sure it maps to your local path.

    ```shell
    export AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=/azure-sdk-for-c/sdk/samples/iot/device_cert_store.pem
    ```

    Save the certificate Fingerprint above (in this example, `C8DC8780C64FFBB5A66D8BC5D39D3C1BBB03FB69`).
    It will be used to create the logical device on your Azure IoT Hub.

6. Create a logical device

    - Log into your Azure account on [Azure Portal](https://portal.azure.com)
    - On the menu on the left, click on "IoT devices" under "Explorers".
    - Click on "New".
    - Type an unique ID for your device.
    - Select "X.509 Self-Signed" for "Authentication type".
    - Type the fingerprint obtained in the previous step (the same can be used for primary and secondary Thumbprints; no colons!).
    - Click on "Save".

7. Collect information about Azure IoT Hub and device

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Get the Azure IoT Hub FQDN.
        - On your Azure IoT Hub page, click on "Overview".
        - Copy and save the "Hostname" value (in this example, "myiothub.azure-devices.net").
    - Get the device ID.
      - On your Azure IoT Hub page, click on "IoT devices" under "Explorers".
      - On the list of devices, click on the device created on the previous step (in this example, "testdevice-x509").
      - Copy and save the "Device ID" value (in this example, "testdevice-x509").

8. Build the Azure SDK for C

    Back into the folder where the Azure SDK for C was cloned...

    ```shell
    /cd ~/azure-sdk-for-c/
    /azure-sdk-for-c$ mkdir cmake
    /azure-sdk-for-c$ cd cmake
    /azure-sdk-for-c/cmake$ cmake -DTRANSPORT_PAHO=ON -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake ..
    /azure-sdk-for-c/cmake$ make -j
    ```

9. Set the environment variables needed for the samples

    According the the [readme documentation](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/iot) for the Azure Embedded SDK for C IoT Hub client certificate samples require the following environment variables.

    ```shell
    export AZ_IOT_HUB_DEVICE_ID=<device ID obtained on step 7>
    export AZ_IOT_HUB_HOSTNAME=<FQDN obtained on step 7>
    ```

    `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH` is not required for all platforms (like Linux).

    `AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH` has already been set on step 5.

    Using the values from the example above, the export command would look like this (don't run these command lines, you should use your own device ID and hostname)

    ```shell
    export AZ_IOT_HUB_DEVICE_ID=testdevice-x509
    export AZ_IOT_HUB_HOSTNAME=myiothub.azure-devices.net
    ```

10. Run the samples.

    This is a similar list of files you should see in your computer:

    ```shell
    /azure-sdk-for-c/cmake$ cd sdk/samples/iot/
    /azure-sdk-for-c/cmake/sdk/samples/iot/$ ll
    total 34544
    drwxrwxrwx 1 user user    4096 Aug  8 14:19 ./
    drwxrwxrwx 1 user user    4096 Aug  8 14:19 ../
    drwxrwxrwx 1 user user    4096 Aug  8 14:19 CMakeFiles/
    -rwxrwxrwx 1 user user     321 Aug  8 14:19 CTestTestfile.cmake*
    -rwxrwxrwx 1 user user   33792 Aug  8 14:19 Makefile*
    -rwxrwxrwx 1 user user    1265 Aug  8 14:19 cmake_install.cmake*
    -rwxrwxrwx 1 user user   19044 Aug  8 14:19 libaz_iot_samples_common.a*
    -rwxrwxrwx 1 user user 3901712 Aug  8 14:19 paho_iot_hub_c2d_sample*
    -rwxrwxrwx 1 user user 3906368 Aug  8 14:19 paho_iot_hub_methods_sample*
    -rwxrwxrwx 1 user user 3956192 Aug  8 14:19 paho_iot_hub_pnp_component_sample*
    -rwxrwxrwx 1 user user 3940352 Aug  8 14:19 paho_iot_hub_pnp_sample*
    -rwxrwxrwx 1 user user 3906416 Aug  8 14:19 paho_iot_hub_sas_telemetry_sample*
    -rwxrwxrwx 1 user user 3901688 Aug  8 14:19 paho_iot_hub_telemetry_sample*
    -rwxrwxrwx 1 user user 3939000 Aug  8 14:19 paho_iot_hub_twin_sample*
    -rwxrwxrwx 1 user user 3923656 Aug  8 14:19 paho_iot_provisioning_sample*
    -rwxrwxrwx 1 user user 3928584 Aug  8 14:19 paho_iot_provisioning_sas_sample*
    /azure-sdk-for-c/cmake/sdk/samples/iot/$
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

    Note: the sample does not terminate automatically. The sample will terminate after 5 messages have been sent or there is a timeout.

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

    Note: the sample does not terminate automatically. The sample will terminate after 5 messages have been sent or there is a timeout.

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
            ...
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

    Note: the sample does not terminate automatically. The sample will terminate after 5 twin device updates have been sent or there is a timeout.

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

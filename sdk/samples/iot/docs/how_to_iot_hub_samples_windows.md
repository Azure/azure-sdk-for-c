# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows

This is a step-by-step documentation of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Samples running on Microsoft Windows.

Prerequisites:

- [Having created an Azure account](https://azure.microsoft.com/en-us/)
- [Having created an Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal)
- Microsoft Visual Studio installed in the local machine.

What is covered:

- Downloading and building the Azure SDK for Embedded C suite
- Configuring and running the IoT Hub client samples.

    _The following was run on Microsoft Windows 10.0.18363.836._

1. Install git

    Get the installer from the official git [page](https://git-scm.com/download/win)

2. Install the latest CMake

    Check the latest version available on https://cmake.org/download/.

    After installing, check if cmake works correctly:

    ```shell
    C:\>cmake --version
    cmake version 3.15.19101501-MSVC_2

    CMake suite maintained and supported by Kitware (kitware.com/cmake).

    C:\>
    ```

3. Install Paho using vcpkg

    The Azure IoT SDK for C uses Eclipse Paho installed via [vcpkg](https://github.com/Microsoft/vcpkg) (for the cmake integration).

    ```shell
    C:\> git clone https://github.com/Microsoft/vcpkg
    C:\> cd vcpkg/
    C:\vcpkg> bootstrap-vcpkg.bat
    C:\vcpkg> vcpkg install --triplet x64-windows-static curl[winssl] cmocka paho-mqtt
    C:\vcpkg> vcpkg integrate install
    ...
    C:\vcpkg> set VCPKG_DEFAULT_TRIPLET=x64-windows-static
    C:\vcpkg> set "VCPKG_ROOT=C:\vcpkg"
    ```

    > Make sure `VCPKG_ROOT` has the path where vcpkg was cloned.

4. Add openssl to the PATH environment variable

    OpenSSL will be used for generating self-signed certificates.
    It gets installed by vcpkg as a dependency for Eclipse Paho.

    Use the commands below to find the path to the openssl.exe tool, then set it in the PATH variable.

    ```shell
    C:\vcpkg>where /r . openssl.exe
    ...
    C:\vcpkg\installed\x86-windows\tools\openssl\openssl.exe
    ...
    C:\vcpkg>set "PATH=%PATH%;C:\vcpkg\installed\x86-windows\tools\openssl"
    ```

    > Note: This applies only the current command window being used. If you open a new one, this step must be done again.

5. Clone the Azure Embedded SDK for C

    ```shell
    C:\>cd..
    C:\>git clone https://github.com/azure/azure-sdk-for-c
    ```

6. Generate a self-signed certificate

    The Azure Embedded SDK for C IoT Client samples use a self-signed certificate.

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot>generate_certificate.cmd
    ```

    <details>
    <summary>
    Expand to see the complete output of the `generate_certificate.cmd` script.
    </summary>

    ```shell
    Certificate:
        Data:
            Version: 1 (0x0)
            Serial Number:
                1a:d0:e8:7c:2f:d4:0d:41:50:2e:3e:e6:ad:9e:df:d7:13:f1:24:c8
            Signature Algorithm: ecdsa-with-SHA256
            Issuer: CN = paho-sample-device1
            Validity
                Not Before: Aug 12 22:20:24 2020 GMT
                Not After : Aug 12 22:20:24 2021 GMT
            Subject: CN = paho-sample-device1
            Subject Public Key Info:
                Public Key Algorithm: id-ecPublicKey
                    Public-Key: (256 bit)
                    pub:
                        04:ff:15:bc:c6:dd:88:86:68:d0:31:3d:dd:b5:3f:
                        12:25:0c:b1:6a:bd:16:74:c5:ca:16:9f:81:ff:59:
                        4c:02:48:d2:e3:98:e5:79:77:3d:82:bb:f2:f9:85:
                        c2:20:90:59:74:c5:80:6a:9a:9f:cd:37:22:ed:bd:
                        66:29:b2:30:05
                    ASN1 OID: prime256v1
                    NIST CURVE: P-256
        Signature Algorithm: ecdsa-with-SHA256
            30:46:02:21:00:b3:20:76:d6:c2:0c:a5:0b:a7:ba:df:fd:4e:
            ac:c1:f3:eb:f6:1e:7c:dd:05:f8:dd:1f:d7:08:ec:9a:89:1a:
            85:02:21:00:aa:fb:77:09:6f:1f:df:c3:91:37:2d:ba:74:54:
            ac:48:a3:89:10:65:e5:c2:ac:05:68:fc:8e:9a:43:1d:1f:bc

    IMPORTANT:
    It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:
    Microsoft Windows SChannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel
    OR
    Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport
    If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system.

    SAMPLE CERTIFICATE GENERATED:
    Use the following command to set the environment variable for the samples:

            set AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=C:\Repos\momuno_azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem

    DPS SAMPLE:
    Upload device_ec_cert.pem when enrolling your device with the Device Provisioning Service.

    IOT HUB SAMPLES:
    Use the following fingerprint when creating your device in IoT Hub.
    (The fingerprint has also been placed in fingerprint.txt for future reference.)

            SHA1 Fingerprint=8669299A4E2217B9BE5415FFA6647BA4D711A41F
    ```

    </details>

    **Important**: Set the environment variable, as per the output shown above (make sure it maps to your local path as shown).

    The example below shows what you should execute (but don't use these examples, use the ones you got on your output)

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot>set "AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem"
    ```

    Save the certificate Fingerprint above (in this example, "53748606517027078B5FE00E7A0D2A31F9AF4C31").
    It will be used to create the logical device on your Azure IoT Hub.

    > Also set the trusted pem file environment variable:

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot>set "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH=C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem"
    ```

    Download the Baltimore PEM CA from https://www.digicert.com/digicert-root-certificates.htm into the same place as the filepath in the trusted pem file path environment variable.

    You should have it saved as shown bellow:

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot>dir BaltimoreCyberTrustRoot.crt.pem
    Volume in drive C is OSDisk
    Volume Serial Number is AAAA-BBBB

    Directory of C:\azure-sdk-for-c\sdk\samples\iot

    06/03/2020  03:33 PM             1,262 BaltimoreCyberTrustRoot.crt.pem
                1 File(s)          1,262 bytes
    ...
    ```

7. Create a logical device

    - Log into your Azure account on [Azure Portal](https://portal.azure.com)
    - On the menu on the left, click on "IoT devices" under "Explorers".
    - Click on "New".
    - Type an unique ID for your device.
    - Select "X.509 Self-Signed" for "Authentication type".
    - Type the fingerprint obtained in the previous step (in this case, the same should be used for primary and secondary Thumbprints).
    - Click on "Save".

8. Collect information about Azure IoT Hub and device

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Get the Azure IoT Hub FQDN.
        - On your Azure IoT Hub page, click on "Overview".
        - Copy and save the "Hostname" value (in this example, "myiothub.azure-devices.net").
    - Get the device ID.
    - On your Azure IoT Hub page, click on "IoT devices" under "Explorers".
    - On the list of devices, click on the device created on the previous step (in this example, "paho-sample-device1").
    - Copy and save the "Device ID" value (in this example, "paho-sample-device1").

9. Set the environment variables needed for the samples

    According the the [readme documentation](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/iot) for the Azure Embedded SDK for C IoT Hub client certificate samples require the following environment variables.

    ```shell
    set AZ_IOT_HUB_DEVICE_ID=<device ID obtained on step 8>
    set AZ_IOT_HUB_HOSTNAME=<FQDN obtained on step 8>
    ```

    `AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH` and `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH` have already been set on step 6.

    Using the values from the example above, the export command would look like this (don't run these command lines, you should use your own device ID and hostname)

    ```shell
    C:\azure-sdk-for-c>set AZ_IOT_HUB_DEVICE_ID=paho-sample-device1
    C:\azure-sdk-for-c>set AZ_IOT_HUB_HOSTNAME=myiothub.azure-devices.net
    ```

10. Create and open the solution for the Azure Embedded SDK for C

    Back into the folder where the Azure SDK for C was cloned...

    ```shell
    C:\azure-sdk-for-c>mkdir cmake
    C:\azure-sdk-for-c>cd cmake
    C:\azure-sdk-for-c\cmake>cmake -DTRANSPORT_PAHO=ON ..
    ...
    C:\azure-sdk-for-c\cmake>az.sln
    ```

11. Build and run the samples.

    ### Telemetry (device-to-cloud messages)

    On the `az.sln` solution open on Visual Studio,
    - Navigate on the Solution Explorer panel to `paho_iot_hub_telemetry_sample` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_telemetry_sample` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    The following traces shall be seen on the Visual Studio Command prompt:

    ```shell
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID =paho-sample-device1
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem

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
    ```

    ### Cloud-to-Device (c2d) messages

    This sample requires two actions:
    - connecting the Azure IoT SDK device client to the hub, and
    - sending a c2d message through the Azure Portal.

    First, run the sample:

    On the `az.sln` solution open on Visual Studio,
    - Navigate on the Solution Explorer panel to `paho_iot_hub_c2d_sample` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_c2d_sample` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 7).
    - Click on "Message to Device".
    - On "Message Body", type "Hello world!" (too cheesy? how about "Lorem Ipsum"?)
    - Click on "Send Message".

    Back to the Visual Studio Command prompt, verify that the message has been received by the sample:

    ```shell
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID =paho-sample-device1
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem

    SUCCESS:        MQTT endpoint created at "ssl://myiothub.azure-devices.net:8883".
    SUCCESS:        Client created and configured.
    SUCCESS:        Client connected to IoT Hub.
    SUCCESS:        Client subscribed to IoT Hub topics.
                    Waiting for message.
    SUCCESS:        Message #1: Client received message from the service.
    SUCCESS:        Client received a valid topic response:
                    Topic: devices/testdevice-x509/messages/devicebound/%24.to=%2Fdevices%2Ftestdevice-x509%2Fmessages%2FdeviceBound
                    Payload: Hello world!
    SUCCESS:        Client parsed message.

                    Waiting for message.
    ```

    Note: The sample does not terminate automatically. The sample will terminate after 5 messages have been sent or there is a timeout.

    ### Direct Methods

    This sample requires two actions:
    - connecting the Azure IoT SDK device client to the hub, and
    - triggering the direct method through the Azure Portal.

    First, run the sample:

    On the `az.sln` solution open on Visual Studio,
    - Navigate on the Solution Explorer panel to `paho_iot_hub_methods_sample` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_methods_sample` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 7).
    - Click on "Direct Method".
    - On "Method Name", type `ping` (the sample expects the name `ping`).
    - Click on "Invoke Method".
    - See the reply from the sample on "Result" (bottom of the page).

    Back to the Visual Studio Command prompt, verify that the direct method has been received by the sample:

    ```shell
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID =paho-sample-device1
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem

    SUCCESS:        MQTT endpoint created at "ssl://myiothub.azure-devices.net:8883".
    SUCCESS:        Client created and configured.
    SUCCESS:        Client connected to IoT Hub.
    SUCCESS:        Client subscribed to IoT Hub topics.
                    Waiting for message.
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

    Note: The sample does not terminate automatically. The sample will terminate after 5 messages have been sent or there is a timeout.

    ### Device Twin

    On the `az.sln` solution open on Visual Studio,
    - Navigate on the Solution Explorer panel to `paho_iot_hub_twin_sample` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_twin_sample` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

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

    Back to the Visual Studio Command prompt, verify that the device twin update has been received by the sample:

    ```shell
    AZ_IOT_HUB_HOSTNAME = myiothub.azure-devices.net
    AZ_IOT_HUB_DEVICE_ID =paho-sample-device1
    AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem
    AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH = C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem

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
     Note: The sample does not terminate automatically. The sample will terminate after 5 twin device updates have been sent or there is a timeout.

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

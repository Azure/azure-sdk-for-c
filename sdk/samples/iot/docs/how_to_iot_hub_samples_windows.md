# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows

## Introduction

This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Certificate Samples running on Microsoft Windows.

### WARNING: Samples are generic and should not be used in any production-level code.

### Prerequisites

- Have an [Azure account](https://azure.microsoft.com/en-us/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) created.
- Have [PowerShell Core](https://github.com/PowerShell/PowerShell/tree/v7.0.3#get-powershell) installed. This is required to run the certificate generation script `generate_certificate.ps1`.
- Have [Microsoft Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) installed with [C and C++ support](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2019).
- Have [Git](https://git-scm.com/download/win) for Windows installed.
- Have the latest version of [CMake](https://cmake.org/download) installed.

### What is Covered

- Setup instructions for the Azure SDK for Embedded C suite.
- Configuration, build, and run instructions for the IoT Hub Client Certificate Samples.

    _The following was run on Microsoft Windows 10.0.18363.836._

## Azure SDK for Embedded C Setup Instructions

1. Install Paho using vcpkg.

    The Azure IoT SDK for C uses Eclipse Paho installed via [vcpkg](https://github.com/Microsoft/vcpkg) (for the CMake integration).  This installation may take an extended amount of time.

    ```powershell
    C:\> git clone https://github.com/Microsoft/vcpkg
    C:\> cd .\vcpkg\
    C:\vcpkg> .\bootstrap-vcpkg.bat
    C:\vcpkg> .\vcpkg.exe install --triplet x64-windows-static curl[winssl] cmocka paho-mqtt
    ```

2. Set the vcpkg environment variables.

    Confirm `VCPKG_ROOT` is the path where vcpkg was cloned.

    ```powershell
    C:\vcpkg> $env:VCPKG_DEFAULT_TRIPLET='x64-windows-static'
    C:\vcpkg> $env:VCPKG_ROOT='C:\vcpkg'
    ```

    NOTE: Setting an environment variable only applies to the current session.  If you open a new window, the variable will have to be reset.

3. Add OpenSSL to the PATH environment variable.

    **WARNING: It is NOT recommended to use OpenSSL in production-level code on Windows or macOS.**

    OpenSSL will be used for generating self-signed certificates. It is installed by vcpkg as a dependency for Eclipse Paho.

    Use the commands below to find the path to the openssl.exe tool, then set it in the PATH variable.

    ```powershell
    C:\vcpkg> where.exe /r . openssl.exe
    ...
    C:\vcpkg\installed\x64-windows-static\tools\openssl\openssl.exe
    ...
    C:\vcpkg> $env:PATH=$env:PATH + ';C:\vcpkg\installed\x64-windows-static\tools\openssl'
    ```

4. Clone the Azure Embedded SDK for C.

    ```powershell
    C:\> cd ..
    C:\> git clone https://github.com/azure/azure-sdk-for-c
    ```

## Configure and Run the IoT Hub Client Certificate Samples

1. Generate a self-signed certificate.

    **WARNING: This script is intended for sample use only and should not be used in any production-level code.**

    The Azure Embedded SDK for C IoT Client Certificate Samples use a self-signed certificate. A script is provided for creating that certificate.

    ```powershell
    C:\azure-sdk-for-c\sdk\samples\iot> .\generate_certificate.ps1
    ```

    <details><summary>Expand to see the complete output of the `generate_certificate.ps1` script.</summary>
    <p>

    ```powershell
    WARNING: Certificates created by this script MUST NOT be used for production.
    WARNING: They expire after 365 days, and most importantly are provided for demonstration purposes to help you quickly understand CA Certificates.
    WARNING: When productizing against CA Certificates, you'll need to use your own security best practices for certification creation and lifetime management.
    Certificate:
        Data:
            Version: 1 (0x0)
            Serial Number:
                04:75:28:23:02:24:5f:61:83:38:79:1d:d2:4a:0f:a1:99:bf:b4:42
            Signature Algorithm: ecdsa-with-SHA256
            Issuer: CN = paho-sample-device1
            Validity
                Not Before: Sep 15 08:00:14 2020 GMT
                Not After : Sep 15 08:00:14 2021 GMT
            Subject: CN = paho-sample-device1
            Subject Public Key Info:
                Public Key Algorithm: id-ecPublicKey
                    Public-Key: (256 bit)
                    pub:
                        04:d5:7f:c6:2f:ef:8a:3d:3a:7f:1c:b1:b2:11:c8:
                        4c:23:b1:f6:35:7c:ef:62:b5:f4:e1:be:b4:98:1d:
                        f1:4c:bc:cd:b1:6c:46:eb:76:11:e6:37:d1:61:aa:
                        64:c7:42:c8:cc:da:ba:44:8e:0e:de:96:87:27:5f:
                        ec:23:e6:9a:1a
                    ASN1 OID: prime256v1
                    NIST CURVE: P-256
        Signature Algorithm: ecdsa-with-SHA256
            30:45:02:20:06:d9:fa:e4:35:b1:a1:db:94:ed:b0:05:a6:b6:
            c0:45:b0:2d:31:42:a5:e8:8f:45:d5:be:5e:d6:35:4e:b5:14:
            02:21:00:d5:6f:58:e3:f6:be:ad:25:76:98:71:1e:08:18:3c:
            ee:06:ba:d7:0f:68:d2:91:f2:63:33:ab:29:c1:a6:ed:6f


    WARNING: IMPORTANT:
    WARNING: It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:
    WARNING: Microsoft Windows Schannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel
    WARNING: OR
    WARNING: Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport
    WARNING: If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system.

    SAMPLE CERTIFICATE GENERATED:
    Use the following command to set the environment variable for the samples:

            $env:AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem

    DPS SAMPLE:
    Upload device_ec_cert.pem when enrolling your device with the Device Provisioning Service.

    IOT HUB SAMPLES:
    Use the following fingerprint when creating your device in IoT Hub.
    (The fingerprint has also been placed in fingerprint.txt for future reference.)
    SHA1 Fingerprint=2A1B236A3839A2D8070E9A0EE21C9E1488DDBA7E
    ```

    </p>
    </details>

2. Set the environment variable from the `generate_certificate.ps1` script output.

    NOTE: Do not copy this filepath.  Please use the output generated from running the script on your system.

    ```powershell
    C:\azure-sdk-for-c\sdk\samples\iot> $env:AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH='C:\azure-sdk-for-c\sdk\samples\iot\device_cert_store.pem'
    ```

3. Save the certificate Fingerprint from the from the `generate_certificate.ps1` script output.

    In this example, it is "2A1B236A3839A2D8070E9A0EE21C9E1488DDBA7E".
    It will be used to create the logical device on your Azure IoT Hub.

4. Download the [Baltimore PEM CA](https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem) into the directory.

    You should have it saved as shown bellow:

    ```powershell
    C:\azure-sdk-for-c\sdk\samples\iot> ls BaltimoreCyberTrustRoot.crt.pem
    Mode                 LastWriteTime         Length Name
    ----                 -------------         ------ ----
    -a---           8/12/2020  6:54 PM           1262 BaltimoreCyberTrustRoot.crt.pem
    ```

    Set the trusted pem file environment variable to match the filepath of the downloaded pem file.

    ```powershell
    C:\azure-sdk-for-c\sdk\samples\iot> $env:AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH='C:\azure-sdk-for-c\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem'
    ```

5. Create a logical device.

    In your Azure IoT Hub, add a new device using a self-signed certificate.  See [here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-security-x509-get-started#create-an-x509-device-for-your-iot-hub) for further instruction, with one exception--**DO NOT** select X.509 CA Signed as the authentication type. Select **X.509 Self-Signed**.

    For the Thumbprint, use the recently generated fingerprint noted at the bottom of the `generate_certificate.ps1` output. (It is also placed in a file named `fingerprint.txt` for your convenience).

6. Set the remaining environment variables needed for the samples.

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Copy the Hostname from the Overview tab in your Azure IoT Hub. (In this example it is "myiothub.azure-devices.net".)
    - Set the associated environment variable:

        ```powershell
        C:\azure-sdk-for-c\sdk\samples\iot> $env:AZ_IOT_HUB_HOSTNAME='myiothub.azure-devices.net'
        ```

    - Select your device from the IoT Devices page and copy its Device Id. (In this example it is "paho-sample-device1".)
    - Set the associated environment variable:

        ```powershell
        C:\azure-sdk-for-c\sdk\samples\iot> $env:AZ_IOT_HUB_DEVICE_ID='paho-sample-device1'
        ```

7. Create and open the solution for the Azure Embedded SDK for C.

    From the root of the cloned repository:

    ```powershell
    C:\azure-sdk-for-c> mkdir build
    C:\azure-sdk-for-c> cd build
    C:\azure-sdk-for-c\build> cmake -DTRANSPORT_PAHO=ON ..
    ...
    C:\azure-sdk-for-c\build> .\az.sln
    ```

8. Build and run the samples.

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

    Note: The sample will terminate after 5 messages have been sent or there is a timeout.

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

    Note: The sample will terminate after 5 messages have been sent or there is a timeout.

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
     Note: The sample will terminate after 5 twin device updates have been sent or there is a timeout.

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

## Need Help?

- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.
- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).

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

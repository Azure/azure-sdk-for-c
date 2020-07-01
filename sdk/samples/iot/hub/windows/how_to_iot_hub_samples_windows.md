# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Microsoft Windows

This is a step-by-step documentation of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Samples running on Microsoft Windows. 

Pre-requisites:
- [Having created an Azure account](https://github.com/ewertons/azure-sdk-for-c/wiki/How-to-create-an-Azure-account)
- [Having created an Azure IoT Hub](https://github.com/ewertons/azure-sdk-for-c/wiki/How-to-create-an-Azure-IoT-Hub)
- Microsoft Visual Studio installed in the local machine.

What is covered:
- Downloading and building the Azure SDK for Embedded C suite
- Configuring and running the IoT Hub client samples.

_The following was run on Microsoft Windows 10.0.18363.836._

01. Install git

    Get the installer from the official git [page](https://git-scm.com/download/win)

02. Install the latest CMake

    Check the latest version available on https://cmake.org/download/.

    After installing, check if cmake works correctly: 

    ```shell
    C:\>cmake --version
    cmake version 3.15.19101501-MSVC_2

    CMake suite maintained and supported by Kitware (kitware.com/cmake).

    C:\> 
    ```

03. Install Paho using vcpkg

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

04. Add openssl to the PATH enviroment variable

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

05. Clone the Azure Embedded SDK for C

    ```shell
    C:\>cd..
    C:\>git clone https://github.com/azure/azure-sdk-for-c
    ```

05. Generate a self-signed certificate

    The Azure Embedded SDK for C IoT Client samples use a self-signed certificate.

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot\hub\src>generate_certificate.cmd
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
                2c:bb:5f:03:cc:13:e1:99:1b:ae:d3:7d:da:0d:43:25:5e:7c:62:7a
            Signature Algorithm: ecdsa-with-SHA256
            Issuer: CN = paho-sample-device1
            Validity
                Not Before: Jun  3 22:08:04 2020 GMT
                Not After : Jun  3 22:08:04 2021 GMT
            Subject: CN = paho-sample-device1
            Subject Public Key Info:
                Public Key Algorithm: id-ecPublicKey
                    Public-Key: (256 bit)
                    pub:
                        04:e7:25:45:a6:99:cc:9c:9c:a9:ba:2b:5f:6e:6d:
                        3c:e1:cf:05:61:b3:99:6d:5b:a9:f8:92:b2:3b:ee:
                        6d:be:0b:51:33:33:2a:15:27:2c:3e:08:7d:28:fa:
                        fa:db:54:58:f5:49:e3:e7:a7:f7:0b:a2:bd:c8:ff:
                        8c:39:2a:88:4f
                    ASN1 OID: prime256v1
                    NIST CURVE: P-256
        Signature Algorithm: ecdsa-with-SHA256
            30:44:02:20:21:dc:fe:f3:6b:3f:33:15:7a:28:3c:d2:b7:57:
            ba:1b:ad:e4:0d:94:c2:0c:50:7c:88:ac:60:8f:32:e9:45:02:
            02:20:10:03:06:9c:c3:4c:c4:05:ba:00:6a:d3:fa:27:02:74:
            79:c0:2f:3a:cf:9d:99:5e:0b:1e:d7:1f:36:3c:56:96

    It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:
    Microsoft Windows SChannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel
    OR
    Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport
    If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system.

    If required (for example on Windows), download the Baltimore PEM CA from https://www.digicert.com/digicert-root-certificates.htm to the current folder.
    Once it is downloaded, run the following command to set the environment variable for the samples:

    set "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE=C:\azure-sdk-for-c\sdk\samples\iot\hub\src\BaltimoreCyberTrustRoot.crt.pem"

    Sample certificate generated
    Use the following command to set the environment variable for the samples:

    set "AZ_IOT_DEVICE_X509_CERT_PEM_FILE=C:\azure-sdk-for-c\sdk\samples\iot\hub\src\device_cert_store.pem"

    Use the following fingerprint when creating your device in IoT Hub:
    SHA1 Fingerprint=53748606517027078B5FE00E7A0D2A31F9AF4C31

    The fingerprint has also been placed in fingerprint.txt for future reference
    ```
    </details>


    **Important**: Set the environment variables, as per the output shown above (make sure it maps to your local path as shown).

    The example below shows what you should execute (but don't use these examples, use the ones you got on your output)

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot\hub\src>set "AZ_IOT_DEVICE_X509_CERT_PEM_FILE=C:\azure-sdk-for-c\sdk\samples\iot\hub\src\device_cert_store.pem"
    C:\azure-sdk-for-c\sdk\samples\iot\hub\src>set "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE=C:\azure-sdk-for-c\sdk\samples\iot\hub\src\BaltimoreCyberTrustRoot.crt.pem"
    ```

    Save the certificate Fingerprint above (in this example, "53748606517027078B5FE00E7A0D2A31F9AF4C31").
    It will be used to create the logical device on your Azure IoT Hub.

    > Follow the instruction in the output of the script and download the Baltimore PEM CA from https://www.digicert.com/digicert-root-certificates.htm into the same place mentioned by the script output.

    You should have it saved as shown bellow:

    ```shell
    C:\azure-sdk-for-c\sdk\samples\iot\hub\src>dir BaltimoreCyberTrustRoot.crt.pem
    Volume in drive C is OSDisk
    Volume Serial Number is AAAA-BBBB

    Directory of C:\azure-sdk-for-c\sdk\samples\iot\hub\src

    06/03/2020  03:33 PM             1,262 BaltimoreCyberTrustRoot.crt.pem
                1 File(s)          1,262 bytes
    ...
    ```

06. Create a logical device

    - Log into your Azure account on [Azure Portal](https://portal.azure.com)
    - On the menu on the left, click on "IoT devices" under "Explorers".
    - Click on "New".
    - Type an unique ID for your device.
    - Select "X.509 Self-Signed" for "Authentication type". 
    - Type the fingerprint obtained in the previous step (in this case, the same should be used for primary and secondary Thumbprints).
    - Click on "Save".

07. Collect information about Azure IoT Hub and device

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Get the Azure IoT Hub FQDN.
        - On your Azure IoT Hub page, click on "Overview".
        - Copy and save the "Hostname" value (in this example, "myiothub.azure-devices.net").
    - Get the device ID. 
    - On your Azure IoT Hub page, click on "IoT devices" under "Explorers".
    - On the list of devices, click on the device created on the previous step (in this example, "paho-sample-device1").
    - Copy and save the "Device ID" value (in this example, "paho-sample-device1"). 


08. Set the environment variables needed for the samples

    According the the [readme documentation](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/iot/hub) for the Azure Embedded SDK for C IoT client samples require the following environment variables.

    ```shell
    set AZ_IOT_DEVICE_ID=<device ID obtained on step 7> 
    set AZ_IOT_HUB_HOSTNAME=<FQDN obtained on step 7>
    ```

    AZ_IOT_DEVICE_X509_CERT_PEM_FILE and AZ_IOT_DEVICE_X509_TRUST_PEM_FILE have already been set on step 5.

    Using the values from the example above, the export command would look like this (don't run these command lines, you should use your own device ID and hostname)

    ```shell
    C:\azure-sdk-for-c>set AZ_IOT_DEVICE_ID=paho-sample-device1
    C:\azure-sdk-for-c>set AZ_IOT_HUB_HOSTNAME=myiothub.azure-devices.net
    ```

9.  Create and open the solution for the Azure Embedded SDK for C

    Back into the folder where the Azure SDK for C was cloned...

    ```shell
    C:\azure-sdk-for-c>mkdir cmake
    C:\azure-sdk-for-c>cd cmake
    C:\azure-sdk-for-c\cmake>cmake -DTRANSPORT_PAHO=ON ..
    ...
    C:\azure-sdk-for-c\cmake>az.sln
    ```

10. Build and run the samples.


    ### Telemetry (device-to-cloud messages)

    On the `az.sln` solution open on Visual Studio, 
    - Navigate on the Solution Explorer panel to `paho_iot_hub_telemetry_example` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_telemetry_example` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    The following traces shall be seen on the Visual Studio Command prompt:

    ```shell
    X509 Certificate PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\device_cert_store.pem
    X509 Trusted PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\BaltimoreCyberTrustRoot.crt.pem
    Device ID = paho-sample-device1
    IoT Hub Hostname = myiothub.azure-devices.net
    Sending Message 1
    Sending Message 2
    Sending Message 3
    Sending Message 4
    Sending Message 5
    Messages Sent [Press ENTER to shut down]
    ```


    ### Cloud-to-Device (c2d) messages

    This sample requires two actions: 
    - connecting the Azure IoT SDK device client to the hub, and 
    - sending a c2d message through the Azure Portal.

    First, run the sample:

    On the `az.sln` solution open on Visual Studio, 
    - Navigate on the Solution Explorer panel to `paho_iot_hub_c2d_example` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_c2d_example` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 6).
    - Click on "Message to Device".
    - On "Message Body", type "Hello world!" (too cheesy? how about "Lorem Ipsum"?)
    - Click on "Send Message".

    Back to the Visual Studio Command prompt, verify that the message has been received by the sample:

    ```shell
    X509 Certificate PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\device_cert_store.pem
    X509 Trusted PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\BaltimoreCyberTrustRoot.crt.pem
    Device ID = paho-sample-device1
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client paho-sample-device1 rc 0Subscribed to topics.
    Waiting for activity. [Press ENTER to abort]
    C2D Message arrived
    Hello world!
    ``` 

    Note: the sample does not terminate automatically. In the output above Enter has been pressed.


    ### Direct Methods

    This sample requires two actions: 
    - connecting the Azure IoT SDK device client to the hub, and 
    - triggering the direct method through the Azure Portal.

    First, run the sample:

    On the `az.sln` solution open on Visual Studio, 
    - Navigate on the Solution Explorer panel to `paho_iot_hub_methods_example` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_methods_example` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 6).
    - Click on "Direct Method".
    - On "Method Name", type `ping` (the sample expects the name `ping`).
    - On "Payload", type `{ "somevalue": 1234 }` (the payload MUST be a valid Json).
    - Click on "Invoke Method".
    - See the reply from the sample on "Result" (bottom of the page).


    Back to the Visual Studio Command prompt, verify that the direct method has been received by the sample:

    ```shell
    X509 Certificate PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\device_cert_store.pem
    X509 Trusted PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\BaltimoreCyberTrustRoot.crt.pem
    Device ID = paho-sample-device1
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client paho-sample-device1 rc 0Subscribed to topics.
    Waiting for activity. [Press ENTER to abort]
    Topic: $iothub/methods/POST/ping/?$rid=1
    Direct Method arrived
    Status: 200     Payload:{"response": "pong"}
    Sent response
    ```

    Note: the sample does not terminate automatically. In the output above Enter has been pressed.


    ### Device Twin

    On the `az.sln` solution open on Visual Studio, 
    - Navigate on the Solution Explorer panel to `paho_iot_hub_twin_example` solution;
    - Make it the default startup project (right-click on `paho_iot_hub_twin_example` project, then click on `Set as StartUp Project`);
    - Build and run the project (`F5` on most installations).

    Then interact with the sample as bellow:

    ```shell
    X509 Certificate PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\device_cert_store.pem
    X509 Trusted PEM Store File = C:\azure-sdk-for-c\sdk\samples\iot\hub\src\BaltimoreCyberTrustRoot.crt.pem
    Device ID = paho-sample-device1
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client paho-sample-device1 rc 0Subscribed to topics.

    Waiting for activity:
    Press 'g' to get the twin document
    Press 'r' to send a reported property
    [Press 'q' to quit]
    g
    Requesting twin document
    Topic: $iothub/twin/res/200/?$rid=get_twin
    Twin Message Arrived
    A twin GET response was received
    Payload:
    {"desired":{"$version":1},"reported":{"$version":1}}
    Response status was 200

    r
    Sending reported property
    Payload: {"foo":0}
    Topic: $iothub/twin/res/204/?$rid=reported_prop&$version=2
    Twin Message Arrived
    A twin reported properties message was received
    Response status was 204

    g
    Requesting twin document
    Topic: $iothub/twin/res/200/?$rid=get_twin
    Twin Message Arrived
    A twin GET response was received
    Payload:
    {"desired":{"$version":1},"reported":{"foo":0,"$version":2}}
    Response status was 200

    q
    Disconnected.

    ```

## Need Help?

* File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
* Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: ../../../../../CONTRIBUTING.md
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE
[azure_sdk_for_c_contributing_developer_guide]: ../../../../../CONTRIBUTING.md#developer-guide
[azure_sdk_for_c_contributing_pull_requests]: ../../../../../CONTRIBUTING.md#pull-requests
[azure_cli]: https://docs.microsoft.com/cli/azure
[azure_pattern_circuit_breaker]: https://docs.microsoft.com/azure/architecture/patterns/circuit-breaker
[azure_pattern_retry]: https://docs.microsoft.com/azure/architecture/patterns/retry
[azure_portal]: https://portal.azure.com
[azure_sub]: https://azure.microsoft.com/free/
[c_compiler]: https://visualstudio.microsoft.com/vs/features/cplusplus/
[cloud_shell]: https://docs.microsoft.com/azure/cloud-shell/overview
[cloud_shell_bash]: https://shell.azure.com/bash

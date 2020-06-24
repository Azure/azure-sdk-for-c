# How to Setup and Run Azure SDK for Embedded C IoT Hub Samples on Linux

This is a step-by-step guide of how to start from scratch and get the Azure SDK for Embedded C IoT Hub Samples running. 

Pre-requisites:
- [Having created an Azure account](https://github.com/ewertons/azure-sdk-for-c/wiki/How-to-create-an-Azure-account)
- [Having created an Azure IoT Hub](https://github.com/ewertons/azure-sdk-for-c/wiki/How-to-create-an-Azure-IoT-Hub)

What is covered:
- Downloading and building the Azure SDK for Embedded C suite
- Configuring and running the IoT Hub client samples.  

_The following was run on an Ubuntu Desktop 18.04 environment, but it also works on WSL 1 and 2 (Windows Subsystem for Linux)_

01. Install library dependencies

    ```
    sudo apt-get install unzip git build-essential pkg-config libssl-dev
    ```

02. Install the latest cmake

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

03. Install Paho using vcpkg
 
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

04. Clone the Azure SDK for C

    ```shell
    $ cd
    $ git clone https://github.com/azure/azure-sdk-for-c
    ```

05. Generate a self-signed certificate

    The Azure Embedded SDK for C IoT Client samples use a self-signed certificate.

    A script is provided for creating that certificate. 

    ```shell
    $ cd azure-sdk-for-c/sdk/samples/iot/hub/src
    azure-sdk-for-c/sdk/samples/iot/hub/src$ ./generate_certificate.sh 
    ```

    <details>
    <summary>
    Complete output of the `generate_certificate.sh` script.
    </summary>

    ```shell
    azure-sdk-for-c/sdk/samples/iot/hub/src$ ./generate_certificate.sh 
    Certificate:
        Data:
            Version: 1 (0x0)
            Serial Number:
                5c:71:ff:0e:39:87:c3:1b:82:52:09:5d:b7:3c:a2:60:6c:73:92:30
            Signature Algorithm: ecdsa-with-SHA256
            Issuer: CN = paho-sample-device1
            Validity
                Not Before: May  8 05:24:45 2020 GMT
                Not After : May  8 05:24:45 2021 GMT
            Subject: CN = paho-sample-device1
            Subject Public Key Info:
                Public Key Algorithm: id-ecPublicKey
                    Public-Key: (256 bit)
                    pub:
                        04:b4:17:28:5f:03:59:d1:87:82:83:6e:ac:38:6e:
                        1b:60:11:bb:0c:45:b0:a3:4a:a4:83:60:37:73:7b:
                        a4:5b:4c:4e:61:8a:3b:eb:a6:1f:bf:a1:c9:90:d6:
                        7a:64:c4:92:3b:1d:b8:5c:38:2c:21:18:5b:93:5e:
                        1e:16:79:90:b8
                    ASN1 OID: prime256v1
                    NIST CURVE: P-256
        Signature Algorithm: ecdsa-with-SHA256
            30:44:02:20:16:c7:43:c8:8a:88:da:77:18:45:1b:97:65:8e:
            a7:b7:50:6a:d8:77:51:b9:8f:23:2f:36:a1:74:6c:75:cd:cc:
            02:20:5c:b5:19:28:b9:64:07:14:8f:cf:28:0b:17:9e:16:6a:
            8a:d5:3e:91:64:79:39:c6:94:0d:9d:66:e5:45:3f:75
    rm: cannot remove 'device_cert_store.pem': No such file or directory

    It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:
    Microsoft Windows SChannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel
    OR
    Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport

    If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system.
    If required (for example on Windows), download the Baltimore PEM CA from https://www.digicert.com/digicert-root-certificates.htm to the current folder.
    export AZ_IOT_DEVICE_X509_TRUST_PEM_FILE=/azure-sdk-for-c/sdk/samples/iot/hub/src/BaltimoreCyberTrustRoot.crt.pem

    Sample certificate generated
    Use the device_cert_store.pem file within the sample:
    export AZ_IOT_DEVICE_X509_CERT_PEM_FILE=/azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem	

    Use the following fingerprint when creating your device in IoT Hub (must remove colons):
    SHA1 Fingerprint=C8DC8780C64FFBB5A66D8BC5D39D3C1BBB03FB69
    /azure-sdk-for-c/sdk/samples/iot/hub/src$
    ```
    </details>

    Do not forget to set the environment variable above, making sure it maps to your local path.

    ```shell
    $ export AZ_IOT_DEVICE_X509_CERT_PEM_FILE=/azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    ```

    Save the certificate Fingerprint above (in this example, `C8DC8780C64FFBB5A66D8BC5D39D3C1BBB03FB69`).
    It will be used to create the logical device on your Azure IoT Hub.


06. Create a logical device

    - Log into your Azure account on [Azure Portal](https://portal.azure.com)
    - On the menu on the left, click on "IoT devices" under "Explorers".
    - Click on "New".
    - Type an unique ID for your device.
    - Select "X.509 Self-Signed" for "Authentication type". 
    - Type the fingerprint obtained in the previous step (the same can be used for primary and secondary Thumbprints; no colons!).
    - Click on "Save".

07. Collect information about Azure IoT Hub and device

    For the Azure IoT Embedded SDK for C samples, we will need the Azure IoT Hub name and device ID.

    - Get the Azure IoT Hub FQDN.
        - On your Azure IoT Hub page, click on "Overview".
        - Copy and save the "Hostname" value (in this example, "myiothub.azure-devices.net").
    - Get the device ID. 
      - On your Azure IoT Hub page, click on "IoT devices" under "Explorers".
      - On the list of devices, click on the device created on the previous step (in this example, "testdevice-x509").
      - Copy and save the "Device ID" value (in this example, "testdevice-x509"). 


08. Build the Azure SDK for C

    Back into the folder where the Azure SDK for C was cloned...

    ```shell
    /cd ~/azure-sdk-for-c/
    /azure-sdk-for-c$ mkdir cmake
    /azure-sdk-for-c$ cd cmake
    /azure-sdk-for-c/cmake$ cmake -DTRANSPORT_PAHO=ON -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake ..
    /azure-sdk-for-c/cmake$ make -j
    ```


09. Set the environment variables needed for the samples

    According the the [readme documentation](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/iot/hub) for the Azure Embedded SDK for C IoT client samples require the following environment variables.

    ```shell
    export AZ_IOT_DEVICE_ID=<device ID obtained on step 7> 
    export AZ_IOT_HUB_HOSTNAME=<FQDN obtained on step 7>
    ```

    `AZ_IOT_DEVICE_X509_TRUST_PEM_FILE` is not required for all platforms (like Linux).

    `AZ_IOT_DEVICE_X509_CERT_PEM_FILE` has already been set on step 5.

    Using the values from the example above, the export command would look like this (don't run these command lines, you should use your own device ID and hostname)

    ```shell
    $ export AZ_IOT_DEVICE_ID=testdevice-x509
    $ export AZ_IOT_HUB_HOSTNAME=myiothub.azure-devices.net
    ```

10. Run the samples.

    This is a similar list of files you should see in your computer:

    ```shell
    /azure-sdk-for-c/cmake$ cd sdk/samples/iot/hub/
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ll
    total 14772
    drwxr-xr-x 3 user user    4096 May  7 22:18 ./
    drwxr-xr-x 4 user user    4096 May  7 22:18 ../
    drwxr-xr-x 6 user user    4096 May  7 22:17 CMakeFiles/
    -rw-r--r-- 1 user user    1145 May  7 22:17 cmake_install.cmake
    -rw-r--r-- 1 user user     329 May  7 22:17 CTestTestfile.cmake
    -rw-r--r-- 1 user user   14656 May  7 22:17 Makefile
    -rwxr-xr-x 1 user user 3766736 May  7 22:18 paho_iot_hub_c2d_example*
    -rwxr-xr-x 1 user user 3771544 May  7 22:18 paho_iot_hub_methods_example*
    -rwxr-xr-x 1 user user 3766688 May  7 22:18 paho_iot_hub_telemetry_example*
    -rwxr-xr-x 1 user user 3776704 May  7 22:18 paho_iot_hub_twin_example*
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$
    ```

    ### Telemetry (device-to-cloud messages)

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ./paho_iot_hub_telemetry_example 
    X509 Certificate PEM Store File = /azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    X509 Trusted PEM Store File = 
    Device ID = testdevice-x509
    IoT Hub Hostname = myiothub.azure-devices.net
    Sending Message 1
    Sending Message 2
    Sending Message 3
    Sending Message 4
    Sending Message 5
    Messages Sent [Press ENTER to shut down]

    Disconnected.
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$
    ```


    ### Cloud-to-Device (c2d) messages

    This sample requires two actions: 
    - connecting the Azure IoT SDK device client to the hub, and 
    - sending a c2d message through the Azure Portal.

    First, run the sample:

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ./paho_iot_hub_c2d_example 
    X509 Certificate PEM Store File = /azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    X509 Trusted PEM Store File = 
    Device ID = testdevice-x509
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client testdevice-x509 rc 0Subscribed to topics.
    Waiting for activity. [Press ENTER to abort]

    ```

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step 8).
      ![Device page](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_01_device.png)
    - Click on "Message to Device".
    - On "Message Body", type "Hello world!" (too cheesy? how about "Lorem Ipsum"?)
      ![Send message](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_02_c2d.png)
    - Click on "Send Message".
      ![Success](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_03_c2d_success.png)

    Back to the shell, verify that the message has been received by the sample:

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ./paho_iot_hub_c2d_example 
    X509 Certificate PEM Store File = /azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    X509 Trusted PEM Store File = 
    Device ID = testdevice-x509
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client testdevice-x509 rc 0Subscribed to topics.
    Waiting for activity. [Press ENTER to abort]
    C2D Message arrived
    Hello world!

    Disconnected.
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$
    ``` 

    Note: the sample does not terminate automatically. In the output above Enter has been pressed.


    ### Direct Methods

    This sample requires two actions: connecting the Azure IoT SDK device client to the hub, and triggering the direct method through the Azure Portal.

    First, run the sample:

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ./paho_iot_hub_methods_example 
    X509 Certificate PEM Store File = /azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    X509 Trusted PEM Store File = 
    Device ID = testdevice-x509
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client testdevice-x509 rc 0Subscribed to topics.
    Waiting for activity. [Press ENTER to abort]

    ```

    On the Azure Portal,
    - Go to your Azure IoT hub page.
    - Click on "IoT devices" under "Explorers".
    - From the list of devices, click on your device (created on step Z).
    - Click on "Direct Method".
      ![Methods](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/iot/resources/embc_samples_04_methods.png)
    - On "Method Name", type "ping" (the sample expects the name "ping").
    - On "Payload", type '{ "somevalue": 1234 }' (the payload can be empty, but MUST be a valid Json).
    - Click on "Invoke Method".
    - See the reply from the sample on "Result" (bottom of the page).
      ![Response](../../../../docs/iot/resources/embc_samples_05_methods_response.png)


    Back to the shell, verify that the message has been received by the sample:

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ./paho_iot_hub_methods_example 
    X509 Certificate PEM Store File = /azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    X509 Trusted PEM Store File = 
    Device ID = testdevice-x509
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client testdevice-x509 rc 0Subscribed to topics.
    Waiting for activity. [Press ENTER to abort]
    Topic: $iothub/methods/POST/ping/?$rid=1
    Direct Method arrived
    Status: 200	Payload:{"response": "pong"}
    Sent response

    Disconnected.
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$
    ```

    Note: the sample does not terminate automatically. In the output above Enter has been pressed.


    ### Device Twin

    ```shell
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$ ./paho_iot_hub_twin_example 
    X509 Certificate PEM Store File = /azure-sdk-for-c/sdk/samples/iot/hub/src/device_cert_store.pem
    X509 Trusted PEM Store File = 
    Device ID = testdevice-x509
    IoT Hub Hostname = myiothub.azure-devices.net
    Posting connect semaphore for client testdevice-x509 rc 0Subscribed to topics.

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
    /azure-sdk-for-c/cmake/sdk/samples/iot/hub/$
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

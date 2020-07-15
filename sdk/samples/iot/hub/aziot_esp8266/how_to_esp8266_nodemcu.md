# How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Espressif ESP8266 NodeMCU

This is a to-the-point documentation of how to run an Azure SDK for Embedded C IoT Hub Telemetry Samples on an Esp8266 NodeMCU microcontroller. 

Pre-requisites:
- [Having created an Azure account](https://github.com/ewertons/azure-sdk-for-c/wiki/How-to-create-an-Azure-account)
- [Having created an Azure IoT Hub](https://github.com/ewertons/azure-sdk-for-c/wiki/How-to-create-an-Azure-IoT-Hub)
- Create a logical device using Authentication Type "Symmetric Key"
- Latest [Arduino IDE](https://www.arduino.cc/en/Main/Software) installed
- [Azure Command Line Interface utility](https://docs.microsoft.com/en-us/cli/azure/install-azure-cli-apt?view=azure-cli-latest#install-with-one-command) and the IoT extension
  ```shell
  $ curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
  $ az extension add --name azure-iot
  ```
  Alternatively, you can use [azure-iot-explorer](https://github.com/Azure/azure-iot-explorer).

What is covered:
- Configuring Arduino IDE to compile a sample using Azure Embedded SDK for C
- Configuring and running an IoT Hub client telemetry sample.
- The sample uses device keys to automatically generate a SAS token for authentication (which is valid by default for one hour).

_The following was run on an Ubuntu Desktop 18.04 environment, with Arduino IDE 1.8.12._


01. Create an Arduino library for Azure Embedded SDK for C 

    ```
    $ wget https://raw.githubusercontent.com/Azure/azure-sdk-for-c/master/sdk/samples/iot/hub/aziot_esp8266/generate_arduino_zip_library.sh
    $ chmod 777 generate_arduino_zip_library.sh
    $ ./generate_arduino_zip_library.sh
    ```

    This will create a local file named `azure-sdk-for-c.zip` containing the whole [Azure SDK for C](https://github.com/Azure/azure-sdk-for-c) as an Arduino library.


02. Run the Arduino IDE

03. Install the Esp8266 board

    Follow [instructions](https://github.com/esp8266/Arduino#installing-with-boards-manager) in the official Esp8266 repository.

03. Install the Azure SDK for C zip library

    - On the Arduino IDE, go to `Sketch`, `Include Library`, `Add .ZIP Library...`
    - Search for the `azure-sdk-for-c.zip` created on step 01
    - Select the file `azure-sdk-for-c.zip` and click on `OK` 

04. Install the Arduino PubSubClient library

    PubSubClient is a popular MQTT client for Arduino.

    - On Arduino IDE, go to menu `Sketch`, `Include Library`, `Manage Libraries...` 
    - Search for `PubSubClient` (by Nick O'Leary)
    - Hover over the library item on the result list, then click on Install.

05. Create a sketch on Arduino IDE for the IoT Hub client telemetry sample

    Clone the [azure-sdk-for-c](https://github.com/Azure/azure-sdk-for-c) repo locally then open the [ESP8266 sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/hub/aziot_esp8266) (from local clone) on Arduino IDE. 
    
    Edit the following parameters in `iot_configs.h`, filling in your own information:

    ```c
    // Wifi
    #define IOT_CONFIG_WIFI_SSID            "SSID"
    #define IOT_CONFIG_WIFI_PASSWORD        "PWD"

    // Azure IoT
    #define IOT_CONFIG_IOTHUB_FQDN          "[your host name].azure-devices.net"
    #define IOT_CONFIG_DEVICE_ID            "Device ID"
    #define IOT_CONFIG_DEVICE_KEY           "Device Key"
    ```

    Save the file.

06. Connect the Esp8266 NodeMCU microcontroller to your USB port

07. On the Arduino IDE, select the board and port

    - Go to menu `Tools`, `Board` and select `NodeMCU 1.0 (ESP-12E Module)`
    - Go to menu `Tools`, `Port` and select the port where the microcontroller is connected to.

08. Upload the sketch

    - Go to menu `Sketch` and click on `Upload`.

    <details>
    <summary>Expected output of the upload</summary>
    Executable segment sizes:
    IROM   : 361788          - code in flash         (default or ICACHE_FLASH_ATTR) 
    IRAM   : 26972   / 32768 - code in IRAM          (ICACHE_RAM_ATTR, ISRs...) 
    DATA   : 1360  )         - initialized variables (global, static) in RAM/HEAP 
    RODATA : 2152  ) / 81920 - constants             (global, static) in RAM/HEAP 
    BSS    : 26528 )         - zeroed variables      (global, static) in RAM/HEAP 
    Sketch uses 392272 bytes (37%) of program storage space. Maximum is 1044464 bytes.
    Global variables use 30040 bytes (36%) of dynamic memory, leaving 51880 bytes for local variables. Maximum is 81920 bytes.
    /home/user/.arduino15/packages/esp8266/tools/python3/3.7.2-post1/python3 /home/user/.arduino15/packages/esp8266/hardware/esp8266/2.7.1/tools/upload.py --chip esp8266 --port /dev/ttyUSB0 --baud 230400 --before default_reset --after hard_reset write_flash 0x0 /tmp/arduino_build_826987/azure_iot_hub_telemetry.ino.bin 
    esptool.py v2.8
    Serial port /dev/ttyUSB0
    Connecting....
    Chip is ESP8266EX
    Features: WiFi
    Crystal is 26MHz
    MAC: dc:4f:22:5e:a7:09
    Uploading stub...
    Running stub...
    Stub running...
    Changing baud rate to 230400
    Changed.
    Configuring flash size...
    Auto-detected Flash size: 4MB
    Compressed 396432 bytes to 292339...

    Writing at 0x00000000... (5 %)
    Writing at 0x00004000... (11 %)
    Writing at 0x00008000... (16 %)
    Writing at 0x0000c000... (22 %)
    Writing at 0x00010000... (27 %)
    Writing at 0x00014000... (33 %)
    Writing at 0x00018000... (38 %)
    Writing at 0x0001c000... (44 %)
    Writing at 0x00020000... (50 %)
    Writing at 0x00024000... (55 %)
    Writing at 0x00028000... (61 %)
    Writing at 0x0002c000... (66 %)
    Writing at 0x00030000... (72 %)
    Writing at 0x00034000... (77 %)
    Writing at 0x00038000... (83 %)
    Writing at 0x0003c000... (88 %)
    Writing at 0x00040000... (94 %)
    Writing at 0x00044000... (100 %)
    Wrote 396432 bytes (292339 compressed) at 0x00000000 in 13.0 seconds (effective 243.4 kbit/s)...
    Hash of data verified.

    Leaving...
    Hard resetting via RTS pin...
    </details>

09. Monitor the micro-controller

    Go to menu `Tools`, `Serial Monitor`.

    If you run that right away after uploading the sketch, the serial monitor will show a similar output on success:

    ```
    Connecting to WIFI SSID buckaroo
    .......................WiFi connected, IP address: 
    192.168.1.123
    Setting time using SNTP..............................done!
    Current time: Thu May 28 02:55:05 2020
    Client ID: mydeviceid
    Username: myiothub.azure-devices.net/mydeviceid/?api-version=2018-06-30&DeviceClientType=c%2F1.0.0-preview.2
    Password: SharedAccessSignature sr=myiothub.azure-devices.net%2Fdevices%2Fmydeviceid&sig=0VFwGiXlIVPeCmPStJ4Fb1wbS8o2W8p1vzIOt%2B8K2eE%3D&se=1590620105
    MQTT connecting ... connected.

    ```

10. Monitor the telemetry messages sent to the Azure IoT Hub 

    ```
    $ az iot hub monitor-events --login <your Azure IoT Hub connection string in quotes> --device-id <your device id>
    ```

    <details>
    <summary>See the expected output.</summary>
    ```bash
    Starting event monitor, filtering on device: mydeviceid, use ctrl-c to stop...
    {
        "event": {
            "origin": "mydeviceid",
            "payload": "payload"
        }
    }
    {
        "event": {
            "origin": "mydeviceid",
            "payload": "payload"
        }
    }
    {
        "event": {
            "origin": "mydeviceid",
            "payload": "payload"
        }
    }
    {
        "event": {
            "origin": "mydeviceid",
            "payload": "payload"
        }
    }
    {
        "event": {
            "origin": "mydeviceid",
            "payload": "payload"
        }
    }
    {
        "event": {
            "origin": "mydeviceid",
            "payload": "payload"
        }
    }
    ^CStopping event monitor...
    ```
    </details>



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
# How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Espressif ESP8266 NodeMCU

- [How to Setup and Run Azure SDK for Embedded C IoT Hub Client on Espressif ESP8266 NodeMCU](#how-to-setup-and-run-azure-sdk-for-embedded-c-iot-hub-client-on-espressif-esp8266-nodemcu)
  - [Introduction](#introduction)
    - [What is Covered](#what-is-covered)
  - [Prerequisites](#prerequisites)
  - [Setup and Run Instructions](#setup-and-run-instructions)
  - [Troubleshooting](#troubleshooting)
  - [Contributing](#contributing)
    - [License](#license)

## Introduction

This is a "to-the-point" guide outlining how to run an Azure SDK for Embedded C IoT Hub telemetry sample on an Esp8266 NodeMCU microcontroller. The command line examples are tailored to Debian/Ubuntu environments.

### What is Covered

- Configuration instructions for the Arduino IDE to compile a sample using the Azure SDK for Embedded C.
- Configuration, build, and run instructions for the IoT Hub telemetry sample.

_The following was run on an Ubuntu Desktop 20.04 environment, with Arduino IDE 1.8.12 and Esp8266 module 2.7.4._

## Prerequisites

- Have an [Azure account](https://azure.microsoft.com/) created.
- Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created.
- Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using the authentication type "Symmetric Key".

    NOTE: Device keys are used to automatically generate a SAS token for authentication, which is only valid for one hour.

- Have the latest [Arduino IDE](https://www.arduino.cc/en/Main/Software) installed.
- Have one of the following interfaces to your Azure IoT Hub set up:
  - [Azure Command Line Interface](https://docs.microsoft.com/cli/azure/install-azure-cli?view=azure-cli-latest) utility installed, along with the [Azure IoT CLI extension](https://github.com/Azure/azure-iot-cli-extension).

    ```bash
    $ curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash
    $ az extension add --name azure-iot
    ```

    A list of all the Azure IoT CLI extension commands can be found [here](https://docs.microsoft.com/cli/azure/iot?view=azure-cli-latest).

  - The most recent version of [Azure IoT Explorer](https://github.com/Azure/azure-iot-explorer/releases) installed. More instruction on its usage can be found [here](https://docs.microsoft.com/azure/iot-pnp/howto-use-iot-explorer).

  NOTE: This guide demonstrates use of the Azure CLI and does NOT demonstrate use of Azure IoT Explorer.

## Setup and Run Instructions

1. Create an Arduino library for the Azure SDK for Embedded C.

    ```bash
    $ wget https://raw.githubusercontent.com/Azure/azure-sdk-for-c/master/sdk/samples/iot/aziot_esp8266/generate_arduino_zip_library.sh
    $ chmod 777 generate_arduino_zip_library.sh
    $ ./generate_arduino_zip_library.sh
    ```

    This will create a local file named `azure-sdk-for-c.zip` containing the entire [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c) repository as an Arduino library.

    NOTE: If you are using WSL, do not run these commands from the Windows system drive (e.g. `/mnt/c/`).

2. Run the Arduino IDE.

3. Install the Esp8266 board.

    - Follow the [instructions](https://github.com/esp8266/Arduino#installing-with-boards-manager) in the official Esp8266 repository.

4. Install the Azure SDK for Embedded C zip library.

    - On the Arduino IDE, go to `Sketch`, `Include Library`, `Add .ZIP Library...`.
    - Search for the `azure-sdk-for-c.zip` created on step 1.
    - Select the file `azure-sdk-for-c.zip` and click on `OK`.

5. Install the Arduino PubSubClient library. (PubSubClient is a popular MQTT client for Arduino.)

    - On the Arduino IDE, go to menu `Sketch`, `Include Library`, `Manage Libraries...`.
    - Search for `PubSubClient` (by Nick O'Leary).
    - Hover over the library item on the result list, then click on "Install".

6. Create a sketch on Arduino IDE for the IoT Hub telemetry sample.

    - Clone the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c) repository locally and then open the [ESP8266 sample](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/samples/iot/aziot_esp8266) (from the local clone) on the Arduino IDE.

    - Edit the following parameters in `iot_configs.h`, filling in your own information:

        ```c
        // Wifi
        #define IOT_CONFIG_WIFI_SSID            "SSID"
        #define IOT_CONFIG_WIFI_PASSWORD        "PWD"

        // Azure IoT
        #define IOT_CONFIG_IOTHUB_FQDN          "[your host name].azure-devices.net"
        #define IOT_CONFIG_DEVICE_ID            "Device ID"
        #define IOT_CONFIG_DEVICE_KEY           "Device Key"
        ```

    - Save the file.

7. Connect the Esp8266 NodeMCU microcontroller to your USB port.

8. On the Arduino IDE, select the board and port.

    - Go to menu `Tools`, `Board` and select `NodeMCU 1.0 (ESP-12E Module)`.
    - Go to menu `Tools`, `Port` and select the port to which the microcontroller is connected.

9. Upload the sketch.

    - Go to menu `Sketch` and click on `Upload`.

        <details><summary><i>Expected output of the upload:</i></summary>
        <p>

        ```text
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
        ```

        </p>
        </details>

10. Monitor the microcontroller.

    - Go to menu `Tools`, `Serial Monitor`.

        If you perform this step right away after uploading the sketch, the serial monitor will show an output similar to the following upon success:

        ```text
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

11. Monitor the telemetry messages sent to the Azure IoT Hub.

    ```bash
    $ az iot hub monitor-events --login <your Azure IoT Hub connection string in quotes> --device-id <your device id>
    ```

    <details><summary><i>Expected telemetry output:</i></summary>
    <p>

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

    </p>
    </details>

## Troubleshooting

- The error policy for the Embedded C SDK client library is documented [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/mqtt_state_machine.md#error-policy).
- File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using the `azure` and `c` tags.

## Contributing

This project welcomes contributions and suggestions. Find more contributing details [here](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

### License

Azure SDK for Embedded C is licensed under the [MIT](https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE) license.

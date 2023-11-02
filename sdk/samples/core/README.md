---
page_type: sample
languages:
  - c
products:
  - azure
urlFragment: core-samples
---

# Azure Core Samples client Library for Embedded C

This document explains MQTT pattern samples and how to use them.

The samples' instructions include specifics for Linux based systems. The Linux examples are tailored to Debian/Ubuntu environments. While Linux devices are not likely to be considered constrained, these samples enable developers to test the Azure SDK for Embedded C libraries, debug, and step through the code, even without a real device. We understand not everyone will have a real device to test and that sometimes these devices won't have debugging capabilities.

**WARNING: Samples are generic and should not be used in any production-level code.**

## Key Concepts

## Prerequisites

- Have the following build environment setup:
  ```
  sudo apt-get update
  sudo apt-get install build-essential curl zip unzip tar pkg-config
  ```
- Have [Git](https://git-scm.com/download) installed.
- Have Microsoft [vcpkg](https://github.com/microsoft/vcpkg) package manager and [Mosquitto MQTT C client](https://mosquitto.org/download/) or [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. This installation may take an extended amount of time (~15-20 minutes).

    <details><summary><i>Instructions:</i></summary>
    <p>

    NOTE: For the correct vcpkg commit, see [vcpkg-commit.txt](https://github.com/Azure/azure-sdk-for-c/blob/main/eng/vcpkg-commit.txt).

    Linux:

    ```bash
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    git checkout <vcpkg commit> # Checkout the vcpkg commit per vcpkg-commit.txt above.
    ./bootstrap-vcpkg.sh
    ./vcpkg install --triplet x64-linux curl cmocka paho-mqtt mosquitto
    ```

    </p>
    </details>

- Have OpenSSL installed.

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    ```bash
    sudo apt-get install openssl libssl-dev
    ```

    </p>
    </details>

- Have CMake installed. The minimum required is 3.11.

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    - Ubuntu 18.04 or 20.04 or 22.04:

      ```bash
      sudo apt-get install cmake
      ```

    - Ubuntu 16.04: Download the latest version of [CMake](https://cmake.org/files).

      ```bash
      wget https://cmake.org/files/v3.18/cmake-3.18.3-Linux-x86_64.sh # Use latest version.
      sudo ./cmake-3.18.3-Linux-x86_64.sh --prefix=/usr
      ```
        - When prompted to include the default subdirectory, enter `n` so to install in `/usr/local`.

    </p>
    </details>

- Have the UUID Library installed if running a sample that has an RPC Client.

    <details><summary><i>Instructions:</i></summary>
    <p>

    Linux:

    ```bash
    sudo apt-get install uuid-dev
    ```

    </p>
    </details>


- Have the Azure SDK for Embedded C IoT repository cloned.

    ```bash
    git clone https://github.com/Azure/azure-sdk-for-c.git
    ```


## Getting Sarted

### Set up the Sample
- Open the file of the sample you'd like to run. Ex. sdk/samples/core/mosquitto_rpc_client_sample.c
- Fill out the User-defined parameters section at the top of the file - at minimum cert path, key path, and hostname, but all of these are customizable for your solution.
- In the main function, adjust any configuration options as needed

## Build and Run the Sample
1. Build the Azure SDK for Embedded C directory structure.

    From the root of the SDK directory `azure-sdk-for-c`:

    ```bash
    mkdir build
    cd build
    # for mosquitto samples
    cmake -DAZ_MQTT_TRANSPORT_IMPL=MOSQUITTO -DAZ_PLATFORM_IMPL=POSIX -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg_repo>/scripts/buildsystems/vcpkg.cmake ..
    # for paho samples
    cmake -DAZ_MQTT_TRANSPORT_IMPL=PAHO -DAZ_PLATFORM_IMPL=POSIX -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg_repo>/scripts/buildsystems/vcpkg.cmake ..
    ```
2. Compile and run the sample.

    Linux:

    ```bash
    cmake --build .
    ./sdk/samples/core/<sample executable here>
    ```

## Sample Descriptions
This section provides an overview of the different samples available to run and what to expect from each.

### Mosquitto RPC Client Sample
- Executable `mosquitto_rpc_client_sample`

  This sample sends RPC execution requests to an RPC Server and waits for a response or times out. It currently sends the request every few seconds, but this is fully configurable to your solution. This sample uses the mosquitto MQTT client to send and receive messages.

### Mosquitto RPC Server Sample
- Executable `mosquitto_rpc_server_sample`

  This sample receives RPC execution requests from an RPC Client, executes the command, and sends a response back to the RPC Client with any execution details. This sample uses the mosquitto MQTT client to send and receive messages.

### Mosquitto RPC Client & Server Sample
- Executable `mosquitto_multiple_rpc_sample`

  This sample has both an RPC Client and an RPC Server on the same device, handling and sending multiple commands. The RPC Client sends RPC execution requests to an external RPC Server and receives RPC execution requests from an external RPC Client. This sample uses the mosquitto MQTT client to send and receive messages.

### Paho RPC Client Sample
- Executable `paho_rpc_client_sample`

  This sample sends RPC execution requests to an RPC Server and waits for a response or times out. It currently sends the request every few seconds, but this is fully configurable to your solution. This sample uses the Paho MQTT client to send and receive messages.

### Paho RPC Server Sample
- Executable `paho_rpc_server_sample`

  This sample receives RPC execution requests from an RPC Client, executes the command, and sends a response back to the RPC Client with any execution details. This sample uses the Paho MQTT client to send and receive messages.

### Paho RPC Client & Server Sample
- Executable `paho_multiple_rpc_sample`

  This sample has both an RPC Client and an RPC Server on the same device, handling and sending multiple commands. The RPC Client sends RPC execution requests to an external RPC Server and receives RPC execution requests from an external RPC Client. This sample uses the Paho MQTT client to send and receive messages.


## Contributing

This project welcomes contributions and suggestions. Find [more contributing][SDK_README_CONTRIBUTING] details here.

<!-- LINKS -->
[SDK_README_CONTRIBUTING]: https://github.com/Azure/azure-sdk-for-c/blob/main/CONTRIBUTING.md


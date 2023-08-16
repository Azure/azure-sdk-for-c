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
<!-- - Have Microsoft [vcpkg](https://github.com/microsoft/vcpkg) package manager and [Mosquitto MQTT C client](TODO) or [Eclipse Paho MQTT C client](https://www.eclipse.org/paho/) installed. This installation may take an extended amount of time (~15-20 minutes).

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
    </details> -->

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

- Have the Azure SDK for Embedded C IoT repository cloned.

    ```bash
    git clone https://github.com/Azure/azure-sdk-for-c.git
    ```


## Getting Sarted

### Certificate Creation

### Set Environment Variables

Samples use environment variables for a variety of purposes, including filepaths and connection parameters. Please keep in mind, **every time a new terminal is opened, the environment variables will have to be reset**. Setting a variable will take the following form:

  Linux:

  ```bash
  export ENV_VARIABLE_NAME=VALUE
  ```

Set the following environment variables for all samples:

  1. Set the vcpkg environment variables.

      Refer to these [directions](https://github.com/Azure/azure-sdk-for-c#development-environment) for more detail.

      Linux:

      ```bash
      export VCPKG_DEFAULT_TRIPLET=x64-linux
      <!-- export VCPKG_ROOT=<FULL PATH to vcpkg> -->
      ```

  2. Set broker details in the sample:

## Build and Run the Sample
1. Build the Azure SDK for Embedded C directory structure.

    From the root of the SDK directory `azure-sdk-for-c`:

    ```bash
    mkdir build
    cd build
    cmake -DAZ_MQTT_TRANSPORT_IMPL=MOSQUITTO -DAZ_PLATFORM_IMPL=POSIX ..
    ```
2. Compile and run the sample.

    Linux:

    ```bash
    cmake --build .
    ./sdk/samples/core/<sample executable here>
    ```
The following settings are needed in settings.json for VSCode (or these set in cmake / use the VSCode CMake extension)

```json
"cmake.configureEnvironment": {
    "VCPKG_ROOT": "<path to vcpkg>",
    "VCPKG_DEFAULT_TRIPLET": "x64-linux"
  },
  "cmake.configureSettings": {
    "WARNINGS_AS_ERRORS" : "ON",
    "TRANSPORT_CURL" : "OFF",
    "UNIT_TESTING" : "OFF",
    "UNIT_TESTING_MOCKS" : "OFF",
    "TRANSPORT_PAHO" : "OFF",
    "PRECONDITIONS" : "ON",
    "LOGGING" : "ON",
    "CMAKE_TOOLCHAIN_FILE" : "<path to vcpkg.cmake>",
    "AZ_MQTT_TRANSPORT_IMPL" : "MOSQUITTO",
    "AZ_PLATFORM_IMPL" : "POSIX"
  },
```

## Examples

## Troubleshooting

## Next Steps

### Additional Documentation

## Contributing

This project welcomes contributions and suggestions. Find [more contributing][SDK_README_CONTRIBUTING] details here.

<!-- LINKS -->
[SDK_README_CONTRIBUTING]: https://github.com/Azure/azure-sdk-for-c/blob/main/CONTRIBUTING.md

![Impressions](https://azure-sdk-impressions.azurewebsites.net/api/impressions/azure-sdk-for-c%2Fsdk%2Fcore%2Fcore%2Fsamples%2FREADME.png)

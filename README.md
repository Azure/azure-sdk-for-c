# Azure SDK for Embedded C

[![Build Status](https://dev.azure.com/azure-sdk/public/_apis/build/status/c/c%20-%20client%20-%20ci?branchName=master)](https://dev.azure.com/azure-sdk/public/_build/latest?definitionId=722&branchName=master)

The Azure SDK for Embedded C is designed to allow small embedded (IoT) devices to communicate with Azure services. Since we expect our client library code to run on microcontrollers, which have very limited amounts of flash and RAM, and have slower CPUs, our C SDK does things very differently than the SDKs we offer for other languages.

With this in mind, there are many tenants or principles that we follow in order to properly address this target audience:

- Customers of our SDK compile our source code along with their own.

- We target the C99 programming language and test with gcc, clang, & MS Visual C compilers.

- We offer very few abstractions making our code easy to understand and debug.

- Our SDK is non allocating. That is, customers must allocate our data structures where they desire (global memory, heap, stack, etc.) and then pass the address of the allocated structure into our functions to initialize them and in order to perform various operations.

- Unlike our other language SDKs, many things (such as composing an HTTP pipeline of policies) are done in source code as opposed to runtime. This reduces code size, improves execution speed and locks-in behavior, reducing the chance of bugs at runtime.

- We support microcontrollers with no operating system, microcontrollers with a real-time operating system (like [Azure RTOS](http://rtos.com)), Linux, and Windows. Customers can implement their own "platform layer" to use our SDK on devices we don’t support out-of-the-box. The platform layer requires minimal functionality such as a clock, a mutex, sleep, and an HTTP stack. We provide some platform layers, and more will be added over time.

## The GitHub Repository

To get help with the SDK:

- File a [Github Issue](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
- Ask new questions or see others' questions on [Stack Overflow](https://stackoverflow.com/questions/tagged/azure+c) using the `azure` and `c` tags.

### Master Branch

The master branch has the most recent code with new features and bug fixes. It does **not** represent the latest General Availability (**GA**) release of the SDK.

### Release Branches and Release Tagging

When we make an official release, we will create a unique git tag containing the name and version to mark the commit. We'll use this tag for servicing via hotfix branches as well as debugging the code for a particular preview or stable release version. A release tag looks like this:

   `<package-name>_<package-version>`

 For more information, please see this [branching strategy](https://github.com/Azure/azure-sdk/blob/master/docs/policies/repobranching.md#release-tagging) document.

## Getting Started Using the SDK

1. Install the required prerequisites:
   - [CMake](https://cmake.org/download/) version 3.10 or later
   - C compiler: [MSVC](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019), [gcc](https://gcc.gnu.org/) or [clang](https://clang.llvm.org/) are recommended
   - [git](https://git-scm.com/downloads) to clone our Azure SDK repository with the desired tag

2. Clone our Azure SDK repository, optionally using the desired version tag.

        git clone https://github.com/Azure/azure-sdk-for-c

        git checkout <tag_name>

    For information about using a specific client library, see the README file located in the client library's folder which is a subdirectory under the [`/sdk`](sdk) folder.

3. Ensure the SDK builds correctly.

   - Create an output directory for your build artifacts (in this example, we named it `build`, but you can pick any name).

          mkdir build

   - Navigate to that newly created directory.

          cd build

   - Run `cmake` pointing to the sources at the root of the repo to generate the builds files.

          cmake ..

   - Launch the underlying build system to compile the libraries.

          cmake --build .

   This results in building each library as a static library file, placed in the output directory you created (for example `build\sdk\core\core\Debug`). At a minimum, you must have an `Azure Core` library, a `Platform` library, and an `HTTP` library. Then, you can build any additional Azure service client library you intend to use from within your application (for example `build\sdk\storage\blobs\Debug`). To use our client libraries in your application, just `#include` our public header files and then link your application's object files with our library files.

4. Provide platform-specific implementations for functionality required by `Azure Core`. For more information, see the [Azure Core Porting Guide](sdk/core/core/README.md#Porting-the-Azure-SDK-to-Another-Platform).

### Compiler Options

By default, when building the project with no options, the following static libraries are generated:

- ``Libraries``:
  - az_core
    - az_span, az_http, az_json, etc.
  - az_iot
    -  iot_provisioning, iot_hub, etc.
  - az_storage_blobs
    -  Storage SDK blobs client.
  - az_noplatform
    - Library that provides a basic returning error for platform abstraction as AZ_NOT_IMPLEMENTED. This ensures the project can be compiled without the need to provide any specific platform implementation. This is useful if you want to use az_core without platform specific functions like `mutex` or `time`. 
  - az_nohttp
    -  Library that provides a basic returning error when calling HTTP stack. Similar to az_noplatform, this library ensures the project can be compiled without requiring any HTTP stack implementation. This is useful if you want to use `az_core` without `az_http` functionality.

The following compiler options are available for adding/removing project features.

<table>
<tr>
<td>Option</td>
<td>Description</td>
<td>Default Value</td>
</tr>
<tr>
<td>UNIT_TESTING</td>
<td>Generates Unit Test for compilation. When turning this option ON, cmocka is a required dependency for compilation.<br>After Compiling, use `ctest` to run Unit Test.</td>
<td>OFF</td>
</tr>
<tr>
<td>UNIT_TESTING_MOCKS</td>
<td>This option works only with GCC. It uses -ld option from linker to mock functions during unit test. This is used to test platform or HTTP functions by mocking the return values.</td>
<td>OFF</td>
</tr>
<tr>
<td>PRECONDITIONS</td>
<td>Turning this option ON would remove all method contracts. This us typically for shipping libraries for production to make it as much optimized as possible.</td>
<td>ON</td>
</tr>
<tr>
<td>TRANSPORT_CURL</td>
<td>This option requires Libcurl dependency to be available. It generates an HTTP stack with libcurl for az_http to be able to send requests thru the wire. This library would replace the no_http.</td>
<td>OFF</td>
</tr>
<tr>
<td>TRANSPORT_PAHO</td>
<td>This option requires paho-mqtt dependency to be available. Provides Paho MQTT support for iot.</td>
<td>OFF</td>
</tr>
<tr>
<td>PLATFORM</td>
<td>This option can be set to any of the next values:<br>- No_value: default value is used and no_platform library is used.<br>- "POSIX": Provides implementation for Linux and Mac systems.<br>- "WIN32": Provides platform implementation for Windows based system<br>- "CUSTOM": Tells cmake to use an specific implementation provided by user. When setting this option, user must provide an implementation library and set option `CUSTOM_PLATFORM_NAME` with the name of the library (i.e. <code>-DPLATFORM=CUSTOM -DCUSTOM_PLATFORM_NAME=user_platform_lib</code>). cmake will look for this library to link az_core</td>
<td>No_value</td>
</tr>
</table>

- ``Samples``: Whenever UNIT_TESTING is ON, samples are built using the default PAL (see [running samples section](#running-samples)). This means that running samples would throw errors like:

      ./keys_client_example
      Running sample with no_op HTTP implementation.
      Recompile az_core with an HTTP client implementation like CURL to see sample sending network requests.

      i.e. cmake -DBUILD_CURL_TRANSPORT=ON ..

## Running Samples

See [compiler options section](#compiler-options) to learn about how to build samples with HTTP implementation in order to be runnable.

After building samples with HTTP stack, set the environment variables for credentials. The samples read these environment values to authenticate to Azure services. See [client secret here](https://docs.microsoft.com/en-us/azure/active-directory/azuread-dev/v1-oauth2-on-behalf-of-flow#service-to-service-access-token-request) for additional details on Azure authentication.

```bash
# On linux, set env var like this. For Windows, do it from advanced settings/ env variables

# KEY-VAULT Sample
export AZURE_TENANT_ID="????????-????-????-????-????????????"
export AZURE_CLIENT_ID="????????-????-????-????-????????????"
export AZURE_CLIENT_SECRET="????????????"
export AZURE_KEYVAULT_URL="https://???????????.??"

# STORAGE Sample (only 1 env var required)
# URL must contain a valid container, blob and SaS token
# e.g "https://storageAccount.blob.core.windows.net/container/blob?sv=xxx&ss=xx&srt=xx&sp=xx&se=xx&st=xxx&spr=https,http&sig=xxx"
export AZURE_STORAGE_URL="https://??????????????"
```

### Development Environment

Project contains files to work on Windows, Mac or Linux based OS.

**Note** For any environment variables set to use with CMake, the environment variables must be set
BEFORE the first cmake generation command (`cmake ..`). The environment variables will NOT be picked up
if you have already generated the build files, set environment variables, and then regenerate. In that
case, you must either delete the `CMakeCache.txt` file or delete the folder in which you are generating build
files and start again.

### Windows

vcpkg is the easiest way to have dependencies installed. It downloads packages sources, headers and build libraries for whatever TRIPLET is set up (platform/arq).
VCPKG maintains any installed package inside its own folder, allowing to have multiple vcpkg folder with different dependencies installed on each. This is also great because you don't have to install dependencies globally on your system.

Follow next steps to install VCPKG and have it linked to cmake

```bash
# Clone vcpkg:
git clone https://github.com/Microsoft/vcpkg.git
# (consider this path as PATH_TO_VCPKG)
cd vcpkg
# build vcpkg (remove .bat on Linux/Mac)
.\bootstrap-vcpkg.bat
# install dependencies (remove .exe in Linux/Mac) and update triplet
vcpkg.exe install --triplet x64-windows-static curl[winssl] cmocka paho-mqtt
# Add this environment variables to link this VCPKG folder with cmake:
# VCPKG_DEFAULT_TRIPLET=x64-windows-static
# VCPKG_ROOT=PATH_TO_VCPKG (replace PATH_TO_VCPKG for where vcpkg is installed)
```

Follow next steps to build project from command prompt:

```bash
# cd to project folder
cd azure_sdk_for_c
# create a new folder to generate cmake files for building (i.e. build)
mkdir build
cd build
# generate files
# cmake will automatically detect what C compiler is used by system by default and will generate files for it
cmake ..
# compile files. Cmake would call compiler and linker to generate libs
cmake --build .
```

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See section [Compiler Options](#compiler-options)

#### Visual Studio 2019

Open project folder with Visual Studio. If VCPKG has been previously installed and set up like mentioned [above](#VCPKG). Everything will be ready to build.
Right after opening project, Visual Studio will read cmake files and generate cache files automatically.

### Linux

#### VCPKG

VCPKG can be used to download packages sources, headers and build libraries for whatever TRIPLET is set up (platform/architecture).
VCPKG maintains any installed package inside its own folder, allowing to have multiple vcpkg folder with different dependencies installed on each. This is also great because you don't have to install dependencies globally on your system.

Follow next steps to install VCPKG and have it linked to cmake

```bash
# Clone vcpkg:
 # (consider this path as PATH_TO_VCPKG)
cd vcpkg
# build vcpkg
./bootstrap-vcpkg.sh
./vcpkg install --triplet x64-linux curl cmocka paho-mqtt
export VCPKG_DEFAULT_TRIPLET=x64-linux
export VCPKG_ROOT=PATH_TO_VCPKG #replace PATH_TO_VCPKG for where vcpkg is installed
```

#### Debian

Alternatively, for Ubuntu 18.04 you can use:

`sudo apt install build-essential cmake libcmocka-dev libcmocka0 gcovr lcov doxygen curl libcurl4-openssl-dev libssl-dev ca-certificates`

#### Build

```bash
# cd to project folder
cd azure_sdk_for_c
# create a new folder to generate cmake files for building (i.e. build)
mkdir build
cd build
# generate files
# cmake will automatically detect what C compiler is used by system by default and will generate files for it
cmake ..
# compile files. Cmake would call compiler and linker to generate libs
make
```

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See section [Compiler Options](#compiler-options)

### Mac

#### VCPKG
VCPKG can be used to download packages sources, headers and build libraries for whatever TRIPLET is set up (platform/architecture).
VCPKG maintains any installed package inside its own folder, allowing to have multiple vcpkg folder with different dependencies installed on each. This is also great because you don't have to install dependencies globally on your system.

First, ensure that you have the latest `gcc` installed:

```
brew update
brew upgrade
brew info gcc
brew install gcc
brew cleanup
```

Follow next steps to install VCPKG and have it linked to cmake

```bash
# Clone vcpkg:
git clone https://github.com/Microsoft/vcpkg.git
# (consider this path as PATH_TO_VCPKG)
cd vcpkg
# build vcpkg
./bootstrap-vcpkg.sh
./vcpkg install --triplet x64-osx curl cmocka paho-mqtt
export VCPKG_DEFAULT_TRIPLET=x64-osx
export VCPKG_ROOT=PATH_TO_VCPKG #replace PATH_TO_VCPKG for where vcpkg is installed
```

#### Build

```bash
# cd to project folder
cd azure_sdk_for_c
# create a new folder to generate cmake files for building (i.e. build)
mkdir build
cd build
# generate files
# cmake will automatically detect what C compiler is used by system by default and will generate files for it
cmake ..
# compile files. Cmake would call compiler and linker to generate libs
make
```

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See section [Compiler Options](#compiler-options)


## SDK Architecture

At the heart of our SDK is, what we refer to as, [Azure Core](sdk/core/core). This code defines several data types and functions for use by the client libraries that build on top of us such as an [Azure Storage Blob](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/storage) client library and [Azure IoT client libraries](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/iot). Here are some of the features that customers use directly:

- **Spans**: A span represents a byte buffer and is used for string manipulations, HTTP requests/responses, building/parsing JSON payloads. It allows us to return a substring within a larger string without any memory allocations. See the [Working With Spans](sdk/core/core/README.md#Working-With-Spans) section of the `Azure Core` README for more information.

- **Logging**: As our SDK performs operations, it can send log messages to a customer-defined callback. Customers can enable this to assist with debugging and diagnosing issues when leveraging our SDK code. See the [Logging SDK Operations](sdk/core/core/README.md#Logging-SDK-Operations) section of the `Azure Core` README for more information.

- **Contexts**: Contexts offer an I/O cancellation mechanism. Multiple contexts can be composed together in your application’s call tree. When a context is canceled, its children are also canceled. See the [Canceling an Operation](sdk/core/core/README.md#Canceling-an-Operation) section of the `Azure Core` README for more information.

- **JSON**: Non-allocating JSON builder and JSON parsing data structures and operations.

- **HTTP**: Non-allocating HTTP request and HTTP response data structures and operations.

- **Argument Validation**: The SDK validates function arguments and invokes a callback when validation fails. By default, this callback suspends the calling thread _forever_. However, you can override this behavior and, in fact, you can disable all argument validation to get smaller and faster code. See the [SDK Function Argument Validation](sdk/core/core/README.md#sdk-function-argument-validation) section of the `Azure Core` README for more information.

In addition to the above features, `Azure Core` provides features available to client libraries written to access other Azure services. Customers use these features indirectly by way of interacting with a client library. By providing these features in `Azure Core`, the client libraries built on top of us will share a common implementation and many features will behave identically across client libraries. For example, `Azure Core` offers a standard set of credential types and an HTTP pipeline with logging, retry, and telemetry policies.

## Contributing

For details on contributing to this repository, see the [contributing guide](CONTRIBUTING.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit [https://cla.microsoft.com](https://cla.microsoft.com).

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Additional Helpful Links for Contributors

Many people all over the world have helped make this project better.  You'll want to check out:

* [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
* [How to build and test your change](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#developer-guide)
* [How you can make a change happen!](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#pull-requests)

### Community

* Chat with other community members [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting Security Issues and Security Bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure SDK for Embedded C is licensed under the [MIT](https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE) license.

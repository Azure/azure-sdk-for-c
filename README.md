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
- Ask new qustions or see others' questions on [Stack Overflow](https://stackoverflow.com/questions/tagged/azure+c) using the `azure` and `c` tags.

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

    ```bash
    git clone https://github.com/Azure/azure-sdk-for-c
    ```

    ```bash
    git checkout <tag_name>
    ```

    For information about using a specific client library, see the README file located in the client library's folder which is a subdirectory under the [`/sdk`](sdk) folder.

3. Ensure the SDK builds correctly.

   - Create an output directory for your build artifacts (in this example, we named it `build`, but you can pick any name).

   ```bash
   mkdir build
   ```

   - Navigate to that newly created directory.

   ```bash
   cd build
   ```

   - Run `cmake` pointing to the sources at the root of the repo to generate the builds files.

    ```bash
    cmake ..
    ```

   - Launch the underlying build system to compile the libraries.

    ```bash
    cmake --build .
    ```

   This results in building each library as a static library file, placed in the output directory you created (for example `build\sdk\core\core\Debug`). At a minimum, you must have an `Azure Core` library, a `Platform` library, and an `HTTP` library. Then, you can build any additional Azure service client library you intend to use from within your application (for example `build\sdk\storage\blobs\Debug`). To use our client libraries in your application, just `#include` our public header files and then link your application's object files with our libray files.

4. Provide platform-specific implementations for functionality required by `Azure Core`. For more information, see the [Azure Core Porting Guide](sdk/core/core/README.md#Porting-the-Azure-SDK-to-Another-Platform).

## SDK Architecture

At the heart of our SDK is, what we refer to as, [Azure Core](sdk/core/core). This code defines several data types and functions for use by the client libraries that build on top of us such as an `Azure Storage Blob` client library and `IoT` [client libraries](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/iot). Here are some of the features that customers use directly:

- **Spans**: A span represents a byte buffer and is used for string manipulations, HTTP requests/responses, building/parsing JSON payloads. It allows us to return a substring within a larger string without any memory allocations. See the [Working With Spans](sdk/core/core/README.md#Working-With-Spans) section of the `Azure Core` README for more information.

- **Logging**: As our SDK performs operations, it can send log messages to a customer-defined callback. Customers can enable this to assist with debugging and diagnosing issues when leveraging our SDK code. See the [Logging SDK Operations](sdk/core/core/README.md#Logging-SDK-Operations) section of the `Azure Core` README for more information.

- **Contexts**: Contexts offer an I/O cancellation mechanism. Multiple contexts can be composed together in your application’s call tree. When a context is canceled, its children are also canceled. See the [Canceling an Operation](sdk/core/core/README.md#Canceling-an-Operation) section of the `Azure Core` README for more information.

- **JSON**: Non-allocating JSON builder and JSON parsing data structures and operations.

- **HTTP**: Non-allocating HTTP request and HTTP response data structures and operations.

In addition to the above features, `Azure Core` provides features available to client libraries written to access other Azure services. Customers use these features indirectly by way of interacting with a client library. By providing these features in `Azure Core`, the client libraries built on top of us will share a common implementation and many features will behave identically across client libraries. For example, `Azure Core` offers a standard set of credential types and an HTTP pipeline with logging, retry, and telemetry policies.

## Contributing

For details on contributing to this repository, see the [contributing guide](CONTRIBUTING.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit [https://cla.microsoft.com](https://cla.microsoft.com).

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Additional Helpful Links for Contributors

Many people all over the world have helped make this project better. You'll want to check out:

- [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
- [How to build and test your change](CONTRIBUTING.md#developer-guide)
- [How you can make a change happen!](CONTRIBUTING.md#pull-requests)

### Community

- Chat with other community members: [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting Security Issues and Security Bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure SDK for Embedded C is licensed under the [MIT](LICENSE) license.

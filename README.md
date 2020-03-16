# Azure SDK for Embedded C

[![Build Status](https://dev.azure.com/azure-sdk/public/_apis/build/status/c/c%20-%20client%20-%20ci?branchName=master)](https://dev.azure.com/azure-sdk/public/_build/latest?definitionId=722&branchName=master)

The Azure C SDK is designed to allow small embedded (IoT) devices to communicate with Azure services. Since we expect our client library code to run on microcontrollers which have very limited amounts of flash, RAM, and slower CPUs, our C SDK does things very differently than the SDKs we offer for other languages. 

With this in mind, there are many tenants or principles that we follow in order to properly address this target audience:

-	Customers of our SDK compile our source code along with their own

- We target C99 and test with gcc, clang, & MS Visual C compilers

- We offer very few abstractions making our code easy to understand and debug

- Our SDK is non allocating. That is, customers must allocate our data structures where they desire (global memory, heap, stack, etc.) and then pass the address of the allocated structure into our functions to initialize them and in order to perform various operations. 

- Unlike our other language SDKs, many things (such as composing an HTTP pipeline of policies) are done in source code as opposed to runtime. This reduces code size, improves execution speed and locks-in behavior reducing the chance of bugs at runtime.

- We support microcontrollers with no operating system, microcontrollers with a real-time operating system (like [Azure RTOS](http://rtos.com)), Linux, and Windows. Customers can implement their own “platform layer” to use our SDK on devices we don’t support out-of-the-box. The platform layer requires minimal functionality such as a clock, a mutex, sleep, and an HTTP stack. We provide some platform layers, and more will be added over time.

## The GitHub Repository

To get help with the SDK:
* File a [Github Issue](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
* Ask new qustions or see others' questions on [Stack Overflow](https://stackoverflow.com/questions/tagged/azure+c) using the `azure` and `c` tags.


### Master Branch

The master branch has the most recent code with new features and bug fixes. It does **not** represent the latest released **GA** SDK.

### Release Branches and Release Tagging

When we make an official release, we will create a unique git tag containing the name and version to mark the commit. We'll use this tag for servicing via hotfix branches as well as debugging the code for a particular preview or stable release version. A release tag looks like this:

   `<package-name>_<package-version>`
 
 For more information please see [branching strategy](https://github.com/Azure/azure-sdk/blob/master/docs/policies/repobranching.md#release-tagging).

## Getting Started Using the SDK

1. Install the required prerequisites:
   * [CMake](http://cmake.org) v3.12 or later
   * C compiler: [MSVC](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019), [gcc](https://gcc.gnu.org/) or [clang](https://clang.llvm.org/) are recommended
   * [git](https://git-scm.com/): to clone our Azure SDK repository with the desired tag

2. Clone our Azure SDK repository using the desired version tag

   `Ahson: please show an example command line`

   For information about using a specific client library, see the **README.md** file located in the client library's folder; a subdirecotory under the [`/sdk`](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk) folder.

3. Ensure the SDK builds correctly. 
   `Ahson: How to run cmake`

   The result of building each library is a static library file; placed <Ahson>. At a minimum, you must have an Azure Core library, Platofrm library, and HTTP library. And then, any addition Azure service client libraries you intened to use from within your application. To use our client libraries in your application, just `#include` our public header files and then link your application's object files with our libray files.

4. Provide platform-specific implementations for functionality required by Azure Core. For more information, see the [Azure Core Porting Guide](#Azure-Core-Porting-Guide).

## SDK Architecture
At the heart of our SDK is what we refer to as [Azure Core](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/core). This code defines several data types and functions for use by the client libraries that build on top of us such as an Azure Storage Blob client library and IoT client libraries. The bullets below list some of the features that customers use directly:

- **Spans**: A span represents a byte buffer and is used for string manipulations, HTTP requests/responses, building/parsing JSON payloads. It allows us to return a substring within a larger string without any memory allocations. See [below](##Working-with-Spans) for more information.

- **Logging**: As our SDK performs operations, it can send log messages to a customer-defined callback. Customers can enable this to assist with debugging and diagnosing issues when leveraging our SDK code. See [below](##Logging-SDK-Operations) for more information.

- **Contexts**: Contexts offer a I/O cancellation mechanism. Multiple contexts can be composed together in your application’s call tree. When a context is cancelled, its children are also canceled. See [below](##Cancelling-an-Operation) for more information.

- **JSON**: Non-allocating JSON builder and JSON parsing data structures and operations.

- **HTTP**: Non-allocating HTTP request and HTTP response data structures and operations. 

In addition to the above features, Azure Core provides features available to client libraries written to access other Azure services. Customers uses these features indirectly by way of interacting with a client library. By providing these features in Azure Core, the client libraries built on top of us will share a common implementation and many features will behave identically across client libraries. For example, Azure Core offers a standard set of credential types and an HTTP pipeline with logging, retry, and telemetry policies. 

## Working with Spans
An `az_span` is a small data structure (defined in our [az_span.h](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/core/core/inc/az_span.h) file) wrapping a byte buffer. Specifically, an `az_span` instance contains:
- a byte pointer
- an integer capacity
- an integer length

Our SDK passes `az_span` instances to functions to ensure that a buffer’s address capacity, and length are always passed together; this reduces the chance of bugs. And, since we have the length and capacity, operations are fast; for example, we never need to call `strlen` to find the length of a string in order to append to it. Furthermore, when our SDK functions write or append to an `az_span`, our functions ensure that we never write beyond the capacity of the buffer; this prevents data corruption. And finally, when reading from an `az_span`, we never read past the `az_span`’s length ensuring that we don’t process uninitialized data.

Since many of our SDK functions require `az_span` parameters, customers must know how to create `az_span` instances so that you can call functions in our SDK. Here are some examples.

Create an empty (or NULL) `az_span`:

```C
az_span span_null = AZ_SPAN_NULL;	// cap=0, len = 0
```

Create an `az_span` literal from an uninitialized byte buffer:

```C
uint8_t buffer[1024];   
az_span span_over_buffer = AZ_SPAN_LITERAL_FROM_BUFFER(buffer);	// cap=1024, len = 0
```

Create an `az_span` literal from an initialized bytes buffer: 

```C
uint8_t buffer[] = { 1, 2, 3, 4, 5 };
az_span span_over_buffer = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(buffer); // cap=5, len=5
```

Create an `az_span` expression from an uninitialized byte buffer:

```C
uint8_t buffer[1024];
some_function(AZ_SPAN_FROM_BUFFER(buffer));  // cap=1024, len = 0
```

Create an `az_span` expression from an initialized bytes buffer: 

```C
uint8_t buffer[] = { 1, 2, 3, 4, 5 };
some_function(AZ_SPAN_FROM_INITIALIZED_BUFFER(buffer));  // cap=5, len = 5
```

Create an `az_span` literal from a string (the span does NOT include the 0-terminating byte):

```C
az_span span_over_str = AZ_SPAN_LITERAL_FROM_STR("Hello");  // cap=5, len = 5
```

Create an `az_span` expression from a string (the span does NOT include the 0-terminating byte): 

```C
some_function(AZ_SPAN_FROM_STR("Hello"));  // cap=5, len = 5
```

As shown above, an `az_span` over a string does not include the 0-terminator. If you need to 0-terminate the string, you can call this function to append a 0 byte (if the string’s length is less than its capacity):

```C
az_result az_span_append_uint8(az_span span, uint8_t c, az_span* out_span);
```

and then call this function to get the address of the 0-terminated string:

```C
char * str = (char*) az_span_ptr(span); // str points to a 0-terminated string
```

Or, you can call this function to copy the string in the `az_span` to your own `char*` buffer; this function will 0-termiante the string in the `char*` buffer: 

```C
az_result az_span_to_str(char* s, int32_t max_size, az_span span);
```

There are many functions to manipulate `az_span` instances. You can slice (subset an `az_span`), parse an `az_span` containing a string into an number, append a number as a string to the end of an `az_span`, check if two `az_span` instances are equal or the contents of two `az_span` instances are equal, and more. 

## Logging SDK Operations

As our SDK performs operations, it can send log messages to a customer-defined callback. Customers can enable this to assist with debugging and diagnosing issues when leveraging our SDK code.

To enable logging, you must first write a callback function that our logging mechanism will invoke periodically with messages. The function signature must match this type definition (defined in the [az_log.h](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/core/core/inc/az_log.h) file):

   ```C
   typedef void (*az_log_fn)(az_log_classification classification, az_span message);
   ```

And then, during your application’s initialization, you must register your function with our SDK by calling this function:

   ```C
   void az_log_set_listener(az_log_fn listener); 
   ```

Now, whenever our SDK wants to send a log message, it will invoke your callback function passing it the log classification and an `az_span` containing the message string (not 0-terminated). Your callback method can now do whatever it wants to with this message such as append it to a file or write it to the console. Note: in a multithreaded application, multiple threads may invoke this callback function simultaneously; if your function requires any kind of thread synchronization, then you must provide it. 

Log classifications allows your application to select which specific log messages it wants to receive. For example, to log just HTTP response messages (and not HTTP request messages), initialize your application by calling this:

   ```C
   az_log_classification const classifications[] = { AZ_LOG_HTTP_REQUEST };
   az_log_set_classifications(classifications, 
   sizeof(classifications) / sizeof(classifications[0]));
   ```

## Cancelling an Operation

Azure Core provides a rich cancellation mechanism by way of its `az_context` type (defined in the [az_context.h](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/core/core/inc/az_context.h) file). As your code executes and functions call other functions, a pointer to an `az_context` is passed as an argument through the functions. At any point, a function can create a new `az_context` specifying a parent `az_context` and a timeout period and then, this new `az_context` is passed down to more functions. When a parent `az_context` instance expired or is canceled, all of its children are cancelled as well.

There is a special singleton instance of the `az_context` type called `az_context_app`. This instance represents your entire application and this `az_context` instance never expires. It is common to use this instance as the ultimate root of all `az_context` instances. So then, as functions call other functions, these functions can create child `az_context` instances and pass the child down through the call tree. Imagine you have the following az_context tree:

- `az_context_app`; never expires
    - `az_context_child`; expires in 10 seconds
      -	`az_context_grandchild`; expires in 60 seconds

Any code using `az_context_grandchild` expires in 10 seconds (not 60 seconds) because it has a parent that expires in 10 seconds. In other words, each child can specify its own expiration time but when a parent expires, all its children also expire. While `az_context_app` never expires, your code can explicitly cancel it thereby cancelling all of the children `az_context` instances. This is a great way to cleanly cancel all operations in your application allowing it to terminate quickly.

Note however that cancellation is performed as a best effort; it is not guaranteed to work in a timely fashion. For example, the HTTP stack that you use may not support cancellation. In this case, cancellation will be detected only after the I/O operation completes or before the next I/O operation starts.

   ```C
   // Some function creates a child with a 10-second expiration:
   az_context child = az_context_with_expiration(&az_context_app, 10);

   // Some function creates a grandchild with a 60-second expiration:
   az_context grandchild = az_context_with_expiration(&child, 60);

   // Some other function (perhaps in response to a SIGINT) cancels the application root:
   az_context_cancel(&az_context_app); 
   // All children are now in the canceled state & the their threads will start unwinding
   ```

## Porting the AzureSDK to Another Platform

The Azure Core library requires you to implement a few functions to provide platform-specific features such as a clock, a thread sleep a mutual-exclusive thread synchronization lock, and an HTTP stack. By default, Azure Core ships with no-op versions of these functions all of which return AZ_RESULT_NOT_IMPLEMENTED. 

  Ahson: Link to PORTING.md 

### Community

* Chat with other community members [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting Security Issues and Security Bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure SDK for C is licensed under the [MIT](LICENSE) license.

<!-- LINKS -->
[azure_sdk_for_c_wiki]: https://github.com/azure/azure-sdk-for-c/wiki
---

### Windows
Follow next steps to build project from command prompt
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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c witch includes static libraries only. See below section [Compiler Options](#compiler-options)


### Linux / Mac
VCPKG can also be used in Linux to avoid installing libraries globally. Follow instructions [here](#vcpkg) to use VCPKG in Linux.

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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c witch includes static libraries only. See below section [Compiler Options](#compiler-options)


### Compiler Options
By default, when building project with no options, next static libraries are generated
- ``Libraries``:
  - az_core
  - az_iot
  - az_keyvault
  - az_storage_blobs

- ``Platform Abstraction Layer``: Default empty implementation for platform functions like time and HTTP stack. This default implementation is used to compile only but will return AZ_ERROR_NOT_IMPLEMENTED when run.
  - az_noplatform
  - az_nohttp
  - az_posix (on Linux/Mac)
  - az_win32 (on Windows)

- ``Samples``: By default, samples are built using the default Plaform implmentation (see [running samples](#running-samples)). This means that running samples would produce errors like:

---

```bash
./keys_client_example
Running sample with no_op HTTP implementation.
Recompile az_core with an HTTP client implementation like CURL to see sample sending network requests.

i.e. cmake -DBUILD_CURL_TRANSPORT=ON ..
```
  - keys_client_example
  - blobs_client_example

When running cmake, next options can be used to change the output libraries/Pal/Samples:

- `BUILD_CURL_TRANSPORT`: This option would build an HTTP transport library using CURL. It requires libcurl to be installed (vcpkg or globally). This option will make samples to be linked with this HTTP and be functional to send HTTP requests<br>
use it as

```bash
cmake -DBUILD_CURL_TRANSPORT ..
cmake --build .
```

---

Ahson: But, this file, mention contributing and link to the CONTRIBUTING.md

## Contributing to the Azure SDK -> CONTRIBUTING.md
Some intro paragraph & some boilerplate stuff (see below)

## Documentation, Tests, Samples, and Code Coverage

If submitting a platform code, all our existing tests and samples must run before you submit a PR.

If submitting a change to Azure Core or some other client library, then all our existing tests and sample must run and you must submit tests for any new functionality included in your PR. Include code coverage

- `UNIT_TESTING`: This option requires cmocka to be installed and it will generate unit tests for each project.<br>
use it as

```bash
cmake -DUNIT_TESTING ..
cmake --build .
# ctest will call and run tests
# -V runs tests in verbose mode to show more info about tests
ctest -V
```

## Running Test and Samples
### Unit test
See [compiler options](#compiler-options) to learn about how to build and run unit tests.

### Running samples
See [compiler options](#compiler-options) to learn about how to build samples with HTTP implementation in order to be runnable.

After building samples with HTTP stack, set next environment variables to set log in credentials. Samples will read this values from env an use it to log in to Azure Service like Storage or KeyVault. Learn about the supported authentication [client secret](https://docs.microsoft.com/en-us/azure/active-directory/azuread-dev/v1-oauth2-on-behalf-of-flow#service-to-service-access-token-request).

```bash
# On linux, set env var like this. For Windows, do it from advanced settings/ env variables

# replace question marks for your id
export tenant_id=????????-????-????-????-????????????
export client_id=????????-????-????-????-????????????
export client_secret=????????????
# set uri depending on Azure Service
export test_uri=https://????.????.azure.net
```

## Build Docs
Running below command from root folder will create a new folder `docs` containing html file with documentation about CORE headers.
> doxygen needs to be installed in the system

```bash
doxygen Doxyfile
```

## Code Coverage Reports
Code coverage reports can be generated after running unit tests for each project. Follow below instructions will generate code coverage reports.

### Requirements
- <b>gcc</b>. clang/MSVC not supported<br>
- <b>Debug</b>. Build files for debug `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- <b>cmocka / Unit Test Enabled</b>. Build cmocka unit tests `cmake --DUNIT_TESTING=ON ..`
- <b>environment variable</b>. `export AZ_SDK_CODE_COV=1`

```bash
# from source code root, create a new folder to build project:
mkdir build
cd build

# set env variable to enable building code coverage
export AZ_SDK_CODE_COV=1
# generate cmake files with Debug and cmocka unit tests enabled 
cmake -DUNIT_TESTING=ON -DCMAKE_BUILD_TYPE=Debug ..
# build
cmake --build .

## There are 3 available reports to generate for each project:
# 1. using lcov. Html files grouped by folders. Make sure lcov
# is installed.
make ${project_name}_cov //i.e. az_core_cov or az_iot_cov

# 2. using gcov. Html page with all results in one page. Make sure
# gcov is installed.
make ${project_name}_cov_html //i.e. az_core_cov_html or az_iot_cov_html

# 3. using gcov. XML file with all results. Make sure
# gcov is installed.
make ${project_name}_cov_xml //i.e. az_core_cov_xml or az_iot_cov_xml

## Code Coverage is available for this projects:
#  az_core
#  az_iot
#  az_keyvault
#  az_storage_blobs
```

## Contributing to the SDK
### Prerequisites

   * [cmocka](https://cmocka.org/): For building and running unit tests. By default building unit tests is disabled, so, unless you want to add unit tests or run it, you don't need to install this.


For details on contributing to this repository, see the [contributing guide](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Additional Helpful Links for Contributors  
Many people all over the world have helped make this project better.  You'll want to check out:

* [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
* [How to build and test your change](CONTRIBUTING.md#developer-guide)
* [How you can make a change happen!](CONTRIBUTING.md#pull-requests)
* Frequently Asked Questions (FAQ) and Conceptual Topics in the detailed [Azure SDK for C wiki][azure_sdk_for_c_wiki].


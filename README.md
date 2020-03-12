# Azure SDK for Embedded C

[![Build Status](https://dev.azure.com/azure-sdk/public/_apis/build/status/c/c%20-%20client%20-%20ci?branchName=master)](https://dev.azure.com/azure-sdk/public/_build/latest?definitionId=722&branchName=master)

This repository contains official Embedded C libraries for Azure services.

## Getting started

To get started with a specific library, see the **README.md** file located in the library's project folder. You can find service libraries in the `/sdk` directory.

### Prerequisites
- CMake version 3.10 is required to use these libraries.
- C compiler. MSVC, gcc or clang are recommended.
- cmocka. For building and running unit tests. By default building unit tests is disabled, so, unless you want to add unit test or run it, you don't need to install this. See below how vcpkg can be used to install dependencies
- curl. Curl is used a http stack and it is required for building and running service samples (keyvault and storage). You don't need to install curl if not building samples.

### Development Environment
Project contains files to work on Windows, Mac or Linux based OS.

### VCPKG
vcpkg is the easiest way to have dependencies installed. It downloads packages sources, headers and build libraries for whatever TRIPLET is set up (platform/arq).
VCPKG maintains any installed package inside its own folder, allowing to have multiple vcpkg folder with different dependencies installed on each. This is also great because you don't have to install dependencies globally on your system.

Follow next steps to install VCPKG and have it linked to cmake
```bash
# Clone vcpgk:
git clone https://github.com/Microsoft/vcpkg.git
# (consider this path as PATH_TO_VCPKG)
cd vcpkg
# build vcpkg (remove .bat on Linux/Mac)
.\bootstrap-vcpkg.bat
# install dependencies (remove .exe in Linux/Mac) and update triplet
vcpkg.exe install --triplet x64-windows-static curl[winssl] cmocka
# Add environment variables:
# VCPKG_DEFAULT_TRIPLET=x64-windows-static
# VCPKG_ROOT=[PATH_TO_VCPKG] (replace PATH_TO_VCPKG for where vcpkg is installed)
```

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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See below section [Compiler Options](#compiler-options)


#### Visual Studio 2019
Open project folder with Visual Studio. If VCPKG has been previously installed and set up like mentioned [above](#VCPKG). Everything will be ready to build.
Right after opening project, Visual Studio will read cmake files and generate cache files automatically.

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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See below section [Compiler Options](#compiler-options)


### Compiler Options
By default, when building project with no options, next static libraries are generated
- ``Libraries``:
  - az_core
  - az_iot
  - az_keyvault
  - az_storage_blobs
- ``Platform Abstraction Layer``: Default empty implementation for platform functions like time and http stack. This default implementation is used to compile only but will return ERROR NOT IMPLEMENTED when running it.
  - az_noplatform
  - az_nohttp
  - az_posix (on Lin/Mac)
  - az_win32 (on Windows)
- ``Samples``: By default, samples are built using the default PAL (see [running samples section](#running-samples)). This means that running samples would throw errors like:
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
See [compiler options section](#compiler-options) to learn about how to build and run unit tests.

### Running samples
See [compiler options section](#compiler-options) to learn about how to build samples with HTTP implementation in order to be runnable.

After building samples with HTTP stack, set next environment variables to set log in credentials. Samples will read this values from env an use it to log in to Azure Service like Storage or KeyVault. Learn about the supported authentication [client secret here](https://docs.microsoft.com/en-us/azure/active-directory/azuread-dev/v1-oauth2-on-behalf-of-flow#service-to-service-access-token-request).
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

## Need help?
* File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
* Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using
  the `azure` and `c` tags.

## Navigating the repository

### Master branch

The master branch has the most recent code with new features and bug fixes. It does **not** represent latest released **GA** SDK.

### Release branches (Release tagging)

For each package we release there will be a unique git tag created that contains the name and the version of the package to mark the commit of the code that produced the package. This tag will be used for servicing via hotfix branches as well as debugging the code for a particular preview or stable release version.
Format of the release tags are `<package-name>_<package-version>`. For more information please see [our branching strategy](https://github.com/Azure/azure-sdk/blob/master/docs/policies/repobranching.md#release-tagging).

## Contributing
For details on contributing to this repository, see the [contributing guide](https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License
Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For
details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate
the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to
do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.


### Additional Helpful Links for Contributors  
Many people all over the world have helped make this project better.  You'll want to check out:

* [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
* [How to build and test your change](CONTRIBUTING.md#developer-guide)
* [How you can make a change happen!](CONTRIBUTING.md#pull-requests)
* Frequently Asked Questions (FAQ) and Conceptual Topics in the detailed [Azure SDK for C wiki][azure_sdk_for_c_wiki].

### Community

* Chat with other community members [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting security issues and security bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure SDK for C is licensed under the [MIT](LICENSE) license.

<!-- LINKS -->
[azure_sdk_for_c_wiki]: https://github.com/azure/azure-sdk-for-c/wiki

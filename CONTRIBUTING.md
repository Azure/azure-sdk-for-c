# Azure SDK for C Contributing Guide

Thank you for your interest in contributing to Azure SDK for C.

- For reporting bugs, requesting features, or asking for support, please file an issue in the [issues](https://github.com/Azure/azure-sdk-for-c/issues) section of the project.

- If you would like to become an active contributor to this project please follow the instructions provided in [Microsoft Azure Projects Contribution Guidelines](http://azure.github.com/guidelines.html).

- To make code changes, or contribute something new, please follow the [GitHub Forks / Pull requests model](https://help.github.com/articles/fork-a-repo/): Fork the repo, make the change and propose it back by submitting a pull request.

- Refer to the [wiki](https://github.com/Azure/azure-sdk-for-c/wiki) to learn about how Azure SDK for C generates lint checker, doxygen, and code coverage reports.

## Pull Requests

- **DO** submit all code changes via pull requests (PRs) rather than through a direct commit. PRs will be reviewed and potentially merged by the repo maintainers after a peer review that includes at least one maintainer.
- **DO NOT** submit "work in progress" PRs.  A PR should only be submitted when it is considered ready for review and subsequent merging by the contributor.
- **DO** give PRs short-but-descriptive names (e.g. "Improve code coverage for Azure.Core by 10%", not "Fix #1234")
- **DO** refer to any relevant issues and include [keywords](https://help.github.com/articles/closing-issues-via-commit-messages/) that automatically close issues when the PR is merged.
- **DO** tag any users that should know about and/or review the change.
- **DO** ensure each commit successfully builds.  The entire PR must pass all tests in the Continuous Integration (CI) system before it'll be merged.
- **DO** address PR feedback in an additional commit(s) rather than amending the existing commits, and only rebase/squash them when necessary.  This makes it easier for reviewers to track changes.
- **DO** assume that ["Squash and Merge"](https://github.com/blog/2141-squash-your-commits) will be used to merge your commit unless you request otherwise in the PR.
- **DO NOT** fix merge conflicts using a merge commit. Prefer `git rebase`.
- **DO NOT** mix independent, unrelated changes in one PR. Separate real product/test code changes from larger code formatting/dead code removal changes. Separate unrelated fixes into separate PRs, especially if they are in different assemblies.

### Merging Pull Requests (for project contributors with write access)

- **DO** use ["Squash and Merge"](https://github.com/blog/2141-squash-your-commits) by default for individual contributions unless requested by the PR author.
  Do so, even if the PR contains only one commit. It creates a simpler history than "Create a Merge Commit".
  Reasons that PR authors may request "Merge and Commit" may include (but are not limited to):

  - The change is easier to understand as a series of focused commits. Each commit in the series must be buildable so as not to break `git bisect`.
  - Contributor is using an e-mail address other than the primary GitHub address and wants that preserved in the history. Contributor must be willing to squash
    the commits manually before acceptance.

## Developer Guide

### Prerequisites

- [CMake](https://cmake.org/download/) version 3.10 or later
- C compiler: [MSVC](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019), [gcc](https://gcc.gnu.org/) or [clang](https://clang.llvm.org/) are recommended
- [git](https://git-scm.com/downloads) to clone our Azure SDK repository with the desired tag
- [cmocka](https://cmocka.org/) for building and running unit tests. By default, building unit tests is disabled, so, unless you want to add unit tests or run then, you don't need to install this. See how `vcpkg` can be used to install dependencies, [below](#VCPKG).
- [libcurl](https://curl.haxx.se/download.html) which is used as an http stack, and is required for building and running service samples (keyvault and storage). You don't need to install libcurl if you are not building samples. The minimum required version of libcurl is 7.1.
See how `vcpkg` can be used to install dependencies, [below](#VCPKG).
- [doxygen](http://www.doxygen.nl/download.html) if you need to generate and view documentation.

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
# Clone vcpgk:
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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See below section [Compiler Options](#compiler-options)

#### Visual Studio 2019

Open project folder with Visual Studio. If VCPKG has been previously installed and set up like mentioned [above](#VCPKG). Everything will be ready to build.
Right after opening project, Visual Studio will read cmake files and generate cache files automatically.

### Linux

#### VCPKG

VCPKG can be used to download packages sources, headers and build libraries for whatever TRIPLET is set up (platform/architecture).
VCPKG maintains any installed package inside its own folder, allowing to have multiple vcpkg folder with different dependencies installed on each. This is also great because you don't have to install dependencies globally on your system.

Follow next steps to install VCPKG and have it linked to cmake

```bash
# Clone vcpgk:
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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See below section [Compiler Options](#compiler-options)

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
# Clone vcpgk:
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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c which includes static libraries only. See below section [Compiler Options](#compiler-options)

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
<td>UNIT_TESTING_MOCK_ENABLED</td>
<td>This option works only with GCC. It uses -ld option from linker to mock functions during unit test. This is used to test platform or HTTP functions by mocking the return values.</td>
<td>OFF</td>
</tr>
<tr>
<td>BUILD_PRECONDITIONS</td>
<td>Turning this option ON would remove all method contracts. This us typically for shipping libraries for production to make it as much optimized as possible.</td>
<td>ON</td>
</tr>
<tr>
<td>BUILD_CURL_TRANSPORT</td>
<td>This option requires Libcurl dependency to be available. It generates an HTTP stack with libcurl for az_http to be able to send requests thru the wire. This library would replace the no_http.</td>
<td>OFF</td>
</tr>
<tr>
<td>BUILD_PAHO_TRANSPORT</td>
<td>This option requires paho-mqtt dependency to be available. Provides Paho MQTT support for iot.</td>
<td>OFF</td>
</tr>
<tr>
<td>AZ_PLATFORM_IMPL</td>
<td>This option can be set to any of the next values:<br>- No_value: default value is used and no_platform library is used.<br>- "POSIX": Provides implementation for Linux and Mac systems.<br>- "WIN32": Provides platform implementation for Windows based system<br>- "USER": Tells cmake to use an specific implementation provided by user. When setting this option, user must provide an implementation library and set option `AZ_USER_PLATFORM_IMPL_NAME` with the name of the library (i.e. <code>-DAZ_PLATFORM_IMPL=USER -DAZ_USER_PLATFORM_IMPL_NAME=user_platform_lib</code>). cmake will look for this library to link az_core</td>
<td>No_value</td>
</tr>
</table>


- ``Samples``: Whenever UNIT_TESTING is ON, samples are built using the default PAL (see [running samples section](#running-samples)). This means that running samples would throw errors like:

```bash
./keys_client_example
Running sample with no_op HTTP implementation.
Recompile az_core with an HTTP client implementation like CURL to see sample sending network requests.

i.e. cmake -DBUILD_CURL_TRANSPORT=ON ..
```

## Running Tests and Samples

### Unit tests

See [compiler options section](#compiler-options) to learn about how to build and run unit tests.

After compiling project with unit test enabled, run tests with:
```bash
cmake -DUNIT_TESTING=ON ..
cmake --build .
# ctest will call and run tests
# -V runs tests in verbose mode to show more info about tests
ctest -V
```


### Test with mocked functions

Some test uses linker option ld to wrap functions and mock the implementation for it to do unit testing. Specially for PAL-related functions, mocking functions becomes a convenient way to break dependency between functions.

In order to run this tests, GCC is required (or any compiler that supports -ld linker flag).

To enable building project and linking with this option, as well as adding tests using mocked functions, add option `-DUNIT_TESTING_MOCK_ENABLED=ON` next to `-DUNIT_TESTING=ON` to cmake cache generation (see below example)

```cmake
cmake -DUNIT_TESTING=ON -DUNIT_TESTING_MOCK_ENABLED=ON ..
```

### Running samples

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

## Build Docs

Running below command from root folder will create a new folder `docs` containing html file with documentation about CORE headers. Make sure you have `doxygen` installed on the system.

```bash
doxygen Doxyfile
```

## Code Coverage Reports

Code coverage reports can be generated after running unit tests for each project. Follow below instructions will generate code coverage reports.

### Requirements

- **gcc** - clang/MSVC are not supported
- **Debug** - Build files for debug `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- **cmocka / Unit Test Enabled** - Build cmocka unit tests `cmake --DUNIT_TESTING=ON ..`
- **environment variable** - `set AZ_SDK_CODE_COV=1`

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

## Code Coverage is available for these projects:
#  az_core
#  az_iot
#  az_keyvault
#  az_storage_blobs

> Note: If `make` fails with "project not found" it's likely you are not using `gcc`. Use `sudo update-alternatives --config c+++` and `sudo update-alternatives --config cc` to switch to gcc.
```

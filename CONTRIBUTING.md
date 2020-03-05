Azure SDK for C Contributing Guide
-------------------------------------

Thank you for your interest in contributing to Azure SDK for C.

- For reporting bugs, requesting features, or asking for support, please file an issue in the [issues](https://github.com/Azure/azure-sdk-for-c/issues) section of the project.

- If you would like to become an active contributor to this project please follow the instructions provided in [Microsoft Azure Projects Contribution Guidelines](http://azure.github.com/guidelines.html).

- To make code changes, or contribute something new, please follow the [GitHub Forks / Pull requests model](https://help.github.com/articles/fork-a-repo/): Fork the repo, make the change and propose it back by submitting a pull request.

- Refer to the [wiki](https://github.com/Azure/azure-sdk-for-c/wiki) to learn about how Azure SDK for C generates lint checker, doxygen, and code coverage reports.

Pull Requests
-------------

* **DO** submit all code changes via pull requests (PRs) rather than through a direct commit. PRs will be reviewed and potentially merged by the repo maintainers after a peer review that includes at least one maintainer.
* **DO NOT** submit "work in progress" PRs.  A PR should only be submitted when it is considered ready for review and subsequent merging by the contributor.
* **DO** give PRs short-but-descriptive names (e.g. "Improve code coverage for Azure.Core by 10%", not "Fix #1234")
* **DO** refer to any relevant issues, and include [keywords](https://help.github.com/articles/closing-issues-via-commit-messages/) that automatically close issues when the PR is merged.
* **DO** tag any users that should know about and/or review the change.
* **DO** ensure each commit successfully builds.  The entire PR must pass all tests in the Continuous Integration (CI) system before it'll be merged.
* **DO** address PR feedback in an additional commit(s) rather than amending the existing commits, and only rebase/squash them when necessary.  This makes it easier for reviewers to track changes.
* **DO** assume that ["Squash and Merge"](https://github.com/blog/2141-squash-your-commits) will be used to merge your commit unless you request otherwise in the PR.
* **DO NOT** fix merge conflicts using a merge commit. Prefer `git rebase`.
* **DO NOT** mix independent, unrelated changes in one PR. Separate real product/test code changes from larger code formatting/dead code removal changes. Separate unrelated fixes into separate PRs, especially if they are in different assemblies.

Merging Pull Requests (for project contributors with write access)
----------------------------------------------------------

* **DO** use ["Squash and Merge"](https://github.com/blog/2141-squash-your-commits) by default for individual contributions unless requested by the PR author.
  Do so, even if the PR contains only one commit. It creates a simpler history than "Create a Merge Commit".
  Reasons that PR authors may request "Merge and Commit" may include (but are not limited to):

  - The change is easier to understand as a series of focused commits. Each commit in the series must be buildable so as not to break `git bisect`.
  - Contributor is using an e-mail address other than the primary GitHub address and wants that preserved in the history. Contributor must be willing to squash
    the commits manually before acceptance.



## Developer Guide

### Prerequisites
- CMake version 3.12 is required to use these libraries.
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
# Add this environment variables to link this VCPKG folder with cmake:
# VCPKG_DEFAULT_TRIPLET=x64-windows-static
# VCPKG_ROOT=[PATH_TO_VCPKG] (replace PATH_TO_VCPKG for where vcpkg is installed)
```

> Note: On macOS, `.\bootstrap-vcpkg` may fail if your version of the C++ toolchain is not new enough to support vcpkg. To resolve
this, vcpkg recommends to install `gcc@6` from Homebrew (`brew install gcc@6`), then re-run the bootstrapping script.


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

> Note: The steps above would compile and generate the default output for azure-sdk-for-c witch includes static libraries only. See below section [Compiler Options](#compiler-options)


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

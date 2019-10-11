# Azure SDK for Embedded C

[![Build Status](https://dev.azure.com/azure-sdk/public/_apis/build/status/c/c%20-%20client%20-%20ci?branchName=master)](https://dev.azure.com/azure-sdk/public/_build/latest?definitionId=722&branchName=master)

This repository contains official Embedded C libraries for Azure services.

## Getting started

To get started with a specific library, see the **README.md** file located in the library's project folder. You can find service libraries in the `/sdk` directory.

### Prerequisites
- CMake version 3.12 is required to use these libraries.

### Development Environment
Project contains files to work on Windows or Linux based OS.

> Note: Project contains git submodules required to build. Before building run `git submodule update --init --recursive`

#### Windows
Use PowerShell to run {projectDir}/build.ps1

> Note: Cmake will look for CURL within windows. Follow next steps to set up CURL using VCPKG:
<br> - Clone vcpgk: `git clone https://github.com/Microsoft/vcpkg.git`
<br> - cd vcpkg (consider this path as PATH_TO_VCPKG)
<br> - .\bootstrap-vcpkg.bat
<br> - vcpkg.exe install --triplet x64-windows-static curl[winssl]
<br> - Add windows system variable: VCPKG_DEFAULT_TRIPLET=x64-windows-static
<br> - Add windows system variable: VCPKG_ROOT=[PATH_TO_VCPKG] (replace PATH_TO_VCPKG for where vcpkg is installed)

##### Visual Studio 2019
When following previous steps to set up CURL with VCPKG, open project forlder with Visual Studio and everything will be ready to build and run tests.

#### Linux
- Install `openssl version 1.1.1`
  - For ubuntu: ```sudo apt-get install libssl-dev```
- Install `uuid`
  - ```sudo apt-get install uuid-dev```

##### steps to build
```
mkdir build
build
cmake ../
make
```
> Note: use `cmake -Duse_default_uuid=ON ../` if no uuid-dev is installed in system

## Need help?
* File an issue via [Github Issues](https://github.com/Azure/azure-sdk-for-c/issues/new/choose).
* Check [previous questions](https://stackoverflow.com/questions/tagged/azure+c) or ask new ones on StackOverflow using `azure` and `c` tags.

## Contributing
For details on contributing to this repository, see the [contributing guide](CONTRIBUTING.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

![Impressions](https://azure-sdk-impressions.azurewebsites.net/api/impressions/azure-sdk-for-c%2FREADME.png)

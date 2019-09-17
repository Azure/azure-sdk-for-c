# Azure IoT uStorageClient

[![Build Status](https://azure-iot-sdks.visualstudio.com/azure-iot-stuben/_apis/build/status/azure-iot-ulib/azure-iot-ulib-build?branchName=master)](https://azure-iot-sdks.visualstudio.com/azure-iot-stuben/_build/latest?definitionId=176&branchName=master)

The intention of this repository is to provide developers useful tools to make developing for small, limited-resource devices easier.

## Reference

Documentation for API's are automatically generated with [doxygen](http://www.doxygen.nl/). You can find the documentation in the [docs](TODO: create doxygen documentation) directory.

## Development Machine Requirements

This repo is built using cmake. The minimum requirement is version 3.2. You can download and install the latest cmake version [here](https://cmake.org/).

## Building the Repo

1. Clone the repo into the directory of your choosing with the following command

    ```bash
    git clone --recursive https://azure-iot-sdks.visualstudio.com/azure-iot-stuben/_git/azure-iot-ustorageclient
    ```

2. From here you can either use build scripts we have provided in `build_all\` or elect to build by yourself using the commands detailed in the following Command Line section.

### Scripts

1. Find the script for the OS on your dev machine and run it. By default, the tests will be compiled and then run at the end of the script.

### Command Line

1. Create the cmake directory to put all of your build files:

    ```bash
    mkdir cmake
    ```

2. cd into the cmake directory:

    ```bash
    cd cmake
    ```

3. Run cmake. Here you have the option of building the tests and/or DISABLING the samples as well.

    If you would like to build the tests, run:

    ```bash
    cmake .. -Drun_azstorage_unit_tests:BOOL=ON
    ```

    If you would like to disable the samples (prevent them from building to save space), run:

     ```bash
    cmake .. -Dskip_azstorage_samples:BOOL=ON
    ```

    Otherwise, to build the source files AND samples, run:

    ```bash
    cmake ..
    ```

4. Build with cmake

    ```bash
    cmake --build .
    ```

5. If you would like to run the tests and added the option from step 3, you can run the following:

    ```bash
    ctest -C "debug" -V
    ```

    The -C option chooses the build configuration to test and the -V turns on verbose output from the tests.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit [Microsoft CLA](https://cla.microsoft.com).

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

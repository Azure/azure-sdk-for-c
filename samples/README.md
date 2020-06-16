# Samples, Snippets, and How-To Guides

Developers like to learn by looking at code, and so the Azure SDK comes with a myriad of code samples in the form of short code snippets, sample applications, and how-to guides. This document describes where to find all these resources.

## Structure of the Repository
The Azure SDK repository is organized in the following folder structure, with the main sample locations highlighted using **bold** font.

`/samples` (this folder)<br>
&nbsp;&nbsp;&nbsp;&nbsp;`README.md` (this file)<br>
`/sdk` (folder containing sources, samples, test for all SDK packages)<br>
&nbsp;&nbsp;&nbsp;&nbsp;`/<samples>`<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`/<service>` (e.g. storage)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**`/<package>`** (e.g. blobs)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**`README.md`** (package READMEs contain hello world samples)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**`src`** (source files for samples)<br>

##  Getting Started Samples
Each package folder contains a package-specific `README.md` file. Most of these `README` files contain `Hello World` code samples illustrating basic usage of the the APIs contained in the package. For example, you can find `Hello World` samples for the `azure-storage-blobs` package [here](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/storage#examples).

## Package Samples and How-To Guides
Under the `/sdk/samples` directory, each service has a set of subfolders with samples for each package. These samples can either be short programs contained in `*.c` files, or more complete how-to guides (code samples and some commentary) contained in `*.md` files. You can find shortcuts to the main how-to guides in the [How-To Guides List](#how-to-guide-list) section below.

## How-To Guide List
This section lists how-to guides for the most commonly used APIs and most common scenarios, i.e. this section does not attempt to be a complete directory of guides contained in this repository. 

#### General How-To Guides
- [How to port the Azure SDK to another platform](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/core#porting-the-azure-sdk-to-another-platform)
- [How to configure, access, and analyze logging information](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/docs/core#logging-sdk-operations)

#### Azure.IoT
- [IoT Hub samples](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/iot/hub#getting-started)
- [IoT Provisioning samples](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/iot/provisioning#getting-started)


#### Azure.Storage.Blobs
- [How to Upload a blob](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/samples/storage/blobs)

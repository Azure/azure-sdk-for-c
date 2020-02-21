# Samples, Snippets, and How-To Guides

Developers like to learn by looking at code, and so the Azure SDK comes with a myriad of code samples in the form of short code snippets, sample applications, and how-to guides. This document describes where to find all these resources.

## Structure of the Repository
The Azure SDK repository is organized in the following folder structure, with the main sample locations highlighted using **bold** font.

`/samples` (this folder)<br>
&nbsp;&nbsp;&nbsp;&nbsp;`README.md` (this file)<br>
`/sdk` (folder containing sources, samples, test for all SDK packages)<br>
&nbsp;&nbsp;&nbsp;&nbsp;`/<service>` (e.g. storage)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`/<package>` (e.g. blobs)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**`README.md`** (package READMEs contain hello world samples)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**`/samples`** (package-specific samples)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`/inc` (header files)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`/src` (implementation)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`/test`<br>

##  Getting Started (a.k.a. `Hello World`) Samples
Each package folder contains a package-specific `README.md` file. Most of these `README` files contain `Hello World` code samples illustrating basic usage of the the APIs contained in the package. For example, you can find `Hello World` samples for the `azure-storage-blobs` package [here](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/storage/blobs/README.md#examples).

## Package Samples and How-To Guides
Each package folder contains a subfolder called `/samples` with additional code samples. These samples can be either short programs contained in `*.c` files, or more complete how-to guides (code samples and some commentary) contained in `*.md` files. You can find shortcuts to main how-to guides in the [**How-To Guides List**](#how-to-guide-list) section below.

## Sample Applications
Sometimes we want to illustrate how several APIs or even packages work together in a context of a more complete program. For these cases, we created sample applications that you can look at, download, compile, and execute. These application samples are located on 
[https://docs.microsoft.com/samples/](https://docs.microsoft.com/samples/).

## How-To Guide List
This section lists how-to guides for the most commonly used APIs and most common scenarios, i.e. this section does not attempt to be a complete directory of guides contained in this repository. 

#### General How-To Guides
- [How to create **KeyVault Http Client**](https://github.com/Azure/azure-sdk-for-c/tree/master/sdk/core/core#examples)
- How to configure, access, and analyze **logging** information (TODO)

#### Azure.Storage.Blobs
- [How to **Upload** blob](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/storage/blobs/samples/)

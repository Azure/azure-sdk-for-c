# Azure SDK Platform

Azure SDK Platform contains two important components. The first component is related to HTTP and the second one is implementation for system specific features that Azure Core uses (like thread sleep).

## HTTP Adapter

Azure SDK has its own HTTP Request and Response structures. For this reason, an HTTP adapter is required to be implemented to use Azure SDK HTTP Request and Response as input for any HTTP Stack library (like libcurl or win32).

Azure SDK provides one implementation for libcurl (`az_curl`). To consume this implementation, link your application against az_core and az_curl (if using Cmake). Http Request will be sent by using libcurl library.

>Note: See [Compiler Options](https://github.com/Azure/azure-sdk-for-c#compiler-options). You have to turn on building curl transport in order to have this adapter available.

Azure SDK will aso build and provide an empty HTTP adapter that is called `az_nohttp`. This target allows you to build az_core lib without any specific HTTP adapter. Use this option when you won't use HTTP API from Azure SDK.

>Note: An `AZ_ERROR_NOT_IMPLEMENTED` will be returned from all HTTP APIs from Azure SDK when building with `az_nohttp`.

You can also implement your own HTTP adapter and use it. This option is in case you want to use an HTTP stack that is not libcurl. Follow the instructions on [use your own http stack implementation](https://github.com/Azure/azure-sdk-for-c#using-your-own-http-stack-implementation).


## Platform

Azure SDK Core depends on some system-specific functions. This functions are not part of the C99 standard library and its implementation depends on system architecture (like a clock, thead sleep or interlock).

Azure SDK provides three platform implementations for you, one for Windows (`az_win32`), another for Linux and MacOS (`az_posix`) and an empty implementation (`az_noplatform`).

See [compiler options](https://github.com/Azure/azure-sdk-for-c#compiler-options) to learn about how to build and use each of this available targets or how to use your own platform implementation.

>Note: `az_noplatform` can be used to link your application and enable Azure Core to run. However, this is not recommended for a production application. It is suggested to be used for testing or Demo.



## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md

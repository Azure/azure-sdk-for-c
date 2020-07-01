# Azure SDK Platform

The Azure SDK platform provides two abstractions with some built-in implementations that the caller can use or override with their own behavior. The first one is a transport adapter with helpers to allow clients to communicate with Azure. The second one is operating system specific implementations for the small set of features that the Azure SDK needs that are not available as part of the C99 standard library (such as thread sleep).

## HTTP Transport Adapter

The Azure SDK has its own HTTP request and response structures. For this reason, an HTTP transport adapter is required to be implemented to use the Azure SDK HTTP request and response as input for any HTTP stack (like libcurl or win32). The library allows clients to communicate with Azure.

Azure SDK provides one implementation for libcurl (`az_curl`). To consume this implementation, link your application against `az_core` and `az_curl` (if using Cmake) and then HTTP requests will be sent using `libcurl`.

>Note: See [Compiler Options](https://github.com/Azure/azure-sdk-for-c#compiler-options). You have to turn on building curl transport in order to have this adapter available.

The Azure SDK also provides empty HTTP adapter stubs called `az_nohttp`. This target allows you to build `az_core` without any specific HTTP adapter. Use this option when you won't use any HTTP specific APIs from the Azure SDK.

>Note: An `AZ_ERROR_NOT_IMPLEMENTED` will be returned from all HTTP APIs from the Azure SDK when building with `az_nohttp`.

You can also implement your own HTTP transport adapter and use it. This allows you to use a different HTTP stack other than `libcurl`. Follow the instructions on [using your own HTTP stack implementation](https://github.com/Azure/azure-sdk-for-c#using-your-own-http-stack-implementation).

## Platform

Azure SDK Core depends on some system-specific functions. These functions are not part of the C99 standard library and their implementation depends on system architecture (for example a clock, thread sleep, or interlock).

Azure SDK provides three platform implementations for you, one for Windows (`az_win32`), another for Linux and MacOS (`az_posix`) and an empty implementation (`az_noplatform`).

See [compiler options](https://github.com/Azure/azure-sdk-for-c#compiler-options) to learn about how to build and use each of these available targets or how to use your own platform implementation.

>Note: `az_noplatform` can be used to link your application and enable Azure Core to run. However, this is not recommended for a production application. It is suggested to be used for testing or just for getting started.

## Contributing

If you'd like to contribute to this library, please read the [contributing guide][azure_sdk_for_c_contributing] to learn more about how to build and test the code.

### License

Azure SDK for Embedded C is licensed under the [MIT][azure_sdk_for_c_license] license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: ../../../CONTRIBUTING.md
[azure_sdk_for_c_license]: https://github.com/Azure/azure-sdk-for-c/blob/master/LICENSE

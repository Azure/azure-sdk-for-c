# Azure SDK Core Library for C

Azure Core library for C (`az_core`) provides shared primitives, abstractions, and helpers for modern Azure SDK client libraries written in the C programming language. These libraries follow the Azure SDK Design Guidelines for Embedded C.

The library allows client libraries to expose common functionality in a consistent fashion.  Once you learn how to use these APIs in one client library, you will know how to use them in other client libraries.

## Getting started

TODO

## Key concepts

### Error Structure

Defined in [inc/az_result.h](inc/az_result.h).

```c
// az_core.h
enum {
  AZ_OK              =          0,
  AZ_ERROR_FLAG      = 0x80000000,

  AZ_CORE_FACILITY   =    0x10000,
};

typedef int32_t az_result;

#define AZ_MAKE_ERROR(facility, code) ((az_result)(0x80000000 | ((uint32_t)(facility) << 16)) | (uint32_t)(code))

#define AZ_MAKE_RESULT(facility, code) ((az_result)(((uint32_t)(facility) << 16)) | (uint32_t)(code))

inline bool az_failed(az_result result) {
  return (result & AZ_ERROR_FLAG) != 0;
}

inline bool az_succeeded(az_result result) {
  return (result & AZ_ERROR_FLAG) == 0;
}
```

Additional information could be passed using output parameters.

### Span of bytes

Defined in [inc/az_span.h](inc/az_span.h).

```c
typedef struct {
  struct {
    uint8_t * ptr;
    int32_t length;
    int32_t capacity;
  } _internal;
} az_span;
```

### Strings

A string is a span of UTF-8 characters. It's not a zero-terminated string. Defined in [inc/az_span.h](inc/az_span.h).

```c
az_span hello_world = AZ_SPAN_FROM_STR("Hello world!");
```

## Examples

### az_log.h
The various components of the SDK are broken up into "classifications". Log messages are filtered with these classification enums so that the user can decide which log messages they want. Classifications are derived from higher level groupings called "facilities". At the beginning of the user's code, they can initialize the logging by optionally setting the classifications they want, setting their logging listener, and then logging any messages they desire. 

*Basic Code Snippet*

Relevant components for logging are located in [az_result.h](./inc/az_result.h) and [az_log.h](./inc/az_log.h). The `az_log_classification` enum will be used to choose features to log.

```c
/* az_result.h */
enum
{
  AZ_FACILITY_CORE = 0x1,
  AZ_FACILITY_PLATFORM = 0x2,
  AZ_FACILITY_JSON = 0x3,
  AZ_FACILITY_HTTP = 0x4,
  AZ_FACILITY_MQTT = 0x5,
  AZ_FACILITY_IOT = 0x6,
  AZ_FACILITY_STD = 0x7FFF,
};

/* az_log.h */
typedef enum {
  AZ_LOG_HTTP_REQUEST  = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_HTTP, 1),
  AZ_LOG_HTTP_RESPONSE = _az_LOG_MAKE_CLASSIFICATION(AZ_FACILITY_HTTP, 2),
} az_log_classification;
```

Here is an example of what basic sdk and user code might look like working together.
The user needs to do two things as exemplified below:
1. Set the classifications you wish to log.
2. Set your logging function that follows the `az_log_fn` prototype. In this case, the logging function uses a basic `printf()`.

```c
/* INTERNAL sdk http code */
static az_span test_log_message = AZ_SPAN_LITERAL_FROM_STR("HTTP Request Success");

void some_http_request_code()
{
  /* Some http code */
  az_log_write(AZ_LOG_HTTP_REQUEST, test_log_message);
}


/* User Application Code */
az_log_classification const classifications[] = { AZ_LOG_HTTP_REQUEST, AZ_LOG_HTTP_RESPONSE };

void test_log_func(az_log_classification classification, az_span message)
{
    printf("%.*s", az_span_length(message), az_span_ptr(message));
}

int main()
{
  az_log_set_classifications(classifications, 
           sizeof(classifications)/sizeof(classifications[0]));
  az_log_set_listener(&test_log_func);

  some_http_request_code();
}
```

Here the classifications are set to `AZ_LOG_HTTP_REQUEST` and `AZ_LOG_RESPONSE`. Should the `AZ_LOG_HTTP_REQUEST` be omitted from the set of classifications, the log message in `some_http_request_code()` will not be logged.

If no classifications are set then all messages are logged.


## Troubleshooting

### General

TODO

### Retry policy

While working with Azure you might encounter transient failures. For information about handling these types of failures, see [Retry pattern][azure_pattern_retry] in the Cloud Design Patterns guide, and the related [Circuit Breaker pattern][azure_pattern_circuit_breaker].

## Next steps

### More sample code

TODO

### Additional documentation

TODO

## Contributing
For details on contributing to this repository, see the [contributing guide][azure_sdk_for_c_contributing].

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### Additional Helpful Links for Contributors  
Many people all over the world have helped make this project better.  You'll want to check out:

* [What are some good first issues for new contributors to the repo?](https://github.com/azure/azure-sdk-for-c/issues?q=is%3Aopen+is%3Aissue+label%3A%22up+for+grabs%22)
* [How to build and test your change][azure_sdk_for_c_contributing_developer_guide]
* [How you can make a change happen!][azure_sdk_for_c_contributing_pull_requests]
* Frequently Asked Questions (FAQ) and Conceptual Topics in the detailed [Azure SDK for C wiki](https://github.com/azure/azure-sdk-for-c/wiki).

### Community

* Chat with other community members [![Join the chat at https://gitter.im/azure/azure-sdk-for-c](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/azure/azure-sdk-for-c?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### Reporting security issues and security bugs

Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC) <secure@microsoft.com>. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://www.microsoft.com/msrc/faqs-report-an-issue).

### License

Azure SDK for C is licensed under the [MIT](LICENSE) license.

<!-- LINKS -->
[azure_sdk_for_c_contributing]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md
[azure_sdk_for_c_contributing_developer_guide]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#developer-guide
[azure_sdk_for_c_contributing_pull_requests]: https://github.com/Azure/azure-sdk-for-c/blob/master/CONTRIBUTING.md#pull-requests
[azure_cli]: https://docs.microsoft.com/cli/azure
[azure_pattern_circuit_breaker]: https://docs.microsoft.com/azure/architecture/patterns/circuit-breaker
[azure_pattern_retry]: https://docs.microsoft.com/azure/architecture/patterns/retry
[azure_portal]: https://portal.azure.com
[azure_sub]: https://azure.microsoft.com/free/
[c_compiler]: https://visualstudio.microsoft.com/vs/features/cplusplus/
[cloud_shell]: https://docs.microsoft.com/azure/cloud-shell/overview
[cloud_shell_bash]: https://shell.azure.com/bash
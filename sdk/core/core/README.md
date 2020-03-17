# Azure SDK Core Library for C

Azure Core library for C (`az_core`) provides shared primitives, abstractions, and helpers for modern Azure SDK client libraries written in the C programming language. These libraries follow the Azure SDK Design Guidelines for Embedded C.

The library allows client libraries to expose common functionality in a consistent fashion.  Once you learn how to use these APIs in one client library, you will know how to use them in other client libraries.

## Getting started

TODO

## Porting the Azure SDK to Another Platform

The `Azure Core` library requires you to implement a few functions to provide platform-specific features such as a clock, a thread sleep, a mutual-exclusive thread synchronization lock, and an HTTP stack. By default, `Azure Core` ships with no-op versions of these functions, all of which return `AZ_RESULT_NOT_IMPLEMENTED`.

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

### Working with Spans

An `az_span` is a small data structure (defined in our [az_span.h](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/core/core/inc/az_span.h) file) wrapping a byte buffer. Specifically, an `az_span` instance contains:

- a byte pointer
- an integer capacity
- an integer length

Our SDK passes `az_span` instances to functions to ensure that a buffer’s address capacity, and length are always passed together; this reduces the chance of bugs. And, since we have the length and capacity, operations are fast; for example, we never need to call `strlen` to find the length of a string in order to append to it. Furthermore, when our SDK functions write or append to an `az_span`, our functions ensure that we never write beyond the capacity of the buffer; this prevents data corruption. And finally, when reading from an `az_span`, we never read past the `az_span`’s length ensuring that we don’t process uninitialized data.

Since many of our SDK functions require `az_span` parameters, customers must know how to create `az_span` instances so that you can call functions in our SDK. Here are some examples.

Create an empty (or NULL) `az_span`:

```C
az_span span_null = AZ_SPAN_NULL; // cap = 0, len = 0
```

Create an `az_span` literal from an uninitialized byte buffer:

```C
uint8_t buffer[1024];
az_span span_over_buffer = AZ_SPAN_LITERAL_FROM_BUFFER(buffer); // cap = 1024, len = 0
```

Create an `az_span` literal from an initialized bytes buffer:

```C
uint8_t buffer[] = { 1, 2, 3, 4, 5 };
az_span span_over_buffer = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(buffer); // cap = 5, len = 5
```

Create an `az_span` expression from an uninitialized byte buffer:

```C
uint8_t buffer[1024];
some_function(AZ_SPAN_FROM_BUFFER(buffer));  // cap = 1024, len = 0
```

Create an `az_span` expression from an initialized bytes buffer:

```C
uint8_t buffer[] = { 1, 2, 3, 4, 5 };
some_function(AZ_SPAN_FROM_INITIALIZED_BUFFER(buffer));  // cap = 5, len = 5
```

Create an `az_span` literal from a string (the span does NOT include the 0-terminating byte):

```C
az_span span_over_str = AZ_SPAN_LITERAL_FROM_STR("Hello");  // cap = 5, len = 5
```

Create an `az_span` expression from a string (the span does NOT include the 0-terminating byte):

```C
some_function(AZ_SPAN_FROM_STR("Hello"));  // cap = 5, len = 5
```

As shown above, an `az_span` over a string does not include the 0-terminator. If you need to 0-terminate the string, you can call this function to append a 0 byte (if the string’s length is less than its capacity):

```C
az_result az_span_append_uint8(az_span destination, uint8_t byte, az_span* out_span);
```

and then call this function to get the address of the 0-terminated string:

```C
char* str = (char*) az_span_ptr(span); // str points to a 0-terminated string
```

Or, you can call this function to copy the string in the `az_span` to your own `char*` buffer; this function will 0-termiante the string in the `char*` buffer:

```C
az_result az_span_to_str(char* destination, int32_t destination_max_size, az_span source);
```

There are many functions to manipulate `az_span` instances. You can slice (subset an `az_span`), parse an `az_span` containing a string into an number, append a number as a string to the end of an `az_span`, check if two `az_span` instances are equal or the contents of two `az_span` instances are equal, and more.

### Strings

A string is a span of UTF-8 characters. It's not a zero-terminated string. Defined in [inc/az_span.h](inc/az_span.h).

```c
az_span hello_world = AZ_SPAN_FROM_STR("Hello world!");
```

### Logging SDK Operations

As our SDK performs operations, it can send log messages to a customer-defined callback. Customers can enable this to assist with debugging and diagnosing issues when leveraging our SDK code.

To enable logging, you must first write a callback function that our logging mechanism will invoke periodically with messages. The function signature must match this type definition (defined in the [az_log.h](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/core/core/inc/az_log.h) file):

   ```C
   typedef void (*az_log_fn)(az_log_classification classification, az_span message);
   ```

And then, during your application’s initialization, you must register your function with our SDK by calling this function:

   ```C
   void az_log_set_listener(az_log_fn listener);
   ```

Now, whenever our SDK wants to send a log message, it will invoke your callback function passing it the log classification and an `az_span` containing the message string (not 0-terminated). Your callback method can now do whatever it wants to with this message such as append it to a file or write it to the console. 

**Note:** In a multi-threaded application, multiple threads may invoke this callback function simultaneously; if your function requires any kind of thread synchronization, then you must provide it.

Log classifications allow your application to select which specific log messages it wants to receive. For example, to log just HTTP response messages (and not HTTP request messages), initialize your application by calling this:

   ```C
   az_log_classification const classifications[] = { AZ_LOG_HTTP_REQUEST };
   az_log_set_classifications(classifications, sizeof(classifications) / sizeof(classifications[0]));
   ```

### Canceling an Operation

`Azure Core` provides a rich cancellation mechanism by way of its `az_context` type (defined in the [az_context.h](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/core/core/inc/az_context.h) file). As your code executes and functions call other functions, a pointer to an `az_context` is passed as an argument through the functions. At any point, a function can create a new `az_context` specifying a parent `az_context` and a timeout period and then, this new `az_context` is passed down to more functions. When a parent `az_context` instance expires or is canceled, all of its children are canceled as well.

There is a special singleton instance of the `az_context` type called `az_context_app`. This instance represents your entire application and this `az_context` instance never expires. It is common to use this instance as the ultimate root of all `az_context` instances. So then, as functions call other functions, these functions can create child `az_context` instances and pass the child down through the call tree. Imagine you have the following `az_context` tree:

- `az_context_app`; never expires
  - `az_context_child`; expires in 10 seconds
    - `az_context_grandchild`; expires in 60 seconds

Any code using `az_context_grandchild` expires in 10 seconds (not 60 seconds) because it has a parent that expires in 10 seconds. In other words, each child can specify its own expiration time but when a parent expires, all its children also expire. While `az_context_app` never expires, your code can explicitly cancel it thereby canceling all the children `az_context` instances. This is a great way to cleanly cancel all operations in your application allowing it to terminate quickly.

Note however that cancellation is performed as a best effort; it is not guaranteed to work in a timely fashion. For example, the HTTP stack that you use may not support cancellation. In this case, cancellation will be detected only after the I/O operation completes or before the next I/O operation starts.

   ```C
   // Some function creates a child with a 10-second expiration:
   az_context child = az_context_with_expiration(&az_context_app, 10);

   // Some function creates a grandchild with a 60-second expiration:
   az_context grandchild = az_context_with_expiration(&child, 60);

   // Some other function (perhaps in response to a SIGINT) cancels the application root:
   az_context_cancel(&az_context_app);
   // All children are now in the canceled state & the threads will start unwinding
   ```

## Examples

### az_log.h

The various components of the SDK are broken up into "classifications". Log messages are filtered with these classification enums so that the user can decide which log messages they want. Classifications are derived from higher level groupings called "facilities". At the beginning of the user's code, they can initialize the logging by optionally setting the classifications they want, setting their logging listener, and then logging any messages they desire.

#### Basic Code Snippet

Relevant components for logging are located in [az_result.h](./inc/az_result.h) and [az_log.h](./inc/az_log.h). The `az_log_classification` enum will be used to choose features to log.

```c
/* az_result.h */
enum
{
  _az_FACILITY_CORE = 0x1,
  _az_FACILITY_PLATFORM = 0x2,
  _az_FACILITY_JSON = 0x3,
  _az_FACILITY_HTTP = 0x4,
  _az_FACILITY_MQTT = 0x5,
  _az_FACILITY_IOT = 0x6,
  _az_FACILITY_STD = 0x7FFF,
};

/* az_log.h */
typedef enum {
  AZ_LOG_HTTP_REQUEST  = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_HTTP, 1),
  AZ_LOG_HTTP_RESPONSE = _az_LOG_MAKE_CLASSIFICATION(_az_FACILITY_HTTP, 2),
} az_log_classification;
```

Here is an example of what basic sdk and user code might look like working together.
The user needs to do two things as exemplified below:

1. Set the classifications you wish to log.
2. Set your logging function that follows the `az_log_message_fn` prototype. In this case, the logging function uses a basic `printf()`.

```c
/* INTERNAL sdk http code */
static az_span test_log_message = AZ_SPAN_LITERAL_FROM_STR("HTTP Request Success");

void some_http_request_code()
{
  /* Some http code */
  az_log_write(AZ_LOG_HTTP_REQUEST, test_log_message);
}


/* User Application Code */
az_log_classification const classifications[] = { AZ_LOG_HTTP_REQUEST, AZ_LOG_HTTP_RESPONSE, AZ_LOG_END_OF_LIST };

void test_log_func(az_log_classification classification, az_span message)
{
    printf("%.*s\n", az_span_length(message), az_span_ptr(message));
}

int main()
{
  az_log_set_classifications(classifications);
  az_log_set_callback(&test_log_func);

  some_http_request_code();
}
```

Here the classifications are set to `AZ_LOG_HTTP_REQUEST` and `AZ_LOG_HTTP_RESPONSE`. Should the `AZ_LOG_HTTP_REQUEST` be omitted from the set of classifications, the log message in `some_http_request_code()` will not be logged.

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
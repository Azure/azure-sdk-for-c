# Azure SDK Core Library

Azure Core library (`az_core`) provides shared primitives, abstractions, and helpers for modern Azure SDK client libraries written in the C programming language. These libraries follow the Azure SDK Design Guidelines for Embedded C.

The library allows client libraries to expose common functionality in a consistent fashion, so that once you learn how to use these APIs in one client library, you will know how to use them in other client libraries.

## 1. Error Structure

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

## 2. Span of bytes

Defined in [inc/az_span.h](inc/az_span.h).

```c
typedef struct {
  uint8_t const *begin;
  size_t size;
} az_const_span;

typedef struct {
  uint8_t *begin;
  size_t size;
} az_span;
```

## 3. Strings

A string is a span of UTF-8 characters. It's not a zero-terminated string. Defined in [inc/az_str.h](inc/az_str.h).

```c
az_const_span const hello_world = AZ_STR("Hello world!");
```

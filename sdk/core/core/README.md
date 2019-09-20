# Azure SDK Core Library

Azure Core library (`az_core`) provides shared primitives, abstractions, and helpers for modern Azure SDK client libraries written in the C programming language. These libraries follow the Azure SDK Design Guidelines for Embedded C.

The library allows client libraries to expose common functionality in a consistent fashion, so that once you learn how to use these APIs in one client library, you will know hot to use them in other client libraries.

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

enum {
  AZ_STREAM_ERROR = AZ_MAKE_ERROR(AZ_CORE_FACILITY, 1),
};

inline bool az_failed(az_result result) {
  return (result & AZ_ERROR_FLAG) != 0;
}

inline bool az_succeeded(az_result result) {
  return (result & AZ_ERROR_FLAG) == 0;
}
```

Additional information could be passed using output parameters.

## 2. Strings

UTF-8. Defined in [inc/az_str.h](inc/az_str.h). It's a pair of a pointer to `uint8_t const` and size.

```c
typedef struct {
  uint8_t const *begin;
  size_t size;
} az_const_str;

az_const_str const hello_world = AZ_CONST_STR("Hello world!");

typedef struct {
  uint8_t *begin;
  size_t size;
} az_str;

inline az_const_str az_to_const_str(az_str const str) {
  return (az_const_str){ .begin = str.begin, .size = str.size };
}
```

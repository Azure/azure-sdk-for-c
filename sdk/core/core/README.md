# Azure SDK Core Library

In high-level languages, an [inversion of control](https://en.wikipedia.org/wiki/Inversion_of_control) is commonly used.
For example, a generic JSON deserializer may call a user provided function to set object property value (similar to [Visitor Pattern](https://en.wikipedia.org/wiki/Visitor_pattern)). The principle requires a call-back
function or a virtual table. In C, a call-back or a virtual table requires an untyped parameter `void *context`.
It's very hard to work safely with such function so Azure SDK Core library for C tries to avoid these technic.
Instead, the Core library provides safe and useful parts to construct complex solutions.

## 1. Data Types

### 1.1. Const in `struct`

Don't use `const` fields in structures and unions. For example:

```c
struct my_struct {
  size_t const size;
};

// ok
struct my_struct my_struct_create() {
  return (struct my_struct){ .size = 5 };
}

void my_struct_init(struct my_struct *const p) {
  // compilation error.
  *p = (struct my_struct){ .size = 5 };
}
```

### 1.2. Tagged Unions

Example of `az_mytype` tagged union.

```c
enum {
  AZ_MYTYPE_FOO,
  AZ_MYTYPE_BAR,
};

typedef uint8_t az_mytype_kind;

typedef double az_mytype_foo;

typedef struct {
  double a;
  double b;
} az_mytype_bar;

inline az_mytype_bar az_mytype_bar_create(double const a, double const b) {
  return (az_mytype_bar){ .a = a, .b = b };
}

struct my_type {
};

typedef struct {
  az_mytype_kind kind;
  union {
    az_mytype_foo foo;
    az_mytype_bar bar;
  } data;
} az_mytype;

inline az_mytype_create_bar(az_mytype_bar const bar) {
  return (az_mytype){ .kind = AZ_MYTYPE_BAR, .data.bar = bar };
}
```

### 1.3. Strings

UTF-8. No MUTF-8. Defined in `az_str.h`. It's a pair of a pointer to `int8_t const` and size.

```c
typedef struct {
  int8_t const *begin;
  size_t size;
} az_const_str;

az_const_str const hello_world = AZ_CONST_STR("Hello world!");

typedef struct {
  int8_t *const begin;
  size_t const size;
} az_str;

inline az_const_str az_to_const_str(az_str const str) {
  return (az_const_str){ .begin = str.begin, .size = str.size };
}
```

### 1.4 Error Structure

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

## 2. JSON Parser

The main purpose of the JSON parser is to deserialize JSON HTTP Responses into known the data structure.
The assumption is that we receive a valid standard JSON.

### 2.1. Layer 1. Synchronous JSON Value Parser

Utility functions to read primitive values, objects and arrays from a continuous JSON string.
The functions doesn't maintain a stack for nested objects and arrays. The assumption is that deserializer knows
the structures and can maintain a proper stack.

```c
typedef struct { ... } az_json_state;

az_json_state az_json_reader_create(az_const_str const buffer);

typedef az_const_str az_json_string;

typedef enum {
  AZ_JSON_VALUE_NONE,
  AZ_JSON_VALUE_NULL,
  AZ_JSON_VALUE_BOOLEAN,
  AZ_JSON_VALUE_STRING,
  AZ_JSON_VALUE_NUMBER,
  AZ_JSON_VALUE_OBJECT,
  AZ_JSON_VALUE_ARRAY,
} az_json_value_kind;

typedef struct {
  az_json_value_kind kind;
  union {
    bool boolean;
    az_json_string string;
    double number;
  } data;
} az_json_value;

az_error az_json_read(az_json_state *const p_state, az_json_value *const out_value);

typedef struct {
  az_json_string name;
  az_json_value value;
} az_json_member;

// if returns AZ_JSON_NO_MORE_ITEMS then there are no more properties and the object is closed.
az_error az_json_read_object_member(az_json_state *const p_state, az_json_member *const out_member);

// if returns AZ_JSON_NO_MORE_ITEMS then there are no more items and the array is closed.
az_error az_json_read_array_element(az_json_state *const p_state, az_json_value *const out_element);
```

JSON stack size https://softwareengineering.stackexchange.com/questions/279207/how-deeply-can-a-json-object-be-nested

## 3. Issues

- [ ] C unit test framework.
  - [ ] Visual Studio support.
  - [ ] Code coverage.
- [ ] Use `AZ_JSON_NO_MORE_ITEMS` or `AZ_JSON_NONE`?
- [ ] string reader/writer (in-memory text reader/writer).
  ```c
  typedef struct {
    az_const_str buffer;
    size_t i;
  } az_str_reader;

  typedef struct {
    az_str buffer;
    size_t i;
  } az_str_writer;

  enum {
    // AZ_STR_ERROR_END_OF_BUFFER
    AZ_STR_ERROR_UNEXPECTED_END = ...
  };
  ```

- [ ] JSON number reader/writer should produce the same numbers as C library.
- [ ] check `static inline` for Linux.

### const fields in az_const_str



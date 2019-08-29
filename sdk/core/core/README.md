# Azure SDK Core Library

In high-level languages, an [inversion of control](https://en.wikipedia.org/wiki/Inversion_of_control) is commonly used.
For example, a generic JSON deserializer may call a user provided function to set object property value (similar to [Visitor Pattern](https://en.wikipedia.org/wiki/Visitor_pattern)). The principle requires a call-back
function or a virtual table. In C, a call-back and a virtual table require an untyped parameter `void *context`.
It's very hard to work safely with such function so Azure SDK Core library for C tries to avoid these technic.
Instead, the Core library provides safe and useful parts to construct complex solutions.

## 1. Data Types

### 1.1. Tagged Unions

Example of `az_mytype` tagged union.

```c
enum {
  AZ_MYTYPE_FOO,
  AZ_MYTYPE_BAR,
};

typedef uint8_t az_mytype_tag;

typedef double az_mytype_foo;

typedef struct {
  double a;
  double b;
} az_mytype_bar;

inline az_mytype_bar az_mytype_bar_create(double const a, double const b) {
  return (az_mytype_bar){ .a = a, .b = b };
}

typedef struct {
  az_mytype_tag tag;
  union {
    az_mytype_foo foo;
    az_mytype_bar bar;
  };
} az_mytype;

inline az_mytype_create_bar(az_mytype_bar const bar) {
  return (az_mytype){ .tag = AZ_MYTYPE_BAR, .bar = bar };
}
```

### 1.2. Strings

UTF-8. No MUTF-8. Defined in `az_cstr.h`. It's a pair of a pointer to `char const` and size.

```c
typedef struct {
  char const *begin;
  size_t size;
} az_cstr;

az_cstr const hello_world = AZ_CSTR("Hello world!");
```

### 1.3 Error Structure

Common `enum az_error`.

```c
// az_core.h
typedef enum {
  AZ_OK = 0,
  ...
  AZ_JSON_ERROR    = 0x10000,
  AZ_STORAGE_ERROR = 0x20000,
  ...
} az_error;
```

```c
// az_storage.h
typedef enum {
  AZ_STORAGE_READ_ERROR = AZ_STORAGE_ERROR + 1,
} az_storage_error;

az_error az_storage_read(...);
```

Additional information could be passed using output parameters.

## 2. JSON Parser

The main purpose of the JSON parser is to deserialize JSON HTTP Responses into known the data structure.
The assumption is that we receive a valid standard JSON.

### 2.1. Layer 0. JSON State Machine

A JSON state machine which can be used as a part of asynchronous/reactive JSON parser.

```c
typedef enum {
  AZ_JSON_STATE_WHITESPACE,
  AZ_JSON_STATE_ERROR,
  ...
  AZ_JSON_STATE_NULL,
  AZ_JSON_STATE_FALSE,
  ...
} az_json_state;

// The function accepts a previous state and a new character and returns an new state.
inline az_json_state az_json_state_value_parse(az_json_state state, char c);
```

### 2.2. Layer 1. Synchronous JSON Value Parser

Utility functions to read primitive values, objects and arrays from a continuous JSON string.
The functions doesn't maintain a stack for nested objects and arrays. The assumption is that deserializer knows
the structures and can maintain a proper stack.

```c
typedef struct {
  size_t begin;
  size_t end;
} az_json_string;

typedef enum {
  AZ_JSON_NULL,
  AZ_JSON_BOOLEAN,
  AZ_JSON_STRING,
  AZ_JSON_NUMBER,
  AZ_JSON_OBJECT,
  AZ_JSON_ARRAY,
} az_json_value_tag;

typedef struct {
  az_json_value_tag tag;
  union {
    bool boolean;
    az_json_string string;
    double number;
    // true - is done (an empty object).
    // false - the object has properties (use `az_json_read_property` and `az_json_read_object_end`).
    bool object;
    // true - is done (an empty array).
    // false - the array has items (use `az_json_read_property` and `az_json_read_object_end`).
    bool array;
  };
} az_json_value;

az_error az_json_read_value(az_cstr const buffer, size_t *p_position, az_json_value *out_json_value);

typedef struct {
  az_json_string name;
  az_json_value value;
} az_json_property;

az_error az_json_read_property(az_cstr const buffer, size_t *p_position, az_json_property *out_property);

az_error az_json_read_object_end(az_cstr const buffer, size_t *p_position, bool *out_more_properties);

az_error az_json_read_item(az_cstr const buffer, size_t *p_position, az_json_value *out_item);

az_error az_json_read_array_end(az_cstr const buffer, size_t *p_position, bool *out_more_items);
```

## Issues

- [ ] JSON number reader/writer should produce the same numbers as C library.
- [ ] check `static inline` for Linux.
- [ ] Immutable value az_cstr.

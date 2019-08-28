# Azure SDK Core Library

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
typedef enum {
  AZ_OK = 0,
  ...
  AZ_JSON_ERROR    = 0x10000,
  AZ_STORAGE_ERROR = 0x20000,
  ...
} az_error;

typedef enum {
  AZ_STORAGE_READ_ERROR = AZ_STORAGE_ERROR + 1,
} az_storage_error;

az_error az_storage_read(...);
```

Additional information could be passed using output parameters.

## 2. JSON Parser

The main purpose of the JSON parser is to deserialize JSON HTTP Responses into known the data structure.
The assumption is that we receive a valid standard JSON.

### 2.1. Layer 0. JSON State Machine.

This layer is a JSON state machine which can be used as a part of asynchnous/reactive JSON parser.

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

This layer is utility functions to read primitive values, object and arrays from a continuous JSON string.
The functions doesn't maintain a stack for nested objects and arrays. The assumption is that deserializer knows
the type structure and can maintain a proper stack.

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
    // true - the object has properties, use `read_property`.
    // false - the object is empty.
    bool object;
    // true - the object has properties, use `read_item`.
    // false - the array is empty.
    bool array;
  };
} az_json_value;

az_error az_json_read_value(az_cstr const buffer, size_t *p_position, az_json_value *out_json_value);

typedef struct {
  az_json_string name;
  az_json_value value;
  bool more_properties;
} az_json_property;

az_error az_json_read_property(az_cstr const buffer, size_t *p_position, az_json_property *out_property);

typedef struct {
  az_json_value value;
  bool more_items;
} az_json_item;

az_error az_json_read_item(az_cstr const buffer, size_t *p_position, az_json_item *out_item);
```

## Issues

- [ ] JSON number reader/writer should produce the same numbers as C library.

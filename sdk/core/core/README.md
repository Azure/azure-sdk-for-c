# Azure SDK Core Library

In high-level languages, an [inversion of control](https://en.wikipedia.org/wiki/Inversion_of_control) is commonly used.
For example, a generic JSON deserializer may call a user provided function to set object property value (similar to [Visitor Pattern](https://en.wikipedia.org/wiki/Visitor_pattern)). The principle requires a call-back
function or a virtual table. In C, a call-back or a virtual table requires an untyped parameter `void *context`.
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

struct my_type {
};

typedef struct {
  az_mytype_tag tag;
  union {
    az_mytype_foo foo;
    az_mytype_bar bar;
  } val;
} az_mytype;

inline az_mytype_create_bar(az_mytype_bar const bar) {
  return (az_mytype){ .tag = AZ_MYTYPE_BAR, .val.bar = bar };
}
```

### 1.2. Strings

UTF-8. No MUTF-8. Defined in `az_str.h`. It's a pair of a pointer to `char const` and size.

```c
typedef struct {
  char const *const begin;
  size_t const size;
} az_const_str;

az_const_str const hello_world = AZ_CONST_STR("Hello world!");

typedef struct {
  char *const begin;
  size_t const size;
} az_str;

inline az_const_str az_to_const_str(az_str const str) {
  return (az_const_str){ .begin = str.begin, .size = str.size };
}
```

### 1.3 Error Structure

Common `enum az_result`.

```c
// az_core.h
typedef enum {
  AZ_OK = 0,
  ...
  AZ_JSON_ERROR    = 0x10000,
  AZ_STORAGE_ERROR = 0x20000,
  ...
} az_result;
```

```c
// az_storage.h
enum {
  AZ_STORAGE_READ_ERROR = AZ_STORAGE_ERROR + 1,
};

az_result az_storage_read(...);
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
typedef struct { ... } az_json_state;

az_json_state az_json_reader_create(az_const_str const buffer);

typedef az_const_str az_json_string;

typedef enum {
  AZ_JSON_NONE,
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
  } val;
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

## Issues

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

## const fields in az_const_str

We can't use const fields in the structure and output parameters. For example:

```c
struct my_struct {
  size_t const size;
};

void my_struct_init(struct my_struct *const p) {
  // compilation error.
  *p = (struct my_struct){ .size = 5 };
}

// ok
struct my_struct my_struct_create() {
  return (struct my_struct){ .size = 5 };
}
```

C++ example:

```c++
class wrap {
public:
  explicit wrap(int const i): i(i) {}
  wrap plus(int const i) const { return wrap(this->i + i); }

  // not required.
  wrap &operator=(wrap const &w) {
    this->i = w.i;
    return *this;
  }
private:
  int i;
};

void test(wrap *const p) {
  *p = wrap(7);
  *p = p->plus(7);
}
```

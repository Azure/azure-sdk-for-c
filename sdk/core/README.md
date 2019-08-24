# Options

## Discriminated Unions

Example of `az_some_class` discriminated union. variant/tag union.

```c
typedef enum {
  AZ_SOME_CLASS_STRING,
  AZ_SOME_CLASS_NUMBER,
  AZ_SOME_CLASS_NOTHING,
} az_some_class_type;

typedef struct {
  az_some_class_type type;
  union {
    // AZ_SOME_CLASS_STRING
    az_index_range string;
    // AZ_SOME_CLASS_NUMBER
    double number;
    // AZ_SOME_CLASS_NOTHING
  };
} az_some_class;
```

We may have a type 'az_core_any_type`...

## Ranges vs. Slices vs. Index Ranges

1. Ranges (common in C++, SubRange)

   ```c
   typedef struct {
     int *begin;
     int *end;
   } az_int_range;
   ```

   ```c
   for (; begin != end; ++begin) {
     ...*begin...
   }
   ```

1. Slices (views, requires utility functions which are difficult to have in C without templates/generics)

   ```c
   typedef struct {
     int *p;
     size_t size;
   } az_int_slice;
   ```

   ```c
   for (size_t i = 0; i < size; ++i) {
     ...p[i]...
   }
   ```

1. Slices and index ranges. **Preferred because it adds extra safety.**

   ```c
   typedef struct {
     int *p;
     size_t size;
   } az_int_container;

   typedef struct {
     size_t begin;
     size_t end;
   } az_index_range;
   ```

## Strings

UTF-8. No MUTF-8. We can use 0xFF as the end of character stream.

## Immutability

1. Immutable by default (common in functional programming languages, including `Rust`)

   ```c
   typedef struct {
     int const *begin;
     int const *end;
   } az_int_range;

   typedef struct {
     int *begin;
     int *end;
   } az_mutable_int_range;
   ```

1. Mutable by default (**Preffered because it is common in C and C++).

   ```c
   typedef struct {
     int *begin;
     int *end;
   } az_int_range;

   typedef struct {
     int const *begin;
     int const *end;
   } az_cint_range;
   ```

## Error Handling

1. All functions returns common `enum az_error`.

   ```c
   typedef enum {
     AZ_OK = 0,
     AZ_STORAGE_ERROR = 0x10000,
   } az_error;

   typedef enum {
     AZ_STORAGE_READ_ERROR = AZ_STORAGE_ERROR + 1,
   } az_storage_error;

   az_error az_storage_read(...);
   ```

   Additional information could be passed using output parameters.

1. Functions use structures to return all results using structures and discriminated unions.

   ```c
   typedef enum {
     AZ_OK = 0,
     AZ_SOME_ERROR = 1,
   } az_error;

   typedef struct {
     az_error error;
     union {
       int ok;
       az_some_error_additional_information some_error;
     };
   } az_myfunc_result;

   az_myfunc_result az_myfunc(int some_parameter);
   ```

## OOP Conventions

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

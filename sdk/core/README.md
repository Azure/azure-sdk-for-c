# Data Types

## Tagged Unions

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

## Strings

UTF-8. No MUTF-8. Defined in `az_cstr.h`. It's a pair of a pointer to `char const` and length.

```c
az_cstr const hello_world = AZ_CSTR("Hello world!");
```

## Ranges vs. Slices vs. Index Ranges

Slices and index ranges.

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

## Immutability

Mutable by default. Use `c` prefix to mark immutable types.

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

## Error Structure

### Common `enum az_error`

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

### Tagged union as a result

Some functions use structures to return all results using structures and discriminated unions.

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

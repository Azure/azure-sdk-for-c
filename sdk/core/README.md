# Options

## Strings

UTF-8. No MUTF-8.

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
   } az_int_slice;

   typedef struct {
     size_t begin;
     size_t end;
   } az_index_range;
   ```

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

# Options

## Strings

UTF-8. No MUTF-8.

## Ranges vs. slices

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
     int *begin;
     size_t size;
   } az_int_slice;
   ```

   ```c
   for (size_t i = 0; i < size; ++i) {
     ...begin[i]...
   }
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

1. Mutable by default (common in C and C++).

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

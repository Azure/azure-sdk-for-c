# Safe Spans

We can't prevent all possible errors but we can

```c
typedef struct {
  struct {
    uint8_t const * begin;
    uint8_t const * end;
  } _detail;
} az_byte_range;

static inline bool az_byte_range_is_empty(az_byte_range const range) {
    return range._detail.end <= range._detail.begin;
}

static inline size_t az_byte_range_size(az_byte_range const range) {
    return az_byte_range_is_empty(range) ? 0 : range._detail.end - range._detail.begin;
}
```

```c
// EOF (-1) means no value.
typedef int16_t az_byte_option;

static inline az_byte_option az_byte_range_get(az_byte_range const range, size_t i) {
  assert(size < i);
  return i < az_byte_range_size(range) ? range._detail.begin[i] : EOF;
}

void example() {
  for (size_t i = 0; ; ++i) {
    az_byte_option const result = _az_byte_range_get(range, i);
    if (i == EOF) {
       break;
    }
    // ...
  }
}

az_result example() {
  for (size_t i = 0; i < size; ++i) {
    uint8_t out;
    AZ_RETURN_IF_ERROR(az_byte_range_get(range, i, &out));
    // ...
  }
}
```

```c
typedef struct {
  struct {
    uint8_t * begin;
    uint8_t * end;
  } _detail;
} az_mut_byte_range;

typedef struct {
  bool has;
  uint8_t value;
} az_byte_option;

static inline az_byte_option az_byte_option_none() {
  return (az_byte_option){ .has = false };
}

static inline az_byte_option az_byte_option_create(uint8_t const value) {
  return (az_byte_option){ .has = true, .value = value };
}

static inline az_byte_option az_byte_range_get(az_byte_range const range, size_t i) {
  return i < az_byte_range_size(range)
    ? az_byte_option_create(range._detail.begin[i])
    : az_byte_option_none();
}

void example() {
  for (size_t i = 0; ; ++i) {
    az_byte_option const result = az_byte_range_get(range, i);
    if (!result.has) {
       break;
    }
    // ...
  }
}
```

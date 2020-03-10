// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_span_private.h"
#include <az_platform_internal.h>
#include <az_precondition.h>
#include <az_span.h>

#include <ctype.h>
#include <stdint.h>

#include <_az_cfg.h>

enum
{
  AZ_ASCII_LOWER_DIF = 'a' - 'A',
};

AZ_NODISCARD az_span az_span_init(uint8_t* ptr, int32_t length, int32_t capacity)
{
  AZ_PRECONDITION_RANGE(0, capacity, INT32_MAX);
  AZ_PRECONDITION_RANGE(0, length, capacity);

  return (az_span){ ._internal = { .ptr = ptr, .length = length, .capacity = capacity, }, };
}

AZ_NODISCARD az_span az_span_from_str(char* str)
{
  AZ_PRECONDITION_NOT_NULL(str);

  int32_t length = (int32_t)strlen(str);
  return az_span_init((uint8_t*)str, length, length);
}

AZ_NODISCARD az_span az_span_slice(az_span span, int32_t low_index, int32_t high_index)
{
  AZ_PRECONDITION_RANGE(-1, high_index, az_span_capacity(span));
  AZ_PRECONDITION(high_index == -1 || (high_index >= 0 && low_index <= high_index));

  int32_t capacity = az_span_capacity(span);

  high_index = high_index == -1 ? capacity : high_index;
  return az_span_init(az_span_ptr(span) + low_index, high_index - low_index, capacity - low_index);
}

/**
 * ASCII lower case.
 */
AZ_NODISCARD AZ_INLINE uint8_t az_ascii_lower(uint8_t value)
{
  return 'A' <= value && value <= 'Z' ? value + AZ_ASCII_LOWER_DIF : value;
}

AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span a, az_span b)
{
  int32_t const size = az_span_length(a);
  if (size != az_span_length(b))
  {
    return false;
  }
  for (int32_t i = 0; i < size; ++i)
  {
    if (az_ascii_lower(az_span_ptr(a)[i]) != az_ascii_lower(az_span_ptr(b)[i]))
    {
      return false;
    }
  }
  return true;
}

AZ_NODISCARD az_result az_span_to_uint64(az_span self, uint64_t* out)
{
  int32_t self_length = az_span_length(self);
  if (self_length <= 0)
  {
    return AZ_ERROR_EOF;
  }

  uint64_t value = 0;
  for (int32_t i = 0; i < self_length; ++i)
  {
    uint8_t result = az_span_ptr(self)[i];
    if (!isdigit(result))
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
    uint64_t const d = (uint64_t)result - '0';
    if ((UINT64_MAX - d) / 10 < value)
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }

    value = value * 10 + d;
  }

  *out = value;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_to_uint32(az_span self, uint32_t* out)
{
  int32_t self_length = az_span_length(self);
  if (self_length <= 0)
  {
    return AZ_ERROR_EOF;
  }

  uint32_t value = 0;
  for (int32_t i = 0; i < self_length; ++i)
  {
    uint8_t result = az_span_ptr(self)[i];
    if (!isdigit(result))
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
    uint32_t const d = (uint32_t)result - '0';
    if ((UINT32_MAX - d) / 10 < value)
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }

    value = value * 10 + d;
  }

  *out = value;
  return AZ_OK;
}

/****************** Mutating az_span (used to be az_span in the origins)     ******/

/**
 * @brief move the content of span @b src to @buffer and make @b out_result point to it
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_span_copy
 */
AZ_NODISCARD az_result az_span_copy(az_span dst, az_span src, az_span* out)
{
  AZ_PRECONDITION_VALID_SPAN(dst, 0, true);
  AZ_PRECONDITION_VALID_SPAN(src, 0, true);
  int32_t src_len = az_span_length(src);

  if (az_span_capacity(dst) < src_len)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  };

  uint8_t* ptr = az_span_ptr(dst);

  memmove((void*)ptr, (void const*)az_span_ptr(src), src_len);

  *out = az_span_init(ptr, src_len, az_span_capacity(dst));

  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE bool should_encode(uint8_t c)
{
  switch (c)
  {
    case '-':
    case '_':
    case '.':
    case '~':
      return false;
    default:
      return !(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
  }
}

AZ_NODISCARD az_result az_span_copy_url_encode(az_span dst, az_span src, az_span* out)
{
  AZ_PRECONDITION_NOT_NULL(out);
  AZ_PRECONDITION_VALID_SPAN(dst, 0, true);
  AZ_PRECONDITION_VALID_SPAN(src, 0, true);

  int32_t const input_size = az_span_length(src);

  int32_t result_size = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    result_size += should_encode(az_span_ptr(src)[i]) ? 3 : 1;
  }

  if (az_span_capacity(dst) < result_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  uint8_t* p_s = az_span_ptr(src);
  uint8_t* p_d = az_span_ptr(dst);
  int32_t s = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    uint8_t c = p_s[i];
    if (!should_encode(c))
    {
      *p_d = c;
      p_d += 1;
      s += 1;
    }
    else
    {
      p_d[0] = '%';
      p_d[1] = _az_number_to_upper_hex(c >> 4);
      p_d[2] = _az_number_to_upper_hex(c & 0x0F);
      p_d += 3;
      s += 3;
    }
  }
  *out = az_span_init(az_span_ptr(dst), s, az_span_capacity(dst));

  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE int32_t _az_size_min(int32_t a, int32_t b) { return a < b ? a : b; }

AZ_INLINE void _az_uint8_swap(uint8_t* a, uint8_t* b)
{
  uint8_t const c = *a;
  *a = *b;
  *b = c;
}

/**
 * @brief move the content from span @b a to span @b b and viceverse.
 * The smallest span (less content) is swapped only. Example
 * a = 111
 * b = 22
 * after swaping
 * a = 221
 * b = 11
 *
 * @param a source/destination span
 * @param b destination/source span
 */
void _az_span_swap(az_span a, az_span b)
{
  uint8_t* pa = az_span_ptr(a);
  uint8_t* pb = az_span_ptr(b);
  for (int32_t i = _az_size_min(az_span_length(a), az_span_length(b)); i > 0; ++pa, ++pb)
  {
    --i;
    _az_uint8_swap(pa, pb);
  }
}

/**
 * @brief converts @b src span to zero-terminated str. Content is copied to @b buffer and then \0
 * is addeed at the end. Then out_result will be created out of buffer
 *
 * @param buffer
 * @param src
 * @param out_result
 * @return AZ_NODISCARD az_span_to_str
 */
AZ_NODISCARD az_result az_span_to_str(char* s, int32_t max_size, az_span span)
{
  AZ_PRECONDITION_VALID_SPAN(span, 0, true);

  int32_t span_length = az_span_length(span);
  if (span_length + 1 > max_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  memmove((void*)s, (void const*)az_span_ptr(span), span_length);

  s[span_length] = 0;

  return AZ_OK;
}

/****************** Building az_span (used to be az_span in the origins) *****/

/**
 * @brief append az_span if there is enough capacity for it
 *
 * @param self src span where to append
 * @param span content to be appended
 * @param out result span
 * @return az_result
 */
AZ_NODISCARD az_result az_span_append(az_span self, az_span span, az_span* out)
{
  AZ_PRECONDITION_NOT_NULL(out);

  int32_t const current_size = az_span_length(self);
  az_span remainder = az_span_slice(self, current_size, -1);
  AZ_RETURN_IF_FAILED(az_span_copy(remainder, span, &remainder));

  *out = az_span_init(
      az_span_ptr(self), current_size + az_span_length(span), az_span_capacity(self));

  return AZ_OK;
}

/**
 * @brief Replace all contents from a starting position to an end position with the content of a
 * provided span
 *
 * @param self src span where to replace content
 * @param start starting position where to replace
 * @param end end position where to replace
 * @param span content to use for replacement
 * @return AZ_NODISCARD az_span_replace
 */
AZ_NODISCARD az_result _az_span_replace(az_span* self, int32_t start, int32_t end, az_span span)
{
  AZ_PRECONDITION_NOT_NULL(self);

  int32_t const current_size = az_span_length(*self);
  int32_t const span_length = az_span_length(span);
  int32_t const replaced_size = end - start;
  int32_t const size_after_replace = current_size - replaced_size + span_length;

  // replaced size must be less or equal to current builder size. Can't replace more than what
  // current is available
  if (replaced_size > current_size)
  {
    return AZ_ERROR_ARG;
  };
  // start and end position must be before the end of current builder size
  if (start > current_size || end > current_size)
  {
    return AZ_ERROR_ARG;
  };
  // Start position must be less or equal than end position
  if (start > end)
  {
    return AZ_ERROR_ARG;
  };
  // size after replacing must be less o equal than buffer size
  if (size_after_replace > az_span_capacity(*self))
  {
    return AZ_ERROR_ARG;
  };

  // insert at the end case (no need to make left or right shift)
  if (start == current_size)
  {
    return az_span_append(*self, span, self);
  }
  // replace all content case (no need to make left or right shift, only copy)
  if (current_size == replaced_size)
  {
    return az_span_copy(*self, span, self);
  }

  // get the span needed to be moved before adding a new span
  az_span dst = az_span_slice(*self, start + span_length, current_size);
  // get the span where to move content
  az_span src = az_span_slice(*self, end, current_size);
  {
    // move content left or right so new span can be added
    AZ_RETURN_IF_FAILED(az_span_copy(dst, src, &dst));
    // add the new span
    az_span copy = az_span_slice(*self, start, -1);
    AZ_RETURN_IF_FAILED(az_span_copy(copy, span, &copy));
  }

  // update builder size
  self->_internal.length = size_after_replace;

  return AZ_OK;
}

AZ_NODISCARD az_result az_span_append_double(az_span span, double value, az_span* out)
{
  AZ_PRECONDITION_NOT_NULL(out);

  if (value == 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(span, AZ_SPAN_FROM_STR("0"), out));
    return AZ_OK;
  }
  *out = span;

  if (value < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*out, AZ_SPAN_FROM_STR("-"), out));
    value = -value;
  }

  {
    uint64_t u = (uint64_t)value;
    if (value == (double)u)
    {
      uint64_t base = 1;
      {
        uint64_t i = u;
        while (10 <= i)
        {
          i /= 10;
          base *= 10;
        }
      }
      do
      {
        uint8_t dec = (uint8_t)(u / base) + '0';
        u %= base;
        base /= 10;
        AZ_RETURN_IF_FAILED(az_span_append(*out, az_span_from_single_item(&dec), out));
      } while (1 <= base);
      return AZ_OK;
    }
  }

  // eg. 0.0012
  if (value < 0.001)
  {
    // D.*De-*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  // eg. 1.2e-4
  {
    // *D.*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

AZ_INLINE uint8_t _az_decimal_to_ascii(uint8_t d) { return '0' + d; }

static AZ_NODISCARD az_result _az_span_builder_append_uint64(az_span* self, uint64_t n)
{
  if (n == 0)
  {
    return az_span_append(*self, AZ_SPAN_FROM_STR("0"), self);
  }

  uint64_t div = 10000000000000000000ull;
  uint64_t nn = n;
  while (nn / div == 0)
  {
    div /= 10;
  }

  while (div > 1)
  {
    uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)(nn / div));
    AZ_RETURN_IF_FAILED(az_span_append(*self, az_span_init(&value_to_append, 1, 1), self));

    nn %= div;
    div /= 10;
  }
  uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)nn);
  return az_span_append(*self, az_span_init(&value_to_append, 1, 1), self);
}

AZ_NODISCARD az_result az_span_append_uint64(az_span* self, uint64_t n)
{
  AZ_PRECONDITION_NOT_NULL(self);
  return _az_span_builder_append_uint64(self, n);
}

AZ_NODISCARD az_result az_span_append_int64(az_span* self, int64_t n)
{
  AZ_PRECONDITION_NOT_NULL(self);

  if (n < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*self, AZ_SPAN_FROM_STR("-"), self));
    return _az_span_builder_append_uint64(self, -n);
  }

  return _az_span_builder_append_uint64(self, n);
}

static AZ_NODISCARD az_result
_az_span_builder_append_u32toa(az_span self, uint32_t n, az_span* out_span)
{
  if (n == 0)
  {
    return az_span_append_uint8(self, '0', out_span);
  }

  uint32_t div = 1000000000;
  uint32_t nn = n;
  while (nn / div == 0)
  {
    div /= 10;
  }

  *out_span = self;

  while (div > 1)
  {
    uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)(nn / div));
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_span, value_to_append, out_span));

    nn %= div;
    div /= 10;
  }

  uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)nn);
  return az_span_append_uint8(*out_span, value_to_append, out_span);
}

AZ_NODISCARD az_result az_span_append_u32toa(az_span span, uint32_t n, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);
  return _az_span_builder_append_u32toa(span, n, out_span);
}

AZ_NODISCARD az_result az_span_append_i32toa(az_span span, int32_t n, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  *out_span = span;

  if (n < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_span, '-', out_span));
    n = -n;
  }

  return _az_span_builder_append_u32toa(*out_span, n, out_span);
}

// TODO: pass az_span by value
AZ_NODISCARD az_result _az_is_expected_span(az_span* self, az_span expected)
{
  az_span actual_span = { 0 };

  int32_t expected_length = az_span_length(expected);

  // EOF because self is smaller than the expected span
  if (expected_length > az_span_length(*self))
  {
    return AZ_ERROR_EOF;
  }

  actual_span = az_span_slice(*self, 0, expected_length);

  if (!az_span_is_equal(actual_span, expected))
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  // move reader after the expected span (means it was parsed as expected)
  *self = az_span_slice(*self, expected_length, -1);

  return AZ_OK;
}

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result _az_scan_until(az_span self, _az_predicate predicate, int32_t* out_index)
{
  for (int32_t index = 0; index < az_span_length(self); ++index)
  {
    az_span s = az_span_slice(self, index, -1);
    az_result predicate_result = predicate(s);
    switch (predicate_result)
    {
      case AZ_OK:
      {
        *out_index = index;
        return AZ_OK;
      }
      case AZ_CONTINUE:
      {
        break;
      }
      default:
      {
        return predicate_result;
      }
    }
  }
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_span_append_uint8(az_span span, uint8_t c, az_span* span_out)
{
  return az_span_append(span, az_span_init(&c, 1, 1), span_out);
}

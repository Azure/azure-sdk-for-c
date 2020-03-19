// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_span_private.h"
#include <az_platform_internal.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>

#include <ctype.h>
#include <stdint.h>

#include <_az_cfg.h>

enum
{
  _az_ASCII_LOWER_DIF = 'a' - 'A',
};

AZ_NODISCARD az_span az_span_init(uint8_t* ptr, int32_t length, int32_t capacity)
{
  // This precondition validates that:
  //    If ptr is not null, then:
  //       0 <= capacity <= INT32_MAX
  //    And
  //       0 <= length <= capacity
  //    Otherwise
  //      length == capacity == 0
  AZ_PRECONDITION(
      (ptr != NULL && length >= 0 && length <= capacity)
      || (ptr + (uint32_t)span_length + (uint32_t)span_capacity == 0));

  return (az_span){ ._internal = { .ptr = ptr, .length = length, .capacity = capacity, }, };
}

AZ_NODISCARD AZ_INLINE az_span
_az_span_init_unchecked(uint8_t* ptr, int32_t length, int32_t capacity)
{
  return (az_span){ ._internal = { .ptr = ptr, .length = length, .capacity = capacity, }, };
}

AZ_NODISCARD az_span az_span_from_str(char* str)
{
  AZ_PRECONDITION_NOT_NULL(str);

  int32_t const length = (int32_t)strlen(str);
  return _az_span_init_unchecked((uint8_t*)str, length, length);
}

AZ_NODISCARD az_span az_span_slice(az_span span, int32_t low_index, int32_t high_index)
{
  // The following set of preconditions validate that:
  //    -1 <= high_index <= span.capacity
  // And
  //    If high_index == -1:
  //        0 <= low_index <= span.capacity
  //    Otherwise
  //        0 <= low_index <= high_index
  AZ_PRECONDITION_INT32_RANGE(-1, high_index, az_span_capacity(span));

  // Do not reorder the OR conditions, since we are relying on short-circuiting
  AZ_PRECONDITION(
      (high_index == -1 && ((uint32_t)low_index <= (uint32_t)az_span_capacity(span)))
      || ((uint32_t)low_index <= (uint32_t)high_index));

  int32_t const capacity = az_span_capacity(span);

  high_index = high_index == -1 ? capacity : high_index;
  return _az_span_init_unchecked(
      az_span_ptr(span) + low_index, high_index - low_index, capacity - low_index);
}

AZ_NODISCARD az_span az_span_slice_start(az_span span, int32_t low_index)
{
  AZ_PRECONDITION_INT32_RANGE(0, low_index, az_span_capacity(span));

  int32_t const new_capacity = az_span_capacity(span) - low_index;

  return _az_span_init_unchecked(az_span_ptr(span) + low_index, new_capacity, new_capacity);
}

AZ_NODISCARD az_span az_span_slice_start_end(az_span span, int32_t low_index, int32_t high_index)
{
  AZ_PRECONDITION_INT32_RANGE(0, high_index, az_span_capacity(span));
  AZ_PRECONDITION_INT32_RANGE(0, low_index, high_index);

  return _az_span_init_unchecked(
      az_span_ptr(span) + low_index, high_index - low_index, az_span_capacity(span) - low_index);
}

AZ_NODISCARD AZ_INLINE uint8_t _az_tolower(uint8_t value)
{
  // This is equivalent to the following but with fewer conditions.
  // return 'A' <= value && value <= 'Z' ? value + AZ_ASCII_LOWER_DIF : value;
  if ((uint8_t)(int8_t)(value - 'A') <= ('Z' - 'A'))
  {
    value = (uint8_t)((value + _az_ASCII_LOWER_DIF) & 0xFF);
  }
  return value;
}

AZ_NODISCARD bool az_span_is_content_equal_ignoring_case(az_span span1, az_span span2)
{
  int32_t const size = az_span_length(span1);
  if (size != az_span_length(span2))
  {
    return false;
  }
  for (int32_t i = 0; i < size; ++i)
  {
    if (_az_tolower(az_span_ptr(span1)[i]) != _az_tolower(az_span_ptr(span2)[i]))
    {
      return false;
    }
  }
  return true;
}

AZ_NODISCARD az_result az_span_to_uint64(az_span span, uint64_t* out_number)
{
  AZ_PRECONDITION_MINIMUM_SPAN_LENGTH(span, 1);
  AZ_PRECONDITION_NOT_NULL(out_number);

  int32_t self_length = az_span_length(span);
  uint64_t value = 0;

  for (int32_t i = 0; i < self_length; ++i)
  {
    uint8_t result = az_span_ptr(span)[i];
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

  *out_number = value;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_to_uint32(az_span span, uint32_t* out_number)
{
  AZ_PRECONDITION_MINIMUM_SPAN_LENGTH(span, 1);
  AZ_PRECONDITION_NOT_NULL(out_number);

  int32_t self_length = az_span_length(span);
  uint32_t value = 0;

  for (int32_t i = 0; i < self_length; ++i)
  {
    uint8_t result = az_span_ptr(span)[i];
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

  *out_number = value;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_copy(az_span destination, az_span source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  int32_t src_len = az_span_length(source);
  int32_t dest_capacity = az_span_capacity(destination);

  if (dest_capacity < src_len)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  uint8_t* ptr = az_span_ptr(destination);
  if (ptr == NULL)
  {
    *out_span = AZ_SPAN_NULL;
  }
  else
  {
    memmove((void*)ptr, (void const*)az_span_ptr(source), (size_t)src_len);

    *out_span = _az_span_init_unchecked(ptr, src_len, dest_capacity);
  }
  return AZ_OK;
}

static const uint8_t _is_ascii_byte_url[256] = {
  3, 3, 3, 3, 3, 3, 3, 3, // U+00..U+07
  3, 3, 3, 3, 3, 3, 3, 3, // U+08..U+0F
  3, 3, 3, 3, 3, 3, 3, 3, // U+10..U+17
  3, 3, 3, 3, 3, 3, 3, 3, // U+18..U+1F
  3, 3, 3, 3, 3, 3, 3, 3, // U+20..U+27
  3, 3, 3, 3, 3, 1, 1, 3, // U+28..U+2F
  1, 1, 1, 1, 1, 1, 1, 1, // U+30..U+37
  1, 1, 3, 3, 3, 3, 3, 3, // U+38..U+3F
  3, 1, 1, 1, 1, 1, 1, 1, // U+40..U+47
  1, 1, 1, 1, 1, 1, 1, 1, // U+48..U+4F
  1, 1, 1, 1, 1, 1, 1, 1, // U+50..U+57
  1, 1, 1, 3, 3, 3, 3, 1, // U+58..U+5F
  3, 1, 1, 1, 1, 1, 1, 1, // U+60..U+67
  1, 1, 1, 1, 1, 1, 1, 1, // U+68..U+6F
  1, 1, 1, 1, 1, 1, 1, 1, // U+70..U+77
  1, 1, 1, 3, 3, 3, 1, 3, // U+78..U+7F
  3, 3, 3, 3, 3, 3, 3, 3, // U+80..U+87
  3, 3, 3, 3, 3, 3, 3, 3, // U+88..U+8F
  3, 3, 3, 3, 3, 3, 3, 3, // U+90..U+97
  3, 3, 3, 3, 3, 3, 3, 3, // U+98..U+9F
  3, 3, 3, 3, 3, 3, 3, 3, // U+A0..U+A7
  3, 3, 3, 3, 3, 3, 3, 3, // U+A8..U+AF
  3, 3, 3, 3, 3, 3, 3, 3, // U+B0..U+B7
  3, 3, 3, 3, 3, 3, 3, 3, // U+B8..U+BF
  3, 3, 3, 3, 3, 3, 3, 3, // U+C0..U+C7
  3, 3, 3, 3, 3, 3, 3, 3, // U+C8..U+CF
  3, 3, 3, 3, 3, 3, 3, 3, // U+D0..U+D7
  3, 3, 3, 3, 3, 3, 3, 3, // U+D8..U+DF
  3, 3, 3, 3, 3, 3, 3, 3, // U+E0..U+E7
  3, 3, 3, 3, 3, 3, 3, 3, // U+E8..U+EF
  3, 3, 3, 3, 3, 3, 3, 3, // U+F0..U+F7
  3, 3, 3, 3, 3, 3, 3, 3, // U+F8..U+FF
};

AZ_NODISCARD AZ_INLINE bool _should_url_encode(uint8_t c) { return _is_ascii_byte_url[c] == 3; }

AZ_NODISCARD az_result
az_span_copy_url_encode(az_span destination, az_span source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  int32_t const input_size = az_span_length(source);

  uint8_t* p_s = az_span_ptr(source);
  int32_t result_size = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    uint8_t c = p_s[i];
    result_size += _is_ascii_byte_url[c];
  }

  int32_t dest_capacity = az_span_capacity(destination);
  if (dest_capacity < result_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  uint8_t* p_d = az_span_ptr(destination);
  for (int32_t i = 0; i < input_size; ++i)
  {
    uint8_t c = p_s[i];
    if (!_should_url_encode(c))
    {
      *p_d = c;
      p_d += 1;
    }
    else
    {
      p_d[0] = '%';
      p_d[1] = _az_number_to_upper_hex(c >> 4);
      p_d[2] = _az_number_to_upper_hex(c & 0x0F);
      p_d += 3;
    }
  }
  *out_span = _az_span_init_unchecked(az_span_ptr(destination), result_size, dest_capacity);

  return AZ_OK;
}

AZ_NODISCARD az_result
az_span_to_str(char* destination, int32_t destination_max_size, az_span source)
{
  AZ_PRECONDITION_NOT_NULL(destination);
  AZ_PRECONDITION(destination_max_size >= 0);

  int32_t span_length = az_span_length(source);

  // Subtract from destination_max_size to avoid integer overflow.
  if (span_length > destination_max_size - 1)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  memmove((void*)destination, (void const*)az_span_ptr(source), (size_t)span_length);

  destination[span_length] = 0;

  return AZ_OK;
}

AZ_NODISCARD az_result az_span_append(az_span destination, az_span source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  int32_t const current_size = az_span_length(destination);
  az_span remainder = az_span_slice_start(destination, current_size);
  AZ_RETURN_IF_FAILED(az_span_copy(remainder, source, &remainder));

  *out_span = az_span_init(
      az_span_ptr(destination),
      current_size + az_span_length(source),
      az_span_capacity(destination));

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
    az_span copy = az_span_slice_start(*self, start);
    AZ_RETURN_IF_FAILED(az_span_copy(copy, span, &copy));
  }

  // update builder size
  self->_internal.length = size_after_replace;

  return AZ_OK;
}

AZ_NODISCARD az_result az_span_append_dtoa(az_span destination, double source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  uint64_t const* const source_bin_rep_view = (uint64_t*)&source;

  if (*source_bin_rep_view == 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(destination, AZ_SPAN_FROM_STR("0"), out_span));
    return AZ_OK;
  }
  *out_span = destination;

  if (source < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(*out_span, AZ_SPAN_FROM_STR("-"), out_span));
    source = -source;
  }

  {
    uint64_t u = (uint64_t)source;
    uint64_t const* const u_bin_rep_view = (uint64_t*)&source;

    if (*source_bin_rep_view == *u_bin_rep_view)
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
        uint8_t dec = (uint8_t)((u / base) + '0');
        u %= base;
        base /= 10;
        AZ_RETURN_IF_FAILED(az_span_append(*out_span, az_span_from_single_item(&dec), out_span));
      } while (1 <= base);
      return AZ_OK;
    }
  }

  // eg. 0.0012
  if (source < 0.001)
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

AZ_INLINE uint8_t _az_decimal_to_ascii(uint8_t d) { return (uint8_t)(('0' + d) & 0xFF); }

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

AZ_NODISCARD az_result
az_span_append_u64toa(az_span destination, uint64_t source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);
  *out_span = destination;

  return _az_span_builder_append_uint64(out_span, source);
}

AZ_NODISCARD az_result az_span_append_i64toa(az_span destination, int64_t source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  if (source < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append(destination, AZ_SPAN_FROM_STR("-"), out_span));
    return _az_span_builder_append_uint64(out_span, (uint64_t)-source);
  }

  return _az_span_builder_append_uint64(out_span, (uint64_t)source);
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

AZ_NODISCARD az_result
az_span_append_u32toa(az_span destination, uint32_t source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);
  return _az_span_builder_append_u32toa(destination, source, out_span);
}

AZ_NODISCARD az_result az_span_append_i32toa(az_span destination, int32_t source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  *out_span = destination;

  if (source < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_span, '-', out_span));
    source = -source;
  }

  return _az_span_builder_append_u32toa(*out_span, (uint32_t)source, out_span);
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

  if (!az_span_is_content_equal(actual_span, expected))
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  // move reader after the expected span (means it was parsed as expected)
  *self = az_span_slice_start(*self, expected_length);

  return AZ_OK;
}

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result _az_scan_until(az_span self, _az_predicate predicate, int32_t* out_index)
{
  for (int32_t index = 0; index < az_span_length(self); ++index)
  {
    az_span s = az_span_slice_start(self, index);
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

AZ_NODISCARD az_result az_span_append_uint8(az_span destination, uint8_t byte, az_span* out_span)
{
  return az_span_append(destination, az_span_init(&byte, 1, 1), out_span);
}

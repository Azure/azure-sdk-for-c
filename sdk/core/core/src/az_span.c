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
  AZ_PRECONDITION_RANGE(0, capacity, INT32_MAX);
  AZ_PRECONDITION_RANGE(0, length, capacity);

  return (az_span){ ._internal = { .ptr = ptr, .length = length, .capacity = capacity, }, };
}

AZ_NODISCARD az_span az_span_from_str(char* str)
{
  AZ_PRECONDITION_NOT_NULL(str);

  int32_t const length = (int32_t)strlen(str);
  return az_span_init((uint8_t*)str, length, length);
}

AZ_NODISCARD az_span az_span_slice(az_span span, int32_t start_index, int32_t end_index)
{
  AZ_PRECONDITION_RANGE(-1, end_index, az_span_capacity(span));
  AZ_PRECONDITION(end_index == -1 || (end_index >= 0 && start_index <= end_index));

  int32_t const capacity = az_span_capacity(span);

  int32_t new_length;

  // If end_index == -1, slice to the end of the span's length.
  if (end_index == -1)
  {
    new_length = az_span_length(span) - start_index;
    // Slicing beyond the length is allowed as long as it is within its capacity.
    // Make sure that length is clamped at the bottom by 0, so it doesn't become negative.
    if (new_length < 0)
    {
      new_length = 0;
    }
  }
  else
  {
    new_length = end_index - start_index;
  }

  return az_span_init(az_span_ptr(span) + start_index, new_length, capacity - start_index);
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
  AZ_PRECONDITION_VALID_SPAN(span, 1, false);
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
  AZ_PRECONDITION_VALID_SPAN(span, 1, false);
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

AZ_NODISCARD int32_t az_span_find(az_span source, az_span target)
{
  /* This function implements the Naive string-search algorithm.
   * The rationale to use this algorithm instead of other potentialy more
   * performing ones (Rabin-Karp, e.g.) is due to no additional space needed.
   * The logic:
   * 1. The function will look into each position of `source` if it contains the same value as the
   * first position of `target`.
   * 2. If it does, it could be that the next bytes in `source` are a perfect match of the remaining
   * bytes of `target`.
   * 3. Being so, it loops through the remaining bytes of `target` and see if they match exactly the
   * next bytes of `source`.
   * 4. If the loop gets to the end (all bytes of `target` are evaluated), it means `target` indeed
   * occurs in that position of `source`.
   * 5. If the loop gets interrupted before cruising through the entire `target`, the function must
   * go back to step 1. from  the next position in `source`.
   *   The loop in 5. gets interrupted if
   *     - a byte in `target` is different than `source`, in the expected corresponding position;
   *     - the loop has reached the end of `source` (and there are still remaing bytes of `target`
   *         to be checked).
   */

  int32_t source_length = az_span_length(source);
  int32_t target_length = az_span_length(target);
  const int32_t target_not_found = -1;

  if (target_length == 0)
  {
    return 0;
  }
  else if (source_length < target_length)
  {
    return target_not_found;
  }
  else
  {
    uint8_t* source_ptr = az_span_ptr(source);
    uint8_t* target_ptr = az_span_ptr(target);

    // This loop traverses `source` position by position (step 1.)
    for (int32_t i = 0; i < (source_length - target_length + 1); i++)
    {
      // This is the check done in step 1. above.
      if (source_ptr[i] == target_ptr[0])
      {
        // The condition in step 2. has been satisfied.
        int32_t j;
        // This is the loop defined in step 3.
        // The loop must be broken if it reaches the ends of `target` (step 3.) OR `source`
        // (step 5.).
        for (j = 1; j < target_length && (i + j) < source_length; j++)
        {
          // Condition defined in step 5.
          if (source_ptr[i + j] != target_ptr[j])
          {
            break;
          }
        }

        if (j == target_length)
        {
          // All bytes in `target` have been checked and matched the corresponding bytes in `source`
          // (from the start point `i`), so this is indeed an instance of `target` in that position
          // of `source` (step 4.).

          return i;
        }
      }
    }
  }

  // If the function hasn't returned before, all positions
  // of `source` have been evaluated but `target` could not be found.
  return target_not_found;
}

az_span az_span_copy(az_span destination, az_span source)
{
  AZ_PRECONDITION(az_span_capacity(destination) >= az_span_length(source));

  int32_t src_len = az_span_length(source);
  uint8_t* ptr = az_span_ptr(destination);
  memmove((void*)ptr, (void const*)az_span_ptr(source), (size_t)src_len);
  return az_span_init(ptr, src_len, az_span_capacity(destination));
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

AZ_NODISCARD az_result
az_span_copy_url_encode(az_span destination, az_span source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);
  AZ_PRECONDITION_VALID_SPAN(destination, 0, true);
  AZ_PRECONDITION_VALID_SPAN(source, 0, true);

  int32_t const input_size = az_span_length(source);

  int32_t result_size = 0;
  for (int32_t i = 0; i < input_size; ++i)
  {
    result_size += should_encode(az_span_ptr(source)[i]) ? 3 : 1;
  }

  if (az_span_capacity(destination) < result_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  uint8_t* p_s = az_span_ptr(source);
  uint8_t* p_d = az_span_ptr(destination);
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
  *out_span = az_span_init(az_span_ptr(destination), s, az_span_capacity(destination));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_span_to_str(char* destination, int32_t destination_max_size, az_span source)
{
  AZ_PRECONDITION_VALID_SPAN(source, 0, true);

  int32_t span_length = az_span_length(source);
  if (span_length + 1 > destination_max_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  memmove((void*)destination, (void const*)az_span_ptr(source), (size_t)span_length);

  destination[span_length] = 0;

  return AZ_OK;
}

az_span az_span_append(az_span destination, az_span source)
{
  AZ_PRECONDITION(
      (az_span_length(destination) + az_span_length(source)) <= az_span_capacity(destination));

  int32_t const dest_length = az_span_length(destination);
  int32_t const dest_capacity = az_span_capacity(destination);
  int32_t const src_length = az_span_length(source);
  int32_t const dest_length_after_appending = dest_length + src_length;

  uint8_t* ptr = az_span_ptr(destination);
  memmove((void*)(&ptr[dest_length]), (void const*)az_span_ptr(source), (size_t)src_length);
  return az_span_init(ptr, dest_length_after_appending, dest_capacity);
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
    *self = az_span_append(*self, span);
    return AZ_OK;
  }
  // replace all content case (no need to make left or right shift, only copy)
  // TODO: Verify and fix this check, if needed.
  if (current_size == replaced_size)
  {
    *self = az_span_copy(*self, span);
    return AZ_OK;
  }

  // get the span needed to be moved before adding a new span
  az_span dst = az_span_slice(*self, start + span_length, current_size);
  // get the span where to move content
  az_span src = az_span_slice(*self, end, current_size);
  {
    // move content left or right so new span can be added
    dst = az_span_copy(dst, src);
    // add the new span
    az_span_copy(az_span_slice(*self, start, -1), span);
  }

  // update builder size
  self->_internal.length = size_after_replace;

  return AZ_OK;
}

AZ_NODISCARD az_result az_span_append_dtoa(az_span destination, double source, az_span* out_span)
{
  AZ_PRECONDITION_NOT_NULL(out_span);

  // Verify to make sure that the destination has at least one byte up front (for either a digit or
  // the sign), since that's is common across all branches.
  // We verify that the destination is large enough for more digits later.
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(destination, 1);

  uint64_t const* const source_bin_rep_view = (uint64_t*)&source;

  if (*source_bin_rep_view == 0)
  {
    *out_span = az_span_append(destination, AZ_SPAN_FROM_STR("0"));
    return AZ_OK;
  }
  *out_span = destination;

  if (source < 0)
  {
    *out_span = az_span_append(*out_span, AZ_SPAN_FROM_STR("-"));
    source = -source;
  }

  {
    uint64_t u = (uint64_t)source;
    uint64_t const* const u_bin_rep_view = (uint64_t*)&source;

    if (*source_bin_rep_view == *u_bin_rep_view)
    {
      uint64_t base = 1;
      int32_t digit_count = 1;
      {
        uint64_t i = u;
        while (10 <= i)
        {
          i /= 10;
          base *= 10;
          digit_count++;
        }
      }

      AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*out_span, digit_count);

      do
      {
        uint8_t dec = (uint8_t)((u / base) + '0');
        u %= base;
        base /= 10;
        *out_span = az_span_append_uint8(*out_span, dec);
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
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*self, 1);

  if (n == 0)
  {
    *self = az_span_append(*self, AZ_SPAN_FROM_STR("0"));
    return AZ_OK;
  }

  uint64_t div = 10000000000000000000ull;
  uint64_t nn = n;
  int32_t digit_count = 20;
  while (nn / div == 0)
  {
    div /= 10;
    digit_count--;
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*self, digit_count);

  while (div > 1)
  {
    uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)(nn / div));
    *self = az_span_append_uint8(*self, value_to_append);
    nn %= div;
    div /= 10;
  }
  uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)nn);
  *self = az_span_append_uint8(*self, value_to_append);
  return AZ_OK;
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
    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(destination, 1);
    *out_span = az_span_append(destination, AZ_SPAN_FROM_STR("-"));
    return _az_span_builder_append_uint64(out_span, (uint64_t)-source);
  }

  return _az_span_builder_append_uint64(out_span, (uint64_t)source);
}

static AZ_NODISCARD az_result
_az_span_builder_append_u32toa(az_span self, uint32_t n, az_span* out_span)
{
  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(self, 1);

  if (n == 0)
  {
    *out_span = az_span_append_uint8(self, '0');
    return AZ_OK;
  }

  uint32_t div = 1000000000;
  uint32_t nn = n;
  int32_t digit_count = 10;
  while (nn / div == 0)
  {
    div /= 10;
    digit_count--;
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(self, digit_count);

  *out_span = self;

  while (div > 1)
  {
    uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)(nn / div));
    *out_span = az_span_append_uint8(*out_span, value_to_append);

    nn %= div;
    div /= 10;
  }

  uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)nn);
  *out_span = az_span_append_uint8(*out_span, value_to_append);
  return AZ_OK;
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
    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*out_span, 1);
    *out_span = az_span_append_uint8(*out_span, '-');
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

az_span az_span_append_uint8(az_span destination, uint8_t byte)
{
  return az_span_append(destination, az_span_init(&byte, 1, 1));
}

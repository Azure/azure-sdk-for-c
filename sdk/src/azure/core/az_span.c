// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_span_private.h"
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <azure/core/_az_cfg.h>

// The maximum integer value that can be stored in a double without losing precision (2^53 - 1)
// An IEEE 64-bit double has 52 bits of mantissa
#define _az_MAX_SAFE_INTEGER 9007199254740991

#ifndef AZ_NO_PRECONDITION_CHECKING
// Note: If you are modifying this method, make sure to modify the inline version in the az_span.h
// file as well.
AZ_NODISCARD az_span az_span_create(uint8_t* ptr, int32_t size)
{
  // If ptr is not null, then:
  //   size >= 0
  // Otherwise, if ptr is null, then:
  //   size == 0
  _az_PRECONDITION((ptr != NULL && size >= 0) || (ptr + (uint32_t)size == 0));

  return (az_span){ ._internal = { .ptr = ptr, .size = size } };
}
#endif // AZ_NO_PRECONDITION_CHECKING

AZ_NODISCARD az_span az_span_create_from_str(char* str)
{
  _az_PRECONDITION_NOT_NULL(str);

  // Avoid passing in null pointer to strlen to avoid memory access violation.
  if (str == NULL)
  {
    return AZ_SPAN_NULL;
  }

  int32_t const length = (int32_t)strlen(str);

  _az_PRECONDITION(length >= 0);

  return az_span_create((uint8_t*)str, length);
}

AZ_NODISCARD az_span az_span_slice(az_span span, int32_t start_index, int32_t end_index)
{
  _az_PRECONDITION_VALID_SPAN(span, 0, true);

  // The following set of preconditions validate that:
  //    0 <= end_index <= span.size
  // And
  //    0 <= start_index <= end_index
  _az_PRECONDITION_RANGE(0, end_index, az_span_size(span));
  _az_PRECONDITION((uint32_t)start_index <= (uint32_t)end_index);

  return az_span_create(az_span_ptr(span) + start_index, end_index - start_index);
}

AZ_NODISCARD az_span az_span_slice_to_end(az_span span, int32_t start_index)
{
  return az_span_slice(span, start_index, az_span_size(span));
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
  int32_t const size = az_span_size(span1);
  if (size != az_span_size(span2))
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

AZ_NODISCARD az_result az_span_atou64(az_span source, uint64_t* out_number)
{
  _az_PRECONDITION_VALID_SPAN(source, 1, false);
  _az_PRECONDITION_NOT_NULL(out_number);

  int32_t const span_size = az_span_size(source);

  if (span_size < 1)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // If the first character is not a digit or an optional + sign, return error.
  int32_t starting_index = 0;
  uint8_t* source_ptr = az_span_ptr(source);
  uint8_t next_byte = source_ptr[0];

  if (!isdigit(next_byte))
  {
    // There must be another byte after a sign.
    // The loop below checks that it must be a digit.
    if (next_byte != '+' || span_size < 2)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    starting_index++;
  }

  uint64_t value = 0;

  for (int32_t i = starting_index; i < span_size; ++i)
  {
    next_byte = source_ptr[i];
    if (!isdigit(next_byte))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    uint64_t const d = (uint64_t)next_byte - '0';

    // Check whether the next digit will cause an integer overflow.
    // Before actually doing the math below, this is checking whether value * 10 + d > UINT64_MAX.
    if ((UINT64_MAX - d) / 10 < value)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    value = value * 10 + d;
  }

  *out_number = value;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_atou32(az_span source, uint32_t* out_number)
{
  _az_PRECONDITION_VALID_SPAN(source, 1, false);
  _az_PRECONDITION_NOT_NULL(out_number);

  int32_t const span_size = az_span_size(source);

  if (span_size < 1)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // If the first character is not a digit or an optional + sign, return error.
  int32_t starting_index = 0;
  uint8_t* source_ptr = az_span_ptr(source);
  uint8_t next_byte = source_ptr[0];

  if (!isdigit(next_byte))
  {
    // There must be another byte after a sign.
    // The loop below checks that it must be a digit.
    if (next_byte != '+' || span_size < 2)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    starting_index++;
  }

  uint32_t value = 0;

  for (int32_t i = starting_index; i < span_size; ++i)
  {
    next_byte = source_ptr[i];
    if (!isdigit(next_byte))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    uint32_t const d = (uint32_t)next_byte - '0';

    // Check whether the next digit will cause an integer overflow.
    // Before actually doing the math below, this is checking whether value * 10 + d > UINT32_MAX.
    if ((UINT32_MAX - d) / 10 < value)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    value = value * 10 + d;
  }

  *out_number = value;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_atoi64(az_span source, int64_t* out_number)
{
  _az_PRECONDITION_VALID_SPAN(source, 1, false);
  _az_PRECONDITION_NOT_NULL(out_number);

  int32_t const span_size = az_span_size(source);

  if (span_size < 1)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // If the first character is not a digit, - sign, or an optional + sign, return error.
  int32_t starting_index = 0;
  uint8_t* source_ptr = az_span_ptr(source);
  uint8_t next_byte = source_ptr[0];
  int64_t sign = 1;

  if (!isdigit(next_byte))
  {
    // There must be another byte after a sign.
    // The loop below checks that it must be a digit.
    if (next_byte != '+')
    {
      if (next_byte != '-')
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      sign = -1;
    }
    if (span_size < 2)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    starting_index++;
  }

  // if sign < 0, (-1 * sign + 1) / 2 = 1
  // else, (-1 * sign + 1) / 2 = 0
  // This is necessary to correctly account for the fact that the absolute value of INT64_MIN is 1
  // more than than the absolute value of INT64_MAX.
  uint64_t sign_factor = (uint64_t)(-1 * sign + 1) / 2;

  // Using unsigned int while parsing to account for potential overflow.
  uint64_t value = 0;

  for (int32_t i = starting_index; i < span_size; ++i)
  {
    next_byte = source_ptr[i];
    if (!isdigit(next_byte))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    uint64_t const d = (uint64_t)next_byte - '0';

    // Check whether the next digit will cause an integer overflow.
    // Before actually doing the math below, this is checking whether value * 10 + d > INT64_MAX, or
    // in the case of negative numbers, checking whether value * 10 + d > INT64_MAX + 1.
    if ((uint64_t)(INT64_MAX - d + sign_factor) / 10 < value)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    value = value * 10 + d;
  }

  *out_number = (int64_t)value * sign;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_atoi32(az_span source, int32_t* out_number)
{
  _az_PRECONDITION_VALID_SPAN(source, 1, false);
  _az_PRECONDITION_NOT_NULL(out_number);

  int32_t const span_size = az_span_size(source);

  if (span_size < 1)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // If the first character is not a digit, - sign, or an optional + sign, return error.
  int32_t starting_index = 0;
  uint8_t* source_ptr = az_span_ptr(source);
  uint8_t next_byte = source_ptr[0];
  int32_t sign = 1;

  if (!isdigit(next_byte))
  {
    // There must be another byte after a sign.
    // The loop below checks that it must be a digit.
    if (next_byte != '+')
    {
      if (next_byte != '-')
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      sign = -1;
    }
    if (span_size < 2)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    starting_index++;
  }

  // if sign < 0, (-1 * sign + 1) / 2 = 1
  // else, (-1 * sign + 1) / 2 = 0
  // This is necessary to correctly account for the fact that the absolute value of INT32_MIN is 1
  // more than than the absolute value of INT32_MAX.
  uint32_t sign_factor = (uint32_t)(-1 * sign + 1) / 2;

  // Using unsigned int while parsing to account for potential overflow.
  uint32_t value = 0;

  for (int32_t i = starting_index; i < span_size; ++i)
  {
    next_byte = source_ptr[i];
    if (!isdigit(next_byte))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    uint32_t const d = (uint32_t)next_byte - '0';

    // Check whether the next digit will cause an integer overflow.
    // Before actually doing the math below, this is checking whether value * 10 + d > INT32_MAX, or
    // in the case of negative numbers, checking whether value * 10 + d > INT32_MAX + 1.
    if ((uint32_t)(INT32_MAX - d + sign_factor) / 10 < value)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    value = value * 10 + d;
  }

  *out_number = (int32_t)value * sign;
  return AZ_OK;
}

static bool _is_valid_start_of_double(uint8_t first_byte)
{
  // ".123", "  123", "nan", or "inf" are considered invalid
  bool result = isdigit(first_byte) || first_byte == '+' || first_byte == '-';

  return result;
}

// Disable the following warning just for this particular use case.
// C4996: 'sscanf': This function or variable may be unsafe. Consider using sscanf_s instead.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

AZ_NODISCARD az_result az_span_atod(az_span source, double* out_number)
{
  _az_PRECONDITION_VALID_SPAN(source, 1, false);
  _az_PRECONDITION_NOT_NULL(out_number);

  int32_t size = az_span_size(source);

  _az_PRECONDITION_RANGE(1, size, 99);

  // This check is necessary to prevent sscanf from reading bytes past the end of the span, when the
  // span might contain whitespace or other invalid bytes at the start.
  uint8_t* source_ptr = az_span_ptr(source);
  if (size < 1 || !_is_valid_start_of_double(source_ptr[0]))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  // Stack based string to allow thread-safe mutation.
  // The length is 8 to allow space for the null-terminating character.
  char format[8] = "%00lf%n";

  // Starting at 1 to skip the '%' character
  format[1] = (char)((size / 10) + '0');
  format[2] = (char)((size % 10) + '0');

  int32_t chars_consumed = 0;
  int32_t n = sscanf((char*)source_ptr, format, out_number, &chars_consumed);

  // Success if the entire source was consumed by sscanf and it set the out_number argument.
  return (size == chars_consumed && n == 1 && _az_isfinite(*out_number)) ? AZ_OK
                                                                         : AZ_ERROR_UNEXPECTED_CHAR;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

AZ_NODISCARD int32_t az_span_find(az_span source, az_span target)
{
  /* This function implements the Naive string-search algorithm.
   * The rationale to use this algorithm instead of other potentially more
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
   *     - the loop has reached the end of `source` (and there are still remaining bytes of `target`
   *         to be checked).
   */

  int32_t source_size = az_span_size(source);
  int32_t target_size = az_span_size(target);
  const int32_t target_not_found = -1;

  if (target_size == 0)
  {
    return 0;
  }
  else if (source_size < target_size)
  {
    return target_not_found;
  }
  else
  {
    uint8_t* source_ptr = az_span_ptr(source);
    uint8_t* target_ptr = az_span_ptr(target);

    // This loop traverses `source` position by position (step 1.)
    for (int32_t i = 0; i < (source_size - target_size + 1); i++)
    {
      // This is the check done in step 1. above.
      if (source_ptr[i] == target_ptr[0])
      {
        // The condition in step 2. has been satisfied.
        int32_t j;
        // This is the loop defined in step 3.
        // The loop must be broken if it reaches the ends of `target` (step 3.) OR `source`
        // (step 5.).
        for (j = 1; j < target_size && (i + j) < source_size; j++)
        {
          // Condition defined in step 5.
          if (source_ptr[i + j] != target_ptr[j])
          {
            break;
          }
        }

        if (j == target_size)
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
  int32_t src_size = az_span_size(source);

  _az_PRECONDITION_VALID_SPAN(destination, src_size, false);

  if (src_size == 0)
  {
    return destination;
  }

  // Even though the contract of this method is that the destination must be larger than source, cap
  // the data move if the source is too large, to avoid memory corruption.
  int32_t dest_size = az_span_size(destination);
  if (src_size > dest_size)
  {
    src_size = dest_size;
  }

  uint8_t* ptr = az_span_ptr(destination);
  memmove((void*)ptr, (void const*)az_span_ptr(source), (size_t)src_size);

  return az_span_slice_to_end(destination, src_size);
}

az_span az_span_copy_u8(az_span destination, uint8_t byte)
{
  _az_PRECONDITION_VALID_SPAN(destination, 1, false);

  // Even though the contract of the method is that the destination must be at least 1 byte large,
  // no-op if it is empty to avoid memory corruption.
  int32_t dest_size = az_span_size(destination);
  if (dest_size < 1)
  {
    return destination;
  }

  uint8_t* dst_ptr = az_span_ptr(destination);
  dst_ptr[0] = byte;
  return az_span_create(dst_ptr + 1, dest_size - 1);
}

void az_span_to_str(char* destination, int32_t destination_max_size, az_span source)
{
  _az_PRECONDITION_NOT_NULL(destination);
  _az_PRECONDITION(destination_max_size > 0);

  // Implementations of memmove generally do the right thing when number of bytes to move is 0, even
  // if the ptr is null, but given the behavior is documented to be undefined, we disallow it as a
  // precondition.
  _az_PRECONDITION_VALID_SPAN(source, 0, false);

  int32_t size_to_write = az_span_size(source);

  _az_PRECONDITION(size_to_write < destination_max_size);

  // Even though the contract of this method is that the destination_max_size must be larger than
  // source to be able to copy all of the source to the char buffer including an extra null
  // terminating character, cap the data move if the source is too large, to avoid memory
  // corruption.
  if (size_to_write >= destination_max_size)
  {
    // Leave enough space for the null terminator.
    size_to_write = destination_max_size - 1;

    // If destination_max_size was 0, we don't want size_to_write to be negative and
    // corrupt data before the destination pointer.
    if (size_to_write < 0)
    {
      size_to_write = 0;
    }
  }

  _az_PRECONDITION(size_to_write >= 0);

  memmove((void*)destination, (void const*)az_span_ptr(source), (size_t)size_to_write);
  destination[size_to_write] = 0;
}

/**
 * @brief Replace all contents from a starting position to an end position with the content of a
 * provided span
 *
 * @param destination src span where to replace content
 * @param start starting position where to replace
 * @param end end position where to replace
 * @param replacement content to use for replacement
 * @return AZ_NODISCARD az_span_replace
 */
AZ_NODISCARD az_result _az_span_replace(
    az_span destination,
    int32_t current_size,
    int32_t start,
    int32_t end,
    az_span replacement)
{
  int32_t const replacement_size = az_span_size(replacement);
  int32_t const replaced_size = end - start;
  int32_t const size_after_replace = current_size - replaced_size + replacement_size;

  // Start and end position must be within the destination span and be positive.
  // Start position must be less or equal than end position.
  if ((uint32_t)start > (uint32_t)current_size || (uint32_t)end > (uint32_t)current_size
      || start > end)
  {
    return AZ_ERROR_ARG;
  };

  // The replaced size must be less or equal to current span size. Can't replace more than what
  // current is available. The size after replacing must be less than or equal to the size of
  // destination span.
  if (replaced_size > current_size || size_after_replace > az_span_size(destination))
  {
    return AZ_ERROR_ARG;
  };

  // insert at the end case (no need to make left or right shift)
  if (start == current_size)
  {
    destination = az_span_copy(az_span_slice_to_end(destination, start), replacement);
    return AZ_OK;
  }
  // replace all content case (no need to make left or right shift, only copy)
  // TODO: Verify and fix this check, if needed.
  if (current_size == replaced_size)
  {
    destination = az_span_copy(destination, replacement);
    return AZ_OK;
  }

  // get the span needed to be moved before adding a new span
  az_span dst = az_span_slice_to_end(destination, start + replacement_size);
  // get the span where to move content
  az_span src = az_span_slice(destination, end, current_size);
  {
    // move content left or right so new span can be added
    az_span_copy(dst, src);
    // add the new span
    az_span_copy(az_span_slice_to_end(destination, start), replacement);
  }

  return AZ_OK;
}

AZ_INLINE uint8_t _az_decimal_to_ascii(uint8_t d) { return (uint8_t)(('0' + d) & 0xFF); }

static AZ_NODISCARD az_result _az_span_builder_append_uint64(az_span* ref_span, uint64_t n)
{
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(*ref_span, 1);

  if (n == 0)
  {
    *ref_span = az_span_copy_u8(*ref_span, '0');
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

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(*ref_span, digit_count);

  while (div > 1)
  {
    uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)(nn / div));
    *ref_span = az_span_copy_u8(*ref_span, value_to_append);
    nn %= div;
    div /= 10;
  }
  uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)nn);
  *ref_span = az_span_copy_u8(*ref_span, value_to_append);
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_u64toa(az_span destination, uint64_t source, az_span* out_span)
{
  _az_PRECONDITION_VALID_SPAN(destination, 0, false);
  _az_PRECONDITION_NOT_NULL(out_span);
  *out_span = destination;

  return _az_span_builder_append_uint64(out_span, source);
}

AZ_NODISCARD az_result az_span_i64toa(az_span destination, int64_t source, az_span* out_span)
{
  _az_PRECONDITION_VALID_SPAN(destination, 0, false);
  _az_PRECONDITION_NOT_NULL(out_span);

  if (source < 0)
  {
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(destination, 1);
    *out_span = az_span_copy_u8(destination, '-');
    return _az_span_builder_append_uint64(out_span, (uint64_t)-source);
  }

  // make out_span point to destination before trying to write on it (might be an empty az_span or
  // pointing else where)
  *out_span = destination;
  return _az_span_builder_append_uint64(out_span, (uint64_t)source);
}

static AZ_NODISCARD az_result
_az_span_builder_append_u32toa(az_span destination, uint32_t n, az_span* out_span)
{
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(destination, 1);

  if (n == 0)
  {
    *out_span = az_span_copy_u8(destination, '0');
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

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(destination, digit_count);

  *out_span = destination;

  while (div > 1)
  {
    uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)(nn / div));
    *out_span = az_span_copy_u8(*out_span, value_to_append);

    nn %= div;
    div /= 10;
  }

  uint8_t value_to_append = _az_decimal_to_ascii((uint8_t)nn);
  *out_span = az_span_copy_u8(*out_span, value_to_append);
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_u32toa(az_span destination, uint32_t source, az_span* out_span)
{
  _az_PRECONDITION_VALID_SPAN(destination, 0, false);
  _az_PRECONDITION_NOT_NULL(out_span);
  return _az_span_builder_append_u32toa(destination, source, out_span);
}

AZ_NODISCARD az_result az_span_i32toa(az_span destination, int32_t source, az_span* out_span)
{
  _az_PRECONDITION_VALID_SPAN(destination, 0, false);
  _az_PRECONDITION_NOT_NULL(out_span);

  *out_span = destination;

  if (source < 0)
  {
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(*out_span, 1);
    *out_span = az_span_copy_u8(*out_span, '-');
    source = -source;
  }

  return _az_span_builder_append_u32toa(*out_span, (uint32_t)source, out_span);
}

AZ_NODISCARD az_result
az_span_dtoa(az_span destination, double source, int32_t fractional_digits, az_span* out_span)
{
  _az_PRECONDITION_VALID_SPAN(destination, 0, false);
  // Inputs that are either positive or negative infinity, or not a number, are not supported.
  _az_PRECONDITION(_az_isfinite(source));
  _az_PRECONDITION_RANGE(0, fractional_digits, _az_MAX_SUPPORTED_FRACTIONAL_DIGITS);
  _az_PRECONDITION_NOT_NULL(out_span);

  *out_span = destination;

  // The input is either positive or negative infinity, or not a number.
  if (!_az_isfinite(source))
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  if (source < 0)
  {
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(*out_span, 1);
    *out_span = az_span_copy_u8(*out_span, '-');
    source = -source;
  }

  double integer_part = 0;
  double after_decimal_part = modf(source, &integer_part);

  if (integer_part > _az_MAX_SAFE_INTEGER)
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  // The double to uint64_t cast should be safe without loss of precision.
  // Append the integer part.
  AZ_RETURN_IF_FAILED(_az_span_builder_append_uint64(out_span, (uint64_t)integer_part));

  // Only print decimal digits if the user asked for at least one to be printed.
  // Or if the decimal part is non-zero.
  if (fractional_digits <= 0)
  {
    return AZ_OK;
  }

  // Clamp the fractional digits to the supported maximum value of 15.
  if (fractional_digits > _az_MAX_SUPPORTED_FRACTIONAL_DIGITS)
  {
    fractional_digits = _az_MAX_SUPPORTED_FRACTIONAL_DIGITS;
  }

  int32_t leading_zeros = 0;
  double shifted_fractional = after_decimal_part;
  for (int32_t d = 0; d < fractional_digits; d++)
  {
    shifted_fractional *= 10;

    // Any decimal component that is less than 0.1, when multiplied by 10, will be less than 1,
    // which indicate a leading zero is present after the decimal point. For example, the decimal
    // part could be 0.00, 0.09, 0.00010, etc.
    if (shifted_fractional < 1)
    {
      leading_zeros++;
      continue;
    }
  }

  double shifted_fractional_integer_part = 0;
  double unused = modf(shifted_fractional, &shifted_fractional_integer_part);
  (void)unused;

  // Since the maximum allowed fractional_digits is 15, this is guaranteed to be true.
  _az_PRECONDITION(shifted_fractional_integer_part <= _az_MAX_SAFE_INTEGER);

  // The double to uint64_t cast should be safe without loss of precision.
  uint64_t fractional_part = (uint64_t)shifted_fractional_integer_part;

  // If there is no fractional part (at least within the number of fractional digits the user
  // specified), or if they were all non-significant zeros, don't print the decimal point or any
  // trailing zeros.
  if (fractional_part == 0)
  {
    return AZ_OK;
  }

  // Remove trailing zeros of the fraction part that don't need to be printed since they aren't
  // significant.
  while (fractional_part % 10 == 0)
  {
    fractional_part /= 10;
  }

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(*out_span, 1 + leading_zeros);
  *out_span = az_span_copy_u8(*out_span, '.');

  for (int32_t z = 0; z < leading_zeros; z++)
  {
    *out_span = az_span_copy_u8(*out_span, '0');
  }

  // Append the fractional part.
  return _az_span_builder_append_uint64(out_span, fractional_part);
}

// TODO: pass az_span by value
AZ_NODISCARD az_result _az_is_expected_span(az_span* ref_span, az_span expected)
{
  az_span actual_span = { 0 };

  int32_t expected_size = az_span_size(expected);

  // EOF because ref_span is smaller than the expected span
  if (expected_size > az_span_size(*ref_span))
  {
    return AZ_ERROR_UNEXPECTED_END;
  }

  actual_span = az_span_slice(*ref_span, 0, expected_size);

  if (!az_span_is_content_equal(actual_span, expected))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }
  // move reader after the expected span (means it was parsed as expected)
  *ref_span = az_span_slice_to_end(*ref_span, expected_size);

  return AZ_OK;
}

AZ_NODISCARD az_span _az_span_trim_whitespace(az_span source)
{
  // Trim from end after trim from start
  return _az_span_trim_whitespace_from_end(_az_span_trim_whitespace_from_start(source));
}

AZ_NODISCARD AZ_INLINE bool _az_is_whitespace(uint8_t c)
{
  switch (c)
  {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
  }
  return false;
}

typedef enum
{
  LEFT = 0,
  RIGHT = 1,
} az_span_trim_side;

// Return a trim az_span. Depending on arg side, function will trim left of right
AZ_NODISCARD static az_span _az_span_trim_side(az_span source, az_span_trim_side side)
{
  int32_t increment = 1;
  uint8_t* source_ptr = az_span_ptr(source);
  int32_t source_size = az_span_size(source);

  if (side == RIGHT)
  {
    increment = -1; // Set increment to be decremental for moving ptr
    source_ptr += ((size_t)source_size - 1); // Set initial position to the end
  }

  // loop source, just to make sure staying within the size range
  int32_t index = 0;
  for (; index < source_size; index++)
  {
    if (!_az_is_whitespace(*source_ptr))
    {
      break;
    }
    // update ptr to next position
    source_ptr += increment;
  }

  // return the slice depending on side
  if (side == RIGHT)
  {
    // calculate index from right.
    index = source_size - index;
    return az_span_slice(source, 0, index);
  }

  return az_span_slice_to_end(source, index); // worst case index would be source_size
}

AZ_NODISCARD az_span _az_span_trim_whitespace_from_start(az_span source)
{
  return _az_span_trim_side(source, LEFT);
}

AZ_NODISCARD az_span _az_span_trim_whitespace_from_end(az_span source)
{
  return _az_span_trim_side(source, RIGHT);
}

AZ_NODISCARD AZ_INLINE bool _az_span_url_should_encode(uint8_t c)
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

AZ_NODISCARD int32_t _az_span_url_encode_calc_length(az_span source)
{
  _az_PRECONDITION_VALID_SPAN(source, 0, true);
  // trying to calculate the number of bytes to encode more than INT32_MAX / 3 might overflow an
  // int32 and return an erroneous number back
  _az_PRECONDITION_RANGE(0, az_span_size(source), INT32_MAX / 3);

  int32_t const source_size = az_span_size(source);
  if (source_size == 0)
  {
    return 0;
  }

  uint8_t* const src_ptr = az_span_ptr(source);
  int32_t required_symbols_to_be_added = 0;
  int32_t src_idx = 0;
  do
  {
    uint8_t c = src_ptr[src_idx];
    if (_az_span_url_should_encode(c))
    {
      // Adding '%' plus 2 digits (minus 1 as original symbol is counted as 1)
      required_symbols_to_be_added += 2;
    }
    ++src_idx;
  } while (src_idx < source_size);

  return source_size + required_symbols_to_be_added;
}

AZ_NODISCARD az_result _az_span_url_encode(az_span destination, az_span source, int32_t* out_length)
{
  _az_PRECONDITION_NOT_NULL(out_length);
  _az_PRECONDITION_VALID_SPAN(source, 0, true);

  int32_t const source_size = az_span_size(source);
  _az_PRECONDITION_VALID_SPAN(destination, source_size, false);

  _az_PRECONDITION_NO_OVERLAP_SPANS(destination, source);

  if (source_size == 0)
  {
    *out_length = 0;
    return AZ_OK;
  }

  int32_t const destination_size = az_span_size(destination);
  if (destination_size < source_size)
  {
    *out_length = 0;
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  // "Extra space" is measured in units of 2 additional characters
  // per single source character ('/' => "%2F").
  int32_t const extra_space_have = (destination_size - source_size) / 2;

  uint8_t* const dest_begin = az_span_ptr(destination);

  uint8_t* const src_ptr = az_span_ptr(source);
  uint8_t* dest_ptr = dest_begin;

  if (extra_space_have >= source_size)
  {
    // We know that there's enough space even if every character gets encoded.
    int32_t src_idx = 0;
    do
    {
      uint8_t c = src_ptr[src_idx];
      if (!_az_span_url_should_encode(c))
      {
        *dest_ptr = c;
        ++dest_ptr;
      }
      else
      {
        dest_ptr[0] = '%';
        dest_ptr[1] = _az_number_to_upper_hex(c >> 4);
        dest_ptr[2] = _az_number_to_upper_hex(c & 0x0F);
        dest_ptr += 3;
      }

      ++src_idx;
    } while (src_idx < source_size);
  }
  else
  {
    // We may or may not have enough space, given whether the input needs much encoding or not.
    int32_t extra_space_used = 0;
    int32_t src_idx = 0;
    do
    {
      uint8_t c = src_ptr[src_idx];
      if (!_az_span_url_should_encode(c))
      {
        *dest_ptr = c;
        ++dest_ptr;
      }
      else
      {
        ++extra_space_used;
        if (extra_space_used > extra_space_have)
        {
          *out_length = 0;
          return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
        }

        dest_ptr[0] = '%';
        dest_ptr[1] = _az_number_to_upper_hex(c >> 4);
        dest_ptr[2] = _az_number_to_upper_hex(c & 0x0F);
        dest_ptr += 3;
      }

      ++src_idx;
    } while (src_idx < source_size);
  }

  *out_length = (int32_t)(dest_ptr - dest_begin);
  return AZ_OK;
}

AZ_NODISCARD az_span _az_span_token(az_span source, az_span delimiter, az_span* out_remainder)
{
  _az_PRECONDITION_VALID_SPAN(delimiter, 1, false);
  _az_PRECONDITION_NOT_NULL(out_remainder);

  if (az_span_size(source) == 0)
  {
    return AZ_SPAN_NULL;
  }

  int32_t index = az_span_find(source, delimiter);

  if (index != -1)
  {
    *out_remainder = az_span_slice(source, index + az_span_size(delimiter), az_span_size(source));

    return az_span_slice(source, 0, index);
  }
  else
  {
    *out_remainder = AZ_SPAN_NULL;

    return source;
  }
}

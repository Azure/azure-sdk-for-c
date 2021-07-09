// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_base64.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

// The maximum integer length of binary data that can be encoded into base 64 text and still fit
// into an az_span which has an int32_t length, i.e. (INT32_MAX / 4) * 3;
#define _az_MAX_SAFE_ENCODED_LENGTH 1610612733

#define _az_ENCODING_PAD '='

static char const _az_base64_encode_array[65]
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int8_t const _az_base64_decode_array[256] = {
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  62,
  -1,
  -1,
  -1,
  63, // 62 is placed at index 43 (for +), 63 at index 47 (for /)
  52,
  53,
  54,
  55,
  56,
  57,
  58,
  59,
  60,
  61,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1, // 52-61 are placed at index 48-57 (for 0-9), 64 at index 61 (for =)
  -1,
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16,
  17,
  18,
  19,
  20,
  21,
  22,
  23,
  24,
  25,
  -1,
  -1,
  -1,
  -1,
  -1, // 0-25 are placed at index 65-90 (for A-Z)
  -1,
  26,
  27,
  28,
  29,
  30,
  31,
  32,
  33,
  34,
  35,
  36,
  37,
  38,
  39,
  40,
  41,
  42,
  43,
  44,
  45,
  46,
  47,
  48,
  49,
  50,
  51,
  -1,
  -1,
  -1,
  -1,
  -1, // 26-51 are placed at index 97-122 (for a-z)
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1, // Bytes over 122 ('z') are invalid and cannot be decoded. Hence, padding the map with 255,
      // which indicates invalid input
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
};

static AZ_NODISCARD int32_t _az_base64_encode(uint8_t* three_bytes)
{
  int32_t i = (*three_bytes << 16) | (*(three_bytes + 1) << 8) | *(three_bytes + 2);

  int32_t i0 = _az_base64_encode_array[i >> 18];
  int32_t i1 = _az_base64_encode_array[(i >> 12) & 0x3F];
  int32_t i2 = _az_base64_encode_array[(i >> 6) & 0x3F];
  int32_t i3 = _az_base64_encode_array[i & 0x3F];

  return i0 | (i1 << 8) | (i2 << 16) | (i3 << 24);
}

static AZ_NODISCARD int32_t _az_base64_encode_and_pad_one(uint8_t* two_bytes)
{
  int32_t i = (*two_bytes << 16) | (*(two_bytes + 1) << 8);

  int32_t i0 = _az_base64_encode_array[i >> 18];
  int32_t i1 = _az_base64_encode_array[(i >> 12) & 0x3F];
  int32_t i2 = _az_base64_encode_array[(i >> 6) & 0x3F];

  return i0 | (i1 << 8) | (i2 << 16) | (_az_ENCODING_PAD << 24);
}

static AZ_NODISCARD int32_t _az_base64_encode_and_pad_two(uint8_t* one_byte)
{
  int32_t i = (*one_byte << 8);

  int32_t i0 = _az_base64_encode_array[i >> 10];
  int32_t i1 = _az_base64_encode_array[(i >> 4) & 0x3F];

  return i0 | (i1 << 8) | (_az_ENCODING_PAD << 16) | (_az_ENCODING_PAD << 24);
}

static void _az_base64_write_int_as_four_bytes(uint8_t* destination, int32_t value)
{
  *(destination + 3) = (uint8_t)((value >> 24) & 0xFF);
  *(destination + 2) = (uint8_t)((value >> 16) & 0xFF);
  *(destination + 1) = (uint8_t)((value >> 8) & 0xFF);
  *(destination + 0) = (uint8_t)(value & 0xFF);
}

AZ_NODISCARD az_result
az_base64_encode(az_span destination_base64_text, az_span source_bytes, int32_t* out_written)
{
  _az_PRECONDITION_VALID_SPAN(destination_base64_text, 4, false);
  _az_PRECONDITION_VALID_SPAN(source_bytes, 1, false);
  _az_PRECONDITION_NOT_NULL(out_written);

  int32_t source_length = az_span_size(source_bytes);
  uint8_t* source_ptr = az_span_ptr(source_bytes);

  int32_t destination_length = az_span_size(destination_base64_text);
  uint8_t* destination_ptr = az_span_ptr(destination_base64_text);

  if (destination_length < az_base64_get_max_encoded_size(source_length))
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  int32_t source_index = 0;
  int32_t result = 0;

  while (source_index < source_length - 2)
  {
    result = _az_base64_encode(source_ptr + source_index);
    _az_base64_write_int_as_four_bytes(destination_ptr, result);
    destination_ptr += 4;
    source_index += 3;
  }

  if (source_index == source_length - 1)
  {
    result = _az_base64_encode_and_pad_two(source_ptr + source_index);
    _az_base64_write_int_as_four_bytes(destination_ptr, result);
    destination_ptr += 4;
    source_index += 1;
  }
  else if (source_index == source_length - 2)
  {
    result = _az_base64_encode_and_pad_one(source_ptr + source_index);
    _az_base64_write_int_as_four_bytes(destination_ptr, result);
    destination_ptr += 4;
    source_index += 2;
  }

  *out_written = (int32_t)(destination_ptr - az_span_ptr(destination_base64_text));
  return AZ_OK;
}

AZ_NODISCARD int32_t az_base64_get_max_encoded_size(int32_t source_bytes_size)
{
  _az_PRECONDITION_RANGE(0, source_bytes_size, _az_MAX_SAFE_ENCODED_LENGTH);
  return (((source_bytes_size + 2) / 3) * 4);
}

static AZ_NODISCARD int32_t _az_base64_decode(uint8_t* encoded_bytes)
{
  int32_t i0 = *encoded_bytes;
  int32_t i1 = *(encoded_bytes + 1);
  int32_t i2 = *(encoded_bytes + 2);
  int32_t i3 = *(encoded_bytes + 3);

  i0 = _az_base64_decode_array[i0];
  i1 = _az_base64_decode_array[i1];
  i2 = _az_base64_decode_array[i2];
  i3 = _az_base64_decode_array[i3];

  i0 <<= 18;
  i1 <<= 12;
  i2 <<= 6;

  i0 |= i3;
  i1 |= i2;

  i0 |= i1;
  return i0;
}

static void _az_base64_write_three_low_order_bytes(uint8_t* destination, int32_t value)
{
  *destination = (uint8_t)(value >> 16);
  *(destination + 1) = (uint8_t)(value >> 8);
  *(destination + 2) = (uint8_t)(value);
}

AZ_NODISCARD az_result
az_base64_decode(az_span destination_bytes, az_span source_base64_text, int32_t* out_written)
{
  _az_PRECONDITION_VALID_SPAN(destination_bytes, 1, false);
  _az_PRECONDITION_VALID_SPAN(source_base64_text, 4, false);
  _az_PRECONDITION_NOT_NULL(out_written);

  int32_t source_length = az_span_size(source_base64_text);
  uint8_t* source_ptr = az_span_ptr(source_base64_text);

  int32_t destination_length = az_span_size(destination_bytes);
  uint8_t* destination_ptr = az_span_ptr(destination_bytes);

  // The input must be non-empty and a multiple of 4 to be valid.
  if (source_length == 0 || source_length % 4 != 0)
  {
    return AZ_ERROR_UNEXPECTED_END;
  }

  if (destination_length < az_base64_get_max_decoded_size(source_length) - 2)
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  int32_t source_index = 0;
  int32_t destination_index = 0;

  while (source_index < source_length - 4)
  {
    int32_t result = _az_base64_decode(source_ptr + source_index);
    if (result < 0)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    _az_base64_write_three_low_order_bytes(destination_ptr, result);
    destination_ptr += 3;
    destination_index += 3;
    source_index += 4;
  }

  // We are guaranteed to have an input with at least 4 bytes at this point, with a size that is a
  // multiple of 4.
  int32_t i0 = *(source_ptr + source_length - 4);
  int32_t i1 = *(source_ptr + source_length - 3);
  int32_t i2 = *(source_ptr + source_length - 2);
  int32_t i3 = *(source_ptr + source_length - 1);

  i0 = _az_base64_decode_array[i0];
  i1 = _az_base64_decode_array[i1];

  i0 <<= 18;
  i1 <<= 12;

  i0 |= i1;

  if (i3 != _az_ENCODING_PAD)
  {
    i2 = _az_base64_decode_array[i2];
    i3 = _az_base64_decode_array[i3];

    i2 <<= 6;

    i0 |= i3;
    i0 |= i2;

    if (i0 < 0)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    if (destination_index > destination_length - 3)
    {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    }
    _az_base64_write_three_low_order_bytes(destination_ptr, i0);
    destination_ptr += 3;
  }
  else if (i2 != _az_ENCODING_PAD)
  {
    i2 = _az_base64_decode_array[i2];

    i2 <<= 6;

    i0 |= i2;

    if (i0 < 0)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    if (destination_index > destination_length - 2)
    {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    }
    *(destination_ptr + 1) = (uint8_t)(i0 >> 8);
    *destination_ptr = (uint8_t)(i0 >> 16);
    destination_ptr += 2;
  }
  else
  {
    if (i0 < 0)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    if (destination_index > destination_length - 1)
    {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    }
    *destination_ptr = (uint8_t)(i0 >> 16);
    destination_ptr += 1;
  }

  *out_written = (int32_t)(destination_ptr - az_span_ptr(destination_bytes));
  return AZ_OK;
}

AZ_NODISCARD int32_t az_base64_get_max_decoded_size(int32_t source_base64_text_size)
{
  _az_PRECONDITION(source_base64_text_size >= 0);
  return (source_base64_text_size / 4) * 3;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/internal/az_precondition_internal.h>

#include "az_json_private.h"

#include <azure/core/_az_cfg.h>

AZ_NODISCARD static uint8_t _az_json_unescape_single_byte(uint8_t ch)
{
  switch (ch)
  {
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case '\\':
    case '"':
    case '/':
    default:
    {
      // We are assuming the JSON token string has already been validated before this and we won't
      // have unexpected bytes folowing the back slash (for example \q). Therefore, just return the
      // same character back for such cases.
      return ch;
    }
  }
}

AZ_NODISCARD bool az_json_token_is_text_equal(
    az_json_token const* json_token,
    az_span expected_text)
{
  _az_PRECONDITION_NOT_NULL(json_token);

  // Cannot compare the value of non-string token kinds
  if (json_token->kind != AZ_JSON_TOKEN_STRING && json_token->kind != AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    return false;
  }

  az_span token_slice = json_token->slice;
  if (!json_token->_internal.string_has_escaped_chars)
  {
    return az_span_is_content_equal(token_slice, expected_text);
  }

  int32_t token_size = az_span_size(token_slice);
  uint8_t* token_ptr = az_span_ptr(token_slice);

  int32_t expected_size = az_span_size(expected_text);
  uint8_t* expected_ptr = az_span_ptr(expected_text);

  // No need to try to unescape the token slice, since the lengths won't match anyway.
  // Unescaping always shrinks the string, at most by a factor of 6.
  if (token_size < expected_size
      || (token_size / _az_MAX_EXPANSION_FACTOR_WHILE_ESCAPING) > expected_size)
  {
    return false;
  }

  int32_t token_idx = 0;
  for (int32_t i = 0; i < expected_size; i++)
  {
    if (token_idx >= token_size)
    {
      return false;
    }
    uint8_t token_byte = token_ptr[token_idx];

    if (token_byte == '\\')
    {
      token_idx++;
      if (token_idx >= token_size)
      {
        return false;
      }
      token_byte = token_ptr[token_idx];
      token_byte = _az_json_unescape_single_byte(token_byte);

      // TODO: Characters escaped in the form of \uXXXX where XXXX is the UTF-16 code point, is
      // not currently supported.
      // To do this, we need to encode UTF-16 codepoints (including surrogate pairs) into UTF-8.
      if (token_byte == 'u')
      {
        return false;
      }
    }

    if (token_ptr[i] != expected_ptr[i])
    {
      return false;
    }

    token_idx++;
  }

  // Only return true if the size of the unescaped token matches the expected size exactly.
  return token_idx == token_size;
}

AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token const* json_token, bool* out_value)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_TRUE && json_token->kind != AZ_JSON_TOKEN_FALSE)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  *out_value = az_span_size(json_token->slice) == _az_STRING_LITERAL_LEN("true");
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_token_get_string(
    az_json_token const* json_token,
    char* destination,
    int32_t destination_max_size,
    int32_t* out_string_length)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(destination);
  _az_PRECONDITION(destination_max_size > 0);

  if (json_token->kind != AZ_JSON_TOKEN_STRING && json_token->kind != AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  az_span token_slice = json_token->slice;
  int32_t token_size = az_span_size(token_slice);

  // There is nothing to unescape here, copy directly.
  if (!json_token->_internal.string_has_escaped_chars)
  {
    // We need enough space to add a null terminator.
    if (token_size >= destination_max_size)
    {
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
    }

    // This will add a null terminator.
    az_span_to_str(destination, destination_max_size, token_slice);

    if (out_string_length != NULL)
    {
      *out_string_length = token_size;
    }
    return AZ_OK;
  }

  // No need to try to unescape the token slice, if the destination is known to be too small.
  // Unescaping always shrinks the string, at most by a factor of 6.
  // We also need enough space to add a null terminator.
  if (token_size / _az_MAX_EXPANSION_FACTOR_WHILE_ESCAPING >= destination_max_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  uint8_t* token_ptr = az_span_ptr(token_slice);

  int32_t dest_idx = 0;
  for (int32_t i = 0; i < token_size; i++)
  {
    if (dest_idx >= destination_max_size)
    {
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
    }
    uint8_t token_byte = token_ptr[i];

    if (token_byte == '\\')
    {
      i++;
      // For all valid JSON tokens, this is guaranteed to be within the bounds.
      token_byte = _az_json_unescape_single_byte(token_ptr[i]);

      // TODO: Characters escaped in the form of \uXXXX where XXXX is the UTF-16 code point, is
      // not currently supported.
      // To do this, we need to encode UTF-16 codepoints (including surrogate pairs) into UTF-8.
      if (token_byte == 'u')
      {
        return AZ_ERROR_NOT_IMPLEMENTED;
      }
    }

    destination[dest_idx] = (char)token_byte;
    dest_idx++;
  }

  if (dest_idx >= destination_max_size)
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }
  destination[dest_idx] = 0;

  if (out_string_length != NULL)
  {
    *out_string_length = dest_idx;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_token_get_uint64(az_json_token const* json_token, uint64_t* out_value)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  return az_span_atou64(json_token->slice, out_value);
}

AZ_NODISCARD az_result
az_json_token_get_uint32(az_json_token const* json_token, uint32_t* out_value)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  return az_span_atou32(json_token->slice, out_value);
}

AZ_NODISCARD az_result az_json_token_get_int64(az_json_token const* json_token, int64_t* out_value)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  return az_span_atoi64(json_token->slice, out_value);
}

AZ_NODISCARD az_result az_json_token_get_int32(az_json_token const* json_token, int32_t* out_value)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  return az_span_atoi32(json_token->slice, out_value);
}

AZ_NODISCARD az_result az_json_token_get_double(az_json_token const* json_token, double* out_value)
{
  _az_PRECONDITION_NOT_NULL(json_token);
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_JSON_INVALID_STATE;
  }

  return az_span_atod(json_token->slice, out_value);
}

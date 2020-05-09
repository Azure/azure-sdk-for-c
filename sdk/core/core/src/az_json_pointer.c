// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include <az_json.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <_az_cfg.h>

static AZ_NODISCARD az_result _az_span_reader_read_json_pointer_char(az_span* self, uint32_t* out)
{
  _az_PRECONDITION_NOT_NULL(self);
  int32_t reader_current_length = az_span_size(*self);

  // check for EOF
  if (reader_current_length == 0)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  uint8_t const result = az_span_ptr(*self)[0];
  switch (result)
  {
    case '/':
    {
      return AZ_ERROR_JSON_POINTER_TOKEN_END;
    }
    case '~':
    {
      // move reader to next position
      *self = az_span_slice_to_end(*self, 1);
      // check for EOF
      if (az_span_size(*self) == 0)
      {
        return AZ_ERROR_EOF;
      }
      // get char
      uint8_t const e = az_span_ptr(*self)[0];
      // move to next position again
      *self = az_span_slice_to_end(*self, 1);
      switch (e)
      {
        case '0':
        {
          *out = '~';
          return AZ_OK;
        }
        case '1':
        {
          *out = '/';
          return AZ_OK;
        }
        default:
        {
          return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }
      }
    }
    default:
    {
      // move reader to next position
      *self = az_span_slice_to_end(*self, 1);

      *out = (uint8_t)result;
      return AZ_OK;
    }
  }
}

/**
 * Returns a next reference token in the JSON pointer.
 *
 * See https://tools.ietf.org/html/rfc6901
 */
AZ_NODISCARD az_result _az_span_reader_read_json_pointer_token(az_span* self, az_span* out)
{
  // read `/` if any.
  {
    // check there is something still to read
    if (az_span_size(*self) == 0)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    // ensure first char of pointer is `/`
    if (az_span_ptr(*self)[0] != '/')
    {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
  }
  // move forward
  *self = az_span_slice_to_end(*self, 1);
  if (az_span_size(*self) == 0)
  {
    *out = *self;
    return AZ_OK;
  }

  // What's happening below: Keep reading/scaning until POINTER_TOKEN_END is found or we get to the
  // end of a Json token. var begin will record the number of bytes read until token_end or
  // pointer_end. TODO: We might be able to implement _az_span_scan_until() here, since we ignore
  // the out of _az_span_reader_read_json_pointer_char()
  int32_t initial_capacity = az_span_size(*self);
  uint8_t* p_reader = az_span_ptr(*self);
  while (true)
  {
    uint32_t ignore = { 0 };
    az_result const result = _az_span_reader_read_json_pointer_char(self, &ignore);
    switch (result)
    {
      case AZ_ERROR_ITEM_NOT_FOUND:
      case AZ_ERROR_JSON_POINTER_TOKEN_END:
      {
        int32_t current_capacity = initial_capacity - az_span_size(*self);
        *out = az_span_init(p_reader, current_capacity);
        return AZ_OK;
      }
      default:
      {
        AZ_RETURN_IF_FAILED(result);
      }
    }
  }
}

/**
 * Returns a next character in the given span reader of JSON pointer reference token.
 */
AZ_NODISCARD az_result _az_span_reader_read_json_pointer_token_char(az_span* self, uint32_t* out)
{
  uint32_t c;
  az_result const result = _az_span_reader_read_json_pointer_char(self, &c);
  if (result == AZ_ERROR_JSON_POINTER_TOKEN_END)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  AZ_RETURN_IF_FAILED(result);
  *out = c;
  return AZ_OK;
}

AZ_NODISCARD static bool az_json_pointer_token_eq_json_string(
    az_span pointer_token,
    az_span json_string)
{
  while (true)
  {
    uint32_t pt_c = 0;
    az_result const pt_result = _az_span_reader_read_json_pointer_token_char(&pointer_token, &pt_c);
    uint32_t js_c = 0;
    az_result const js_result = _az_span_reader_read_json_string_char(&json_string, &js_c);
    if (js_result == AZ_ERROR_ITEM_NOT_FOUND && pt_result == AZ_ERROR_ITEM_NOT_FOUND)
    {
      return true;
    }
    if (az_failed(js_result) || az_failed(pt_result))
    {
      return false;
    }
    if (pt_c != js_c)
    {
      return false;
    }
  }
}

AZ_NODISCARD static az_result az_json_parser_get_by_pointer_token(
    az_json_parser* json_parser,
    az_span pointer_token,
    az_json_token* inout_token)
{
  switch (inout_token->kind)
  {
    case AZ_JSON_TOKEN_BEGIN_ARRAY:
    {
      uint64_t i = 0;
      AZ_RETURN_IF_FAILED(az_span_atou64(pointer_token, &i));
      while (true)
      {
        AZ_RETURN_IF_FAILED(az_json_parser_parse_array_item(json_parser, inout_token));
        if (i == 0)
        {
          return AZ_OK;
        }
        --i;
        AZ_RETURN_IF_FAILED(az_json_parser_skip_children(json_parser, *inout_token));
      }
    }
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
    {
      while (true)
      {
        az_json_token_member token_member = { 0 };
        AZ_RETURN_IF_FAILED(az_json_parser_parse_token_member(json_parser, &token_member));
        if (az_json_pointer_token_eq_json_string(pointer_token, token_member.name))
        {
          *inout_token = token_member.token;
          return AZ_OK;
        }
        AZ_RETURN_IF_FAILED(az_json_parser_skip_children(json_parser, token_member.token));
      }
    }
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }
}

AZ_NODISCARD az_result
az_json_parse_by_pointer(az_span json_buffer, az_span json_pointer, az_json_token* out_token)
{
  _az_PRECONDITION_NOT_NULL(out_token);

  az_json_parser json_parser = { 0 };
  AZ_RETURN_IF_FAILED(az_json_parser_init(&json_parser, json_buffer));

  AZ_RETURN_IF_FAILED(az_json_parser_parse_token(&json_parser, out_token));
  while (true)
  {
    az_span pointer_token = AZ_SPAN_NULL;
    az_result const result = _az_span_reader_read_json_pointer_token(&json_pointer, &pointer_token);
    if (result == AZ_ERROR_ITEM_NOT_FOUND)
    {
      return AZ_OK; // no more pointer tokens so we found the JSON value.
    }
    AZ_RETURN_IF_FAILED(result);
    AZ_RETURN_IF_FAILED(
        az_json_parser_get_by_pointer_token(&json_parser, pointer_token, out_token));
  }
}

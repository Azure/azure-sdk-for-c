// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_json_string_private.h"
#include <az_json.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <ctype.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_hex_to_digit(uint8_t c, uint8_t* out)
{
  if (isdigit(c))
  {
    *out = (uint8_t)(c - '0');
  }
  else if ('a' <= c && c <= 'f')
  {
    *out = (uint8_t)(c - ('a' - 10));
  }
  else if ('A' <= c && c <= 'F')
  {
    *out = (uint8_t)(c - ('A' - 10));
  }
  else
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_json_esc_decode(uint8_t c, uint8_t* out)
{
  switch (c)
  {
    case '\\':
    case '"':
    case '/':
    {
      *out = c;
      break;
    }
    case 'b':
    {
      *out = '\b';
      break;
    }
    case 'f':
    {
      *out = '\f';
      break;
    }
    case 'n':
    {
      *out = '\n';
      break;
    }
    case 'r':
    {
      *out = '\r';
      break;
    }
    case 't':
    {
      *out = '\t';
      break;
    }
    default:
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  return AZ_OK;
}

/**
 * Encodes the given character into a JSON escape sequence. The function returns an empty span if
 * the given character doesn't require to be escaped.
 */
AZ_NODISCARD az_span _az_json_esc_encode(uint8_t c)
{
  switch (c)
  {
    case '\\':
    {
      return AZ_SPAN_FROM_STR("\\\\");
    }
    case '"':
    {
      return AZ_SPAN_FROM_STR("\\\"");
    }
    case '\b':
    {
      return AZ_SPAN_FROM_STR("\\b");
    }
    case '\f':
    {
      return AZ_SPAN_FROM_STR("\\f");
    }
    case '\n':
    {
      return AZ_SPAN_FROM_STR("\\n");
    }
    case '\r':
    {
      return AZ_SPAN_FROM_STR("\\r");
    }
    case '\t':
    {
      return AZ_SPAN_FROM_STR("\\t");
    }
    default:
    {
      return AZ_SPAN_NULL;
    }
  }
}

/**
 * TODO: this function and JSON pointer read functions should return proper UNICODE
 *       code-point to be compatible.
 */
AZ_NODISCARD az_result _az_span_reader_read_json_string_char(az_span* json_string, uint32_t* out)
{
  int32_t reader_length = az_span_size(*json_string);
  if (reader_length == 0)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  uint8_t const result = az_span_ptr(*json_string)[0];
  switch (result)
  {
    case '"':
    {
      return AZ_ERROR_JSON_STRING_END;
    }
    case '\\':
    {
      // moving reader fw
      *json_string = az_span_slice_to_end(*json_string, 1);
      if (az_span_size(*json_string) == 0)
      {
        return AZ_ERROR_EOF;
      }
      uint8_t const c = az_span_ptr(*json_string)[0];
      *json_string = az_span_slice_to_end(*json_string, 1);

      if (c == 'u')
      {
        uint32_t r = 0;
        for (size_t i = 0; i < 4; ++i)
        {
          uint8_t digit = 0;
          if (az_span_size(*json_string) == 0)
          {
            return AZ_ERROR_EOF;
          }
          AZ_RETURN_IF_FAILED(az_hex_to_digit(az_span_ptr(*json_string)[0], &digit));
          r = (r << 4) + digit;
          *json_string = az_span_slice_to_end(*json_string, 1);
        }
        *out = r;
      }
      else
      {
        uint8_t r = 0;
        AZ_RETURN_IF_FAILED(az_json_esc_decode(c, &r));
        *out = r;
      }
      return AZ_OK;
    }
    default:
    {
      if (result < 0x20)
      {
        return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
      }
      *json_string = az_span_slice_to_end(*json_string, 1);
      *out = (uint16_t)result;
      return AZ_OK;
    }
  }
}

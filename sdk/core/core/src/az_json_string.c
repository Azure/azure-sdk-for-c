// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_contract_internal.h>
#include <az_json.h>

#include "az_hex_private.h"

#include <ctype.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result az_hex_to_digit(uint8_t c, uint8_t* out)
{
  if (isdigit(c))
  {
    *out = c - '0';
  }
  else if ('a' <= c && c <= 'f')
  {
    *out = c - _az_HEX_LOWER_OFFSET;
  }
  else if ('A' <= c && c <= 'F')
  {
    *out = c - _az_HEX_UPPER_OFFSET;
  }
  else
  {
    return AZ_ERROR_PARSING;
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
      return AZ_ERROR_PARSING;
  }
  return AZ_OK;
}

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

AZ_NODISCARD az_result _az_span_reader_read_json_string_char(az_span* self, uint32_t* out)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  int32_t reader_length = az_span_length(*self);
  if (reader_length == 0)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  uint8_t const result = az_span_ptr(*self)[0];
  switch (result)
  {
    case '"':
    {
      return AZ_ERROR_JSON_STRING_END;
    }
    case '\\':
    {
      // moving reader fw
      AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));
      if (az_span_length(*self) == 0)
      {
        return AZ_ERROR_EOF;
      }
      uint8_t const c = az_span_ptr(*self)[0];
      AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));

      if (c == 'u')
      {
        uint32_t r = 0;
        for (size_t i = 0; i < 4; ++i)
        {
          uint8_t digit = 0;
          if (az_span_length(*self) == 0)
          {
            return AZ_ERROR_EOF;
          }
          AZ_RETURN_IF_FAILED(az_hex_to_digit(az_span_ptr(*self)[0], &digit));
          r = (r << 4) + digit;
          AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));
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
        return AZ_ERROR_PARSING;
      }
      AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));
      *out = (uint16_t)result;
      return AZ_OK;
    }
  }
}

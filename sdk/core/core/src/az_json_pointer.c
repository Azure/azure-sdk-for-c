// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_contract_internal.h>
#include <az_json.h>

#include <_az_cfg.h>

AZ_NODISCARD static az_result az_span_reader_read_json_pointer_char(az_span* self, uint32_t* out)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);
  int32_t reader_current_length = az_span_length(*self);

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
      AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));
      // check for EOF
      if (az_span_length(*self) == 0)
      {
        return AZ_ERROR_EOF;
      }
      // get char
      uint8_t const e = az_span_ptr(*self)[0];
      // move to next position again
      AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));
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
      AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));

      *out = (uint8_t)result;
      return AZ_OK;
    }
  }
}

AZ_NODISCARD az_result _az_span_reader_read_json_pointer_token(az_span* self, az_span* out)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // read `/` if any.
  {
    // check there is something still to read
    if (az_span_length(*self) == 0)
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
  AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));
  if (az_span_length(*self) == 0)
  {
    *out = *self;
    return AZ_OK;
  }

  // What's happening below: Keep reading/scaning until POINTER_TOKEN_END is found or we get to the
  // end of a Json token. var begin will record the number of bytes read until token_end or
  // pointer_end. TODO: We might be able to implement _az_span_scan_until() here, since we ignore
  // the out of az_span_reader_read_json_pointer_char()
  int32_t initial_capacity = az_span_capacity(*self);
  uint8_t* p_reader = az_span_ptr(*self);
  while (true)
  {
    uint32_t ignore = { 0 };
    az_result const result = az_span_reader_read_json_pointer_char(self, &ignore);
    switch (result)
    {
      case AZ_ERROR_ITEM_NOT_FOUND:
      case AZ_ERROR_JSON_POINTER_TOKEN_END:
      {
        int32_t current_capacity = initial_capacity - az_span_capacity(*self);
        *out = az_span_init(p_reader, current_capacity, current_capacity);
        return AZ_OK;
      }
      default:
      {
        AZ_RETURN_IF_FAILED(result);
      }
    }
  }
}

AZ_NODISCARD az_result _az_span_reader_read_json_pointer_token_char(az_span* self, uint32_t* out)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  uint32_t c;
  az_result const result = az_span_reader_read_json_pointer_char(self, &c);
  if (result == AZ_ERROR_JSON_POINTER_TOKEN_END)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  AZ_RETURN_IF_FAILED(result);
  *out = c;
  return AZ_OK;
}

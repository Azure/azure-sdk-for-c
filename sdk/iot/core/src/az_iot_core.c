// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_core.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_iot_get_status_from_uint32(uint32_t status_int, az_iot_status* status)
{
  switch (status_int)
  {
    case AZ_IOT_STATUS_OK:
      *status = AZ_IOT_STATUS_OK;
      break;
    case AZ_IOT_STATUS_ACCEPTED:
      *status = AZ_IOT_STATUS_ACCEPTED;
      break;
    case AZ_IOT_STATUS_NO_CONTENT:
      *status = AZ_IOT_STATUS_NO_CONTENT;
      break;
    case AZ_IOT_STATUS_BAD_REQUEST:
      *status = AZ_IOT_STATUS_BAD_REQUEST;
      break;
    case AZ_IOT_STATUS_UNAUTHORIZED:
      *status = AZ_IOT_STATUS_UNAUTHORIZED;
      break;
    case AZ_IOT_STATUS_FORBIDDEN:
      *status = AZ_IOT_STATUS_FORBIDDEN;
      break;
    case AZ_IOT_STATUS_NOT_FOUND:
      *status = AZ_IOT_STATUS_NOT_FOUND;
      break;
    case AZ_IOT_STATUS_NOT_ALLOWED:
      *status = AZ_IOT_STATUS_NOT_ALLOWED;
      break;
    case AZ_IOT_STATUS_NOT_CONFLICT:
      *status = AZ_IOT_STATUS_NOT_CONFLICT;
      break;
    case AZ_IOT_STATUS_PRECONDITION_FAILED:
      *status = AZ_IOT_STATUS_PRECONDITION_FAILED;
      break;
    case AZ_IOT_STATUS_REQUEST_TOO_LARGE:
      *status = AZ_IOT_STATUS_REQUEST_TOO_LARGE;
      break;
    case AZ_IOT_STATUS_UNSUPPORTED_TYPE:
      *status = AZ_IOT_STATUS_UNSUPPORTED_TYPE;
      break;
    case AZ_IOT_STATUS_THROTTLED:
      *status = AZ_IOT_STATUS_THROTTLED;
      break;
    case AZ_IOT_STATUS_CLIENT_CLOSED:
      *status = AZ_IOT_STATUS_CLIENT_CLOSED;
      break;
    case AZ_IOT_STATUS_SERVER_ERROR:
      *status = AZ_IOT_STATUS_SERVER_ERROR;
      break;
    case AZ_IOT_STATUS_BAD_GATEWAY:
      *status = AZ_IOT_STATUS_BAD_GATEWAY;
      break;
    case AZ_IOT_STATUS_SERVICE_UNAVAILABLE:
      *status = AZ_IOT_STATUS_SERVICE_UNAVAILABLE;
      break;
    case AZ_IOT_STATUS_TIMEOUT:
      *status = AZ_IOT_STATUS_TIMEOUT;
      break;
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

AZ_NODISCARD az_span az_span_token(az_span source, az_span delimiter, az_span* out_remainder)
{
  AZ_PRECONDITION_VALID_SPAN(delimiter, 1, false);
  AZ_PRECONDITION_NOT_NULL(out_remainder);

  if (az_span_size(source) == 0)
  {
    return AZ_SPAN_NULL;
  }
  else
  {
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
}

AZ_NODISCARD int32_t u32toa_size(uint32_t number)
{
  if (number == 0)
  {
    return 1;
  }
  else
  {
    uint32_t div = 1000000000;
    int32_t digit_count = 10;
    while (number / div == 0)
    {
      div /= 10;
      digit_count--;
    }

    return digit_count; 
  }
}

/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <azure/core/az_json.h>
#include <sys/time.h>
#include "unlock_json_parser.h"


//"{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}"
az_result deserialize_unlock_request(az_span request_data, unlock_request* unlock_json_out)
{
  az_json_reader jr = {0};
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, request_data, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }
  while (az_result_succeeded(az_json_reader_next_token(&jr)) && jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("RequestTimestamp")))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_NUMBER)
      {
        return AZ_ERROR_ITEM_NOT_FOUND;
      }
      AZ_RETURN_IF_FAILED(az_json_token_get_int64(&jr.token, &unlock_json_out->request_timestamp));
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("RequestedFrom")))
    {
      AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_ITEM_NOT_FOUND;
      }
      unlock_json_out->requested_from = jr.token.slice;
      AZ_RETURN_IF_FAILED(az_json_token_get_string(
          &jr.token, az_span_ptr(unlock_json_out->requested_from), jr.token.size + 1, &unlock_json_out->requested_from._internal.size));
    }
    else
    {
      // ignore other tokens
      AZ_RETURN_IF_FAILED(az_json_reader_skip_children(&jr));
    }
  }
  return AZ_OK;
}

// "{\"Succeed\":true,\"ReceivedFrom\":\"mobile-app\",\"processedMs\":5}"
az_result serialize_response_payload(unlock_request req, az_span out_payload)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  long now_ms = t.tv_sec * 1000 + t.tv_usec / 1000;
  long processedMs = now_ms - req.request_timestamp;

  az_json_writer jw;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, out_payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, az_span_create_from_str("Succeed")));
  AZ_RETURN_IF_FAILED(az_json_writer_append_bool(&jw, true));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, az_span_create_from_str("ReceivedFrom")));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, req.requested_from));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, az_span_create_from_str("ProcessedMs")));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, processedMs, 0));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  return AZ_OK;
}
/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include "mosquitto_rpc_server_sample_json_parser.h"
#include <az_log_listener.h>
#include <azure/core/az_json.h>
#include <sys/time.h>

//"{\"RequestTimestamp\":1691530585198,\"RequestedFrom\":\"mobile-app\"}"
az_result deserialize_unlock_request(az_span request_data, unlock_request* unlock_json_out)
{
  az_json_reader jr = { 0 };
  LOG_AND_EXIT_IF_FAILED(az_json_reader_init(&jr, request_data, NULL));
  LOG_AND_EXIT_IF_FAILED(az_json_reader_next_token(&jr));

  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }
  while (az_result_succeeded(az_json_reader_next_token(&jr))
         && jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("RequestTimestamp")))
    {
      LOG_AND_EXIT_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_NUMBER)
      {
        return AZ_ERROR_ITEM_NOT_FOUND;
      }
      LOG_AND_EXIT_IF_FAILED(
          az_json_token_get_int64(&jr.token, &unlock_json_out->request_timestamp));
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("RequestedFrom")))
    {
      LOG_AND_EXIT_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_ITEM_NOT_FOUND;
      }
      unlock_json_out->requested_from = jr.token.slice;
      LOG_AND_EXIT_IF_FAILED(az_json_token_get_string(
          &jr.token,
          (char*)az_span_ptr(unlock_json_out->requested_from),
          jr.token.size + 1,
          &unlock_json_out->requested_from._internal.size));
    }
    else
    {
      // nothing else should be in the request
      return AZ_ERROR_UNEXPECTED_CHAR;
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
  LOG_AND_EXIT_IF_FAILED(az_json_writer_init(&jw, out_payload, NULL));
  LOG_AND_EXIT_IF_FAILED(az_json_writer_append_begin_object(&jw));
  LOG_AND_EXIT_IF_FAILED(
      az_json_writer_append_property_name(&jw, az_span_create_from_str("Succeed")));
  LOG_AND_EXIT_IF_FAILED(az_json_writer_append_bool(&jw, true));
  LOG_AND_EXIT_IF_FAILED(
      az_json_writer_append_property_name(&jw, az_span_create_from_str("ReceivedFrom")));
  LOG_AND_EXIT_IF_FAILED(az_json_writer_append_string(&jw, req.requested_from));
  LOG_AND_EXIT_IF_FAILED(
      az_json_writer_append_property_name(&jw, az_span_create_from_str("ProcessedMs")));
  LOG_AND_EXIT_IF_FAILED(az_json_writer_append_double(&jw, (double)processedMs, 0));
  LOG_AND_EXIT_IF_FAILED(az_json_writer_append_end_object(&jw));
  out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
  return AZ_OK;
}

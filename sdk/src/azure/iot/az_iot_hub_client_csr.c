// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_hub_client.h>

#include <azure/core/_az_cfg.h>

static const uint8_t null_terminator = '\0';
static const az_span certificat_signing_request_topic_prefix
    = AZ_SPAN_LITERAL_FROM_STR("$iothub/credentials/");
static const az_span certificat_signing_request_sub_topic = AZ_SPAN_LITERAL_FROM_STR("res/");
static const az_span certificat_signing_request_pub_topic
    = AZ_SPAN_LITERAL_FROM_STR("POST/issueCertificate/");
static const az_span certificat_signing_request_id_prop = AZ_SPAN_LITERAL_FROM_STR("?$rid=");

AZ_NODISCARD az_result az_iot_hub_client_certificate_signing_request_get_publish_topic(
    az_iot_hub_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(client->_internal.iot_hub_hostname, 1, false);
  _az_PRECONDITION_VALID_SPAN(request_id, 1, false);
  _az_PRECONDITION_NOT_NULL(mqtt_topic);
  _az_PRECONDITION(mqtt_topic_size > 0);
  (void)client;

  az_span mqtt_topic_span = az_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  int32_t required_length = az_span_size(certificat_signing_request_topic_prefix)
      + az_span_size(certificat_signing_request_pub_topic) + az_span_size(certificat_signing_request_id_prop)
      + az_span_size(request_id);

  _az_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  az_span remainder = az_span_copy(mqtt_topic_span, certificat_signing_request_topic_prefix);
  remainder = az_span_copy(remainder, certificat_signing_request_pub_topic);
  remainder = az_span_copy(remainder, certificat_signing_request_id_prop);
  remainder = az_span_copy(remainder, request_id);
  az_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_certificate_signing_request_get_request_payload(
    az_iot_hub_client const* client,
    az_iot_hub_client_certificate_signing_request const* certificate_signing_request,
    uint8_t* mqtt_payload,
    size_t mqtt_payload_size,
    size_t* out_mqtt_payload_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(certificate_signing_request);
  _az_PRECONDITION_VALID_SPAN(certificate_signing_request->csr, 1, false);
  _az_PRECONDITION_NOT_NULL(mqtt_payload);
  _az_PRECONDITION(mqtt_payload_size > 0);
  _az_PRECONDITION_NOT_NULL(out_mqtt_payload_length);

  az_json_writer json_writer;
  az_span payload_buffer = az_span_create(mqtt_payload, (int32_t)mqtt_payload_size);

  _az_RETURN_IF_FAILED(az_json_writer_init(&json_writer, payload_buffer, NULL));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_writer));

  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&json_writer, AZ_SPAN_FROM_STR("id")));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(&json_writer, client->_internal.device_id));

  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&json_writer, AZ_SPAN_FROM_STR("csr")));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(&json_writer, certificate_signing_request->csr));

  if (az_span_size(certificate_signing_request->replace) > 0)
  {
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(&json_writer, AZ_SPAN_FROM_STR("replace")));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(&json_writer, certificate_signing_request->replace));
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_writer));
  *out_mqtt_payload_length
      = (size_t)az_span_size(az_json_writer_get_bytes_used_in_destination(&json_writer));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_certificate_signing_request_parse_received_topic(
    az_iot_hub_client const* client,
    az_span received_topic,
    az_iot_hub_client_certificate_signing_response_info* out_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_response);
  (void)client;

  int32_t prefix_index = az_span_find(received_topic, certificat_signing_request_topic_prefix);
  if (prefix_index < 0)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  _az_LOG_WRITE(AZ_LOG_MQTT_RECEIVED_TOPIC, received_topic);

  az_span after_prefix = az_span_slice(
      received_topic,
      prefix_index + az_span_size(certificat_signing_request_topic_prefix),
      az_span_size(received_topic));

  int32_t res_index = az_span_find(after_prefix, certificat_signing_request_sub_topic);
  if (res_index < 0)
  {
    return AZ_ERROR_IOT_TOPIC_NO_MATCH;
  }

  // Extract status code: everything between "res/" and the next "/"
  az_span after_res = az_span_slice(
      after_prefix,
      res_index + az_span_size(certificat_signing_request_sub_topic),
      az_span_size(after_prefix));

  int32_t index = 0;
  az_span remainder;
  az_span status_str = _az_span_token(after_res, AZ_SPAN_FROM_STR("/"), &remainder, &index);

  uint32_t status_int = 0;
  _az_RETURN_IF_FAILED(az_span_atou32(status_str, &status_int));
  out_response->status_code = (az_iot_status)status_int;

  if (index == -1)
  {
    return AZ_ERROR_UNEXPECTED_END;
  }

  // Extract request_id from properties after "?"
  az_iot_message_properties props;
  az_span prop_span = az_span_slice(remainder, 1, az_span_size(remainder));
  _az_RETURN_IF_FAILED(
      az_iot_message_properties_init(&props, prop_span, az_span_size(prop_span)));
  _az_RETURN_IF_FAILED(az_iot_message_properties_find(
      &props, AZ_SPAN_FROM_STR("$rid"), &out_response->request_id));

  // Classify response type
  if (out_response->status_code == (az_iot_status)AZ_IOT_STATUS_ACCEPTED)
  {
    out_response->response_type = AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ACCEPTED;
  }
  else if (out_response->status_code == AZ_IOT_STATUS_OK)
  {
    out_response->response_type = AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_COMPLETED;
  }
  else
  {
    out_response->response_type = AZ_IOT_HUB_CLIENT_CERTIFICATE_SIGNING_RESPONSE_TYPE_ERROR;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_certificate_signing_request_parse_accepted_response(
    az_iot_hub_client const* client,
    az_span received_payload,
    az_iot_hub_client_certificate_signing_accepted_response* out_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_payload, 1, false);
  _az_PRECONDITION_NOT_NULL(out_response);
  (void)client;

  out_response->correlation_id = AZ_SPAN_EMPTY;
  out_response->operation_expires = AZ_SPAN_EMPTY;

  az_json_reader jr;
  _az_RETURN_IF_FAILED(az_json_reader_init(&jr, received_payload, NULL));
  _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (az_result_succeeded(az_json_reader_next_token(&jr))
         && jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("correlationId")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->correlation_id = jr.token.slice;
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("operationExpires")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->operation_expires = jr.token.slice;
    }
    else
    {
      _az_RETURN_IF_FAILED(az_json_reader_skip_children(&jr));
    }
  }

  return AZ_OK;
}

static az_result _az_iot_hub_client_certificate_signing_response_parse_info_object(
    az_json_reader* jr,
    az_iot_hub_client_certificate_signing_error_response* out_response)
{
  if (jr->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (az_result_succeeded(az_json_reader_next_token(jr))
         && jr->token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&jr->token, AZ_SPAN_FROM_STR("correlationId")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      if (jr->token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->correlation_id = jr->token.slice;
    }
    else if (az_json_token_is_text_equal(&jr->token, AZ_SPAN_FROM_STR("credentialError")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      if (jr->token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->info_error = jr->token.slice;
    }
    else if (az_json_token_is_text_equal(
                 &jr->token, AZ_SPAN_FROM_STR("credentialMessage")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      if (jr->token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->info_message = jr->token.slice;
    }
    else if (az_json_token_is_text_equal(
                 &jr->token, AZ_SPAN_FROM_STR("requestId")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      if (jr->token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->info_request_id = jr->token.slice;
    }
    else if (az_json_token_is_text_equal(
                 &jr->token, AZ_SPAN_FROM_STR("operationExpires")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      if (jr->token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->info_operation_expires = jr->token.slice;
    }
    else
    {
      _az_RETURN_IF_FAILED(az_json_reader_skip_children(jr));
    }
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_certificate_signing_request_parse_error_response(
    az_iot_hub_client const* client,
    az_span received_payload,
    az_iot_hub_client_certificate_signing_error_response* out_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_payload, 1, false);
  _az_PRECONDITION_NOT_NULL(out_response);
  (void)client;

  out_response->error_code = 0;
  out_response->message = AZ_SPAN_EMPTY;
  out_response->tracking_id = AZ_SPAN_EMPTY;
  out_response->timestamp_utc = AZ_SPAN_EMPTY;
  out_response->correlation_id = AZ_SPAN_EMPTY;
  out_response->retry_after_seconds = 0;
  out_response->info_error = AZ_SPAN_EMPTY;
  out_response->info_message = AZ_SPAN_EMPTY;
  out_response->info_request_id = AZ_SPAN_EMPTY;
  out_response->info_operation_expires = AZ_SPAN_EMPTY;

  az_json_reader jr;
  _az_RETURN_IF_FAILED(az_json_reader_init(&jr, received_payload, NULL));
  _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (az_result_succeeded(az_json_reader_next_token(&jr))
         && jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("errorCode")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      _az_RETURN_IF_FAILED(az_json_token_get_uint32(&jr.token, &out_response->error_code));
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("message")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->message = jr.token.slice;
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("trackingId")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->tracking_id = jr.token.slice;
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("timestampUtc")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      if (jr.token.kind != AZ_JSON_TOKEN_STRING)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      out_response->timestamp_utc = jr.token.slice;
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("retryAfter")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      _az_RETURN_IF_FAILED(
          az_json_token_get_uint32(&jr.token, &out_response->retry_after_seconds));
    }
    else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR("info")))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      _az_RETURN_IF_FAILED(
          _az_iot_hub_client_certificate_signing_response_parse_info_object(&jr, out_response));
    }
    else
    {
      _az_RETURN_IF_FAILED(az_json_reader_skip_children(&jr));
    }
  }

  return AZ_OK;
}

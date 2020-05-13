// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_provisioning_client.h>
#include <az_precondition.h>
#include <az_span.h>
#include <az_span_internal.h>
#include <az_iot_common_internal.h>

#include <az_precondition_internal.h>
#include <az_log_internal.h>

#include <stdint.h>

#include <_az_cfg.h>

#define LF '\n'
#define AMPERSAND '&'
#define EQUAL_SIGN '='
#define STRING_NULL_TERMINATOR '\0'
#define SCOPE_REGISTRATIONS_STRING "%2fregistrations%2f"
#define SAS_TOKEN_SR "SharedAccessSignature sr"
#define SAS_TOKEN_SE "se"
#define SAS_TOKEN_SIG "sig"
#define SAS_TOKEN_SKN "skn"

static const az_span resources_string = AZ_SPAN_LITERAL_FROM_STR(SCOPE_REGISTRATIONS_STRING);
static const az_span sr_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SR);
static const az_span sig_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SIG);
static const az_span skn_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SKN);
static const az_span se_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SE);

AZ_NODISCARD az_result az_iot_provisioning_client_sas_get_signature(
    az_iot_provisioning_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION(token_expiration_epoch_time > 0);
  _az_PRECONDITION_VALID_SPAN(signature, 0, false);
  _az_PRECONDITION_NOT_NULL(out_signature);

  // Produces the following signature:
  // url-encoded(<resource-string>)\n<expiration-time>
  // Where
  // resource-string: <scope-id>/registrations/<registration-id>

  int32_t required_size = 
      az_span_size(client->_internal.id_scope)
      + az_span_size(resources_string)
      + az_span_size(client->_internal.registration_id)
      + 1 // LF
      + _az_iot_u32toa_size(token_expiration_epoch_time);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(signature, required_size);

  az_span remainder = signature;
  remainder = az_span_copy(remainder, client->_internal.id_scope);
  remainder = az_span_copy(remainder, resources_string);
  remainder = az_span_copy(remainder, client->_internal.registration_id);
  remainder = az_span_copy_u8(remainder, LF);
  
  AZ_RETURN_IF_FAILED(az_span_u32toa(remainder, token_expiration_epoch_time, &remainder));

  *out_signature = az_span_slice(signature, 0, required_size);
  _az_LOG_WRITE(AZ_LOG_IOT_SAS_TOKEN, *out_signature);

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_provisioning_client_sas_get_password(
    az_iot_provisioning_client const* client,
    az_span base64_hmac_sha256_signature,
    uint32_t token_expiration_epoch_time,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(base64_hmac_sha256_signature, 1, false);
  _az_PRECONDITION(token_expiration_epoch_time > 0);
  _az_PRECONDITION_NOT_NULL(mqtt_password);
  _az_PRECONDITION(mqtt_password_size > 0);

  // Concatenates:
  // "SharedAccessSignature sr=<url-encoded(resource-string)>&sig=<signature>&se=<expiration-time>"
  // plus, if key_name is not NULL, "&skn=<key-name>"
  //
  // Where:
  // resource-string: <scope-id>/registrations/<registration-id>

  int32_t required_size = 
      az_span_size(sr_string) 
      + 1 // EQUAL_SIGN
      + az_span_size(client->_internal.id_scope) 
      + az_span_size(resources_string)
      + az_span_size(client->_internal.registration_id)
      + 1 // AMPERSAND
      + az_span_size(sig_string) 
      + 1 // EQUAL_SIGN
      + az_span_size(base64_hmac_sha256_signature)
      + 1 // AMPERSAND
      + az_span_size(se_string)
      + 1 // EQUAL_SIGN
      + _az_iot_u32toa_size(token_expiration_epoch_time) 
      + 1; // STRING_NULL_TERMINATOR

  if (az_span_size(key_name) > 0)
  {
      required_size += 
      1 // AMPERSAND
      + az_span_size(skn_string) 
      + 1 // EQUAL_SIGN
      + az_span_size(key_name);
  }

  az_span mqtt_password_span = az_span_init((uint8_t*)mqtt_password, (int32_t)mqtt_password_size);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_password_span, required_size);

  // SharedAccessSignature, resource string
  mqtt_password_span = az_span_copy(mqtt_password_span, sr_string);
  mqtt_password_span = az_span_copy_u8(mqtt_password_span, EQUAL_SIGN);
  mqtt_password_span = az_span_copy(mqtt_password_span, client->_internal.id_scope);
  mqtt_password_span = az_span_copy(mqtt_password_span, resources_string);
  mqtt_password_span = az_span_copy(mqtt_password_span, client->_internal.registration_id);

  // Signature
  mqtt_password_span = az_span_copy_u8(mqtt_password_span, AMPERSAND);
  mqtt_password_span = az_span_copy(mqtt_password_span, sig_string);
  mqtt_password_span = az_span_copy_u8(mqtt_password_span, EQUAL_SIGN);
  mqtt_password_span = az_span_copy(mqtt_password_span, base64_hmac_sha256_signature);

  // Expiration
  mqtt_password_span = az_span_copy_u8(mqtt_password_span, AMPERSAND);
  mqtt_password_span = az_span_copy(mqtt_password_span, se_string);
  mqtt_password_span = az_span_copy_u8(mqtt_password_span, EQUAL_SIGN);
  AZ_RETURN_IF_FAILED(az_span_u32toa(mqtt_password_span, token_expiration_epoch_time, &mqtt_password_span));

  if (az_span_size(key_name) > 0)
  {
    // Key Name
    mqtt_password_span = az_span_copy_u8(mqtt_password_span, AMPERSAND);
    mqtt_password_span = az_span_copy(mqtt_password_span, skn_string);
    mqtt_password_span = az_span_copy_u8(mqtt_password_span, EQUAL_SIGN);
    mqtt_password_span = az_span_copy(mqtt_password_span, key_name);
  }

  mqtt_password_span = az_span_copy_u8(mqtt_password_span, STRING_NULL_TERMINATOR);

  if (out_mqtt_password_length != NULL)
  {
    *out_mqtt_password_length = ((size_t)required_size - 1);
  }

  return AZ_OK;
}

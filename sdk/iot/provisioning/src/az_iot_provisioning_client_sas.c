// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_provisioning_client.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <stdint.h>

#include <_az_cfg.h>

#define LF '\n'
#define AMPERSAND '&'
#define EQUAL_SIGN '='
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
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION(token_expiration_epoch_time > 0);
  AZ_PRECONDITION_VALID_SPAN(signature, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_signature);

  // Produces the following signature:
  // url-encoded(<resource-string>)\n<expiration-time>
  // Where
  // resource-string: <scope-id>/registrations/<registration-id>

  int32_t required_size = az_span_size(client->_internal.id_scope)
      + az_span_size(client->_internal.registration_id) + az_span_size(resources_string)
      + u32toa_size(token_expiration_epoch_time)
      + 1; // 1 is for one line feed character.

  az_span remainder = signature;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_size);

  remainder = az_span_copy(remainder, client->_internal.id_scope);
  remainder = az_span_copy(remainder, resources_string);
  remainder = az_span_copy(remainder, client->_internal.registration_id);
  remainder = az_span_copy_u8(remainder, LF);
  
  AZ_RETURN_IF_FAILED(az_span_u32toa(remainder, token_expiration_epoch_time, &remainder));

  *out_signature = az_span_slice(signature, 0, required_size);

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
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(base64_hmac_sha256_signature, 1, false);
  AZ_PRECONDITION(token_expiration_epoch_time > 0);
  AZ_PRECONDITION_NOT_NULL(mqtt_password);
  AZ_PRECONDITION(mqtt_password_size > 0);

  // Concatenates:
  // "SharedAccessSignature sr=<url-encoded(resource-string)>&sig=<signature>&se=<expiration-time>"
  // plus, if key_name is not NULL, "&skn=<key-name>"
  //
  // Where:
  // resource-string: <scope-id>/registrations/<registration-id>

  int32_t required_size = az_span_size(sr_string) + az_span_size(resources_string)
      + az_span_size(sig_string) + az_span_size(se_string)
      + az_span_size(client->_internal.id_scope) + az_span_size(client->_internal.registration_id)
      + az_span_size(base64_hmac_sha256_signature)
      + u32toa_size(token_expiration_epoch_time) 
      + 6; // Number of ampersands, equal signs and null terminator.

  if (az_span_size(key_name) > 0)
  {
    required_size += az_span_size(skn_string) + az_span_size(key_name)
        + 2; // 2 is for one ampersand and one equal sign.
  }

  az_span remainder = az_span_init((uint8_t*)mqtt_password, (int32_t)mqtt_password_size);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_size);

  // SharedAccessSignature, resource string
  remainder = az_span_copy(remainder, sr_string);
  remainder = az_span_copy_u8(remainder, EQUAL_SIGN);
  remainder = az_span_copy(remainder, client->_internal.id_scope);
  remainder = az_span_copy(remainder, resources_string);
  remainder = az_span_copy(remainder, client->_internal.registration_id);

  // Signature
  remainder = az_span_copy_u8(remainder, AMPERSAND);
  remainder = az_span_copy(remainder, sig_string);
  remainder = az_span_copy_u8(remainder, EQUAL_SIGN);
  remainder = az_span_copy(remainder, base64_hmac_sha256_signature);

  // Expiration
  remainder = az_span_copy_u8(remainder, AMPERSAND);
  remainder = az_span_copy(remainder, se_string);
  remainder = az_span_copy_u8(remainder, EQUAL_SIGN);
  AZ_RETURN_IF_FAILED(az_span_u32toa(remainder, token_expiration_epoch_time, &remainder));

  if (az_span_size(key_name) > 0)
  {
    // Key Name
    remainder = az_span_copy_u8(remainder, AMPERSAND);
    remainder = az_span_copy(remainder, skn_string);
    remainder = az_span_copy_u8(remainder, EQUAL_SIGN);
    remainder = az_span_copy(remainder, key_name);
  }

  remainder = az_span_copy_u8(remainder, '\0');

  if (out_mqtt_password_length != NULL)
  {
    *out_mqtt_password_length = ((size_t)required_size - 1);
  }

  return AZ_OK;
}

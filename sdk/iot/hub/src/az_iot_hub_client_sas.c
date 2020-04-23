// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_hub_client.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <stdint.h>

#include <_az_cfg.h>

#define LF '\n'
#define AMPERSAND '&'
#define EQUAL_SIGN '='
#define SCOPE_DEVICES_STRING "/devices/"
#define SCOPE_MODULES_STRING "/modules/"
#define SAS_TOKEN_SR "SharedAccessSignature sr"
#define SAS_TOKEN_SE "se"
#define SAS_TOKEN_SIG "sig"
#define SAS_TOKEN_SKN "skn"

static const az_span devices_string = AZ_SPAN_LITERAL_FROM_STR(SCOPE_DEVICES_STRING);
static const az_span modules_string = AZ_SPAN_LITERAL_FROM_STR(SCOPE_MODULES_STRING);
static const az_span skn_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SKN);
static const az_span sr_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SR);
static const az_span sig_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SIG);
static const az_span se_string = AZ_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SE);

AZ_NODISCARD az_result az_iot_hub_client_sas_get_signature(
    az_iot_hub_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION(token_expiration_epoch_time > 0);
  AZ_PRECONDITION_VALID_SPAN(signature, 1, false);
  AZ_PRECONDITION_NOT_NULL(out_signature);

  int32_t required_size = az_span_size(client->_internal.iot_hub_hostname)
      + az_span_size(client->_internal.device_id) + az_span_size(devices_string)
      + 1; // 1 is for one line feed character.

  if (az_span_size(client->_internal.options.module_id) > 0)
  {
    required_size
        += az_span_size(client->_internal.options.module_id) + az_span_size(modules_string);
  }

  az_span remainder = signature;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_size);

  remainder = az_span_copy(remainder, client->_internal.iot_hub_hostname);
  remainder = az_span_copy(remainder, devices_string);
  remainder = az_span_copy(remainder, client->_internal.device_id);

  if (az_span_size(client->_internal.options.module_id) > 0)
  {
    remainder = az_span_copy(remainder, modules_string);
    remainder = az_span_copy(remainder, client->_internal.options.module_id);
  }

  remainder = az_span_copy_u8(remainder, LF);

  AZ_RETURN_IF_FAILED(az_span_u32toa(remainder, token_expiration_epoch_time, &remainder));

  *out_signature = az_span_slice(signature, 0, _az_span_diff(remainder, signature));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_sas_get_password(
    az_iot_hub_client const* client,
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

  // Concatenates: "SharedAccessSignature sr=" scope "&sig=" sig  "&se=" expiration_time_secs
  //               plus, if key_name size > 0, "&skn=" key_name

  // This does not account for the size of `token_expiration_epoch_time`, which will be handled by az_span_u32toa.
  int32_t required_size = az_span_size(sr_string) + az_span_size(devices_string)
      + az_span_size(sig_string) + az_span_size(se_string)
      + az_span_size(client->_internal.iot_hub_hostname) + az_span_size(client->_internal.device_id)
      + az_span_size(base64_hmac_sha256_signature) + 6; // Number of ampersands, equal signs and null terminator.

  if (az_span_size(client->_internal.options.module_id) > 0)
  {
    required_size
        += az_span_size(client->_internal.options.module_id) + az_span_size(modules_string);
  }

  if (az_span_size(key_name) > 0)
  {
    required_size += az_span_size(skn_string) + az_span_size(key_name)
        + 2; // 2 is for one ampersand and one equal sign.
  }

  az_span remainder = az_span_init((uint8_t*)mqtt_password, (int32_t)mqtt_password_size);

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_size);

  // SharedAccessSignature
  remainder = az_span_copy(remainder, sr_string);
  remainder = az_span_copy_u8(remainder, EQUAL_SIGN);
  remainder = az_span_copy(remainder, client->_internal.iot_hub_hostname);

  // Device ID
  remainder = az_span_copy(remainder, devices_string);
  remainder = az_span_copy(remainder, client->_internal.device_id);

  // Module ID
  if (az_span_size(client->_internal.options.module_id) > 0)
  {
    remainder = az_span_copy(remainder, modules_string);
    remainder = az_span_copy(remainder, client->_internal.options.module_id);
  }

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

  if (out_mqtt_password_length != NULL)
  {
    *out_mqtt_password_length = (mqtt_password_size - (size_t)az_span_size(remainder));
  }

  remainder = az_span_copy_u8(remainder, '\0');

  return AZ_OK;
}

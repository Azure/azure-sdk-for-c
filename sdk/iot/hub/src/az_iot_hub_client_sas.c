// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_hub_client.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>

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

AZ_NODISCARD az_result az_iot_hub_client_sas_signature_get(
    az_iot_hub_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION(token_expiration_epoch_time > 0);
  AZ_PRECONDITION_VALID_SPAN(signature, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_signature);

  int32_t required_length = az_span_length(client->_internal.iot_hub_hostname)
      + az_span_length(client->_internal.device_id) + az_span_length(devices_string)
      + 1; // 1 is for one line feed character.

  if (az_span_length(client->_internal.options.module_id) > 0)
  {
    required_length
        += az_span_length(client->_internal.options.module_id) + az_span_length(modules_string);
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(signature, required_length);

  signature = az_span_append(signature, client->_internal.iot_hub_hostname);
  signature = az_span_append(signature, devices_string);
  signature = az_span_append(signature, client->_internal.device_id);

  if (az_span_length(client->_internal.options.module_id) > 0)
  {
    signature = az_span_append(signature, modules_string);
    signature = az_span_append(signature, client->_internal.options.module_id);
  }

  signature = az_span_append_uint8(signature, LF);

  AZ_RETURN_IF_FAILED(az_span_append_u32toa(signature, token_expiration_epoch_time, &signature));

  *out_signature = signature;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_hub_client_sas_password_get(
    az_iot_hub_client const* client,
    az_span base64_hmac_sha256_signature,
    uint32_t token_expiration_epoch_time,
    az_span key_name,
    az_span mqtt_password,
    az_span* out_mqtt_password)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(base64_hmac_sha256_signature, 1, false);
  AZ_PRECONDITION(token_expiration_epoch_time > 0);
  AZ_PRECONDITION_VALID_SPAN(mqtt_password, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_password);

  // Concatenates: "SharedAccessSignature sr=" scope "&sig=" sig  "&se=" expiration_time_secs
  //               plus, if key_name != NULL, "&skn=" key_name

  az_span sr_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SR);
  az_span sig_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SIG);
  az_span se_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SE);
  int32_t required_length = az_span_length(sr_string) + az_span_length(devices_string)
      + az_span_length(sig_string) + az_span_length(se_string)
      + az_span_length(client->_internal.iot_hub_hostname)
      + az_span_length(client->_internal.device_id) + az_span_length(base64_hmac_sha256_signature)
      + 5; // Number of ampersands and equal signs.

  if (az_span_length(client->_internal.options.module_id) > 0)
  {
    required_length
        += az_span_length(client->_internal.options.module_id) + az_span_length(modules_string);
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_password, required_length);

  // SharedAccessSignature
  mqtt_password = az_span_append(mqtt_password, sr_string);
  mqtt_password = az_span_append_uint8(mqtt_password, EQUAL_SIGN);
  mqtt_password = az_span_append(mqtt_password, client->_internal.iot_hub_hostname);

  // Device ID
  mqtt_password = az_span_append(mqtt_password, devices_string);
  mqtt_password = az_span_append(mqtt_password, client->_internal.device_id);

  // Module ID
  if (az_span_length(client->_internal.options.module_id) > 0)
  {
    mqtt_password = az_span_append(mqtt_password, modules_string);
    mqtt_password = az_span_append(mqtt_password, client->_internal.options.module_id);
  }

  // Signature
  mqtt_password = az_span_append_uint8(mqtt_password, AMPERSAND);
  mqtt_password = az_span_append(mqtt_password, sig_string);
  mqtt_password = az_span_append_uint8(mqtt_password, EQUAL_SIGN);
  mqtt_password = az_span_append(mqtt_password, base64_hmac_sha256_signature);

  // Expiration
  mqtt_password = az_span_append_uint8(mqtt_password, AMPERSAND);
  mqtt_password = az_span_append(mqtt_password, se_string);
  mqtt_password = az_span_append_uint8(mqtt_password, EQUAL_SIGN);
  AZ_RETURN_IF_FAILED(
      az_span_append_u32toa(mqtt_password, token_expiration_epoch_time, &mqtt_password));

  if (az_span_ptr(key_name) != NULL && az_span_length(key_name) > 0)
  {
    az_span skn_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SKN);
    required_length = az_span_length(skn_string) + az_span_length(key_name)
        + 2; // 2 is for one ampersand and one equal sign.

    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_password, required_length);

    // Key Name
    mqtt_password = az_span_append_uint8(mqtt_password, AMPERSAND);
    mqtt_password = az_span_append(mqtt_password, skn_string);
    mqtt_password = az_span_append_uint8(mqtt_password, EQUAL_SIGN);
    mqtt_password = az_span_append(mqtt_password, key_name);
  }

  *out_mqtt_password = mqtt_password;

  return AZ_OK;
}

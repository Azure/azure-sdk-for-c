// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_iot_sas_token.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg.h>

#define LF '\n'
#define AMPERSAND '&'
#define EQUAL_SIGN '='
#define SCOPE_DEVICES_STRING "/devices/"
#define SAS_TOKEN_SR "SharedAccessSignature sr"
#define SAS_TOKEN_SE "se"
#define SAS_TOKEN_SIG "sig"
#define SAS_TOKEN_SKN "skn"

static AZ_NODISCARD int32_t _az_span_diff(az_span new_span, az_span old_span)
{
  int32_t answer = az_span_size(old_span) - az_span_size(new_span);
  AZ_PRECONDITION(answer == (int32_t)(az_span_ptr(new_span) - az_span_ptr(old_span)));
  return answer;
}

az_result az_iot_sas_token_get_document(
    az_span iothub_fqdn,
    az_span device_id,
    int32_t expiry_time_secs,
    az_span document,
    az_span* out_document)
{
  AZ_PRECONDITION_VALID_SPAN(device_id, 1, false);
  AZ_PRECONDITION_VALID_SPAN(iothub_fqdn, 1, false);
  AZ_PRECONDITION_VALID_SPAN(document, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_document);

  az_span devices_string = AZ_SPAN_FROM_STR(SCOPE_DEVICES_STRING);
  int32_t required_length
      = az_span_size(iothub_fqdn) + az_span_size(device_id) + az_span_size(devices_string) + 1;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(document, required_length);

  az_span remainder = document;

  remainder = az_span_copy(remainder, iothub_fqdn);
  remainder = az_span_copy(remainder, devices_string);
  remainder = az_span_copy(remainder, device_id);
  remainder = az_span_copy_uint8(remainder, LF);

  AZ_RETURN_IF_FAILED(az_span_copy_i32toa(remainder, expiry_time_secs, &remainder));

  *out_document = az_span_slice(document, 0, _az_span_diff(remainder, document));

  return AZ_OK;
}

az_result az_iot_sas_token_generate(
    az_span iothub_fqdn,
    az_span device_id,
    az_span signature,
    int32_t expiry_time_secs,
    az_span key_name,
    az_span sas_token,
    az_span* out_sas_token)
{
  AZ_PRECONDITION_VALID_SPAN(device_id, 1, false);
  AZ_PRECONDITION_VALID_SPAN(iothub_fqdn, 1, false);
  AZ_PRECONDITION_VALID_SPAN(signature, 1, false);
  AZ_PRECONDITION(expiry_time_secs > 0);
  AZ_PRECONDITION_VALID_SPAN(sas_token, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_sas_token);

  // Concatenates: "SharedAccessSignature sr=" scope "&sig=" sig  "&se=" expiration_time_secs
  //               plus, if key_name != NULL, "&skn=" key_name

  az_span sr_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SR);
  az_span devices_string = AZ_SPAN_FROM_STR(SCOPE_DEVICES_STRING);
  az_span sig_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SIG);
  az_span se_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SE);
  int32_t required_length = az_span_size(sr_string) + az_span_size(devices_string)
      + az_span_size(sig_string) + az_span_size(se_string) + az_span_size(iothub_fqdn)
      + az_span_size(device_id) + az_span_size(signature) + 5;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(sas_token, required_length);

  az_span remainder = sas_token;

  // SharedAccessSignature
  remainder = az_span_copy(remainder, sr_string);
  remainder = az_span_copy_uint8(remainder, EQUAL_SIGN);
  remainder = az_span_copy(remainder, iothub_fqdn);
  remainder = az_span_copy(remainder, devices_string);
  remainder = az_span_copy(remainder, device_id);

  // Signature
  remainder = az_span_copy_uint8(remainder, AMPERSAND);
  remainder = az_span_copy(remainder, sig_string);
  remainder = az_span_copy_uint8(remainder, EQUAL_SIGN);
  remainder = az_span_copy(remainder, signature);

  // Expiration
  remainder = az_span_copy_uint8(remainder, AMPERSAND);
  remainder = az_span_copy(remainder, se_string);
  remainder = az_span_copy_uint8(remainder, EQUAL_SIGN);
  AZ_RETURN_IF_FAILED(az_span_copy_i32toa(remainder, expiry_time_secs, &remainder));

  if (az_span_ptr(key_name) != NULL && az_span_size(key_name) > 0)
  {
    az_span skn_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SKN);
    required_length = az_span_size(skn_string) + az_span_size(key_name) + 2;

    AZ_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);

    // Key Name
    remainder = az_span_copy_uint8(remainder, AMPERSAND);
    remainder = az_span_copy(remainder, skn_string);
    remainder = az_span_copy_uint8(remainder, EQUAL_SIGN);
    az_span_copy(remainder, key_name);
  }

  *out_sas_token = az_span_slice(sas_token, 0, _az_span_diff(remainder, sas_token));

  return AZ_OK;
}

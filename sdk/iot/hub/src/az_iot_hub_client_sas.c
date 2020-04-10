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
  int32_t required_length = az_span_length(iothub_fqdn) + az_span_length(device_id)
      + az_span_length(devices_string) + 1;

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(document, required_length);

  *out_document = document;

  *out_document = az_span_append(*out_document, iothub_fqdn);
  *out_document = az_span_append(*out_document, devices_string);
  *out_document = az_span_append(*out_document, device_id);
  *out_document = az_span_append_uint8(*out_document, LF);

  AZ_RETURN_IF_FAILED(az_span_append_i32toa(*out_document, expiry_time_secs, out_document));

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
  int32_t required_length = az_span_length(sr_string) + az_span_length(devices_string)
      + az_span_length(sig_string) + az_span_length(se_string) + az_span_length(iothub_fqdn)
      + az_span_length(device_id) + az_span_length(signature) + 5;

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(sas_token, required_length);

  *out_sas_token = sas_token;

  // SharedAccessSignature
  *out_sas_token = az_span_append(*out_sas_token, sr_string);
  *out_sas_token = az_span_append_uint8(*out_sas_token, EQUAL_SIGN);
  *out_sas_token = az_span_append(*out_sas_token, iothub_fqdn);
  *out_sas_token = az_span_append(*out_sas_token, devices_string);
  *out_sas_token = az_span_append(*out_sas_token, device_id);

  // Signature
  *out_sas_token = az_span_append_uint8(*out_sas_token, AMPERSAND);
  *out_sas_token = az_span_append(*out_sas_token, sig_string);
  *out_sas_token = az_span_append_uint8(*out_sas_token, EQUAL_SIGN);
  *out_sas_token = az_span_append(*out_sas_token, signature);

  // Expiration
  *out_sas_token = az_span_append_uint8(*out_sas_token, AMPERSAND);
  *out_sas_token = az_span_append(*out_sas_token, se_string);
  *out_sas_token = az_span_append_uint8(*out_sas_token, EQUAL_SIGN);
  AZ_RETURN_IF_FAILED(az_span_append_i32toa(*out_sas_token, expiry_time_secs, out_sas_token));

  if (az_span_ptr(key_name) != NULL && az_span_length(key_name) > 0)
  {
    az_span skn_string = AZ_SPAN_FROM_STR(SAS_TOKEN_SKN);
    required_length = az_span_length(skn_string) + az_span_length(key_name) + 2;

    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(*out_sas_token, required_length);

    // Key Name
    *out_sas_token = az_span_append_uint8(*out_sas_token, AMPERSAND);
    *out_sas_token = az_span_append(*out_sas_token, skn_string);
    *out_sas_token = az_span_append_uint8(*out_sas_token, EQUAL_SIGN);
    *out_sas_token = az_span_append(*out_sas_token, key_name);
  }

  return AZ_OK;
}

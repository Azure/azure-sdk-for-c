// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <az_iot_sas_token.h>
#include <az_contract_internal.h>
#include <az_span.h>
#include <_az_cfg.h>

#define LF                      '\n'
#define AMPERSAND               '&'
#define EQUAL_SIGN              '='
#define SCOPE_DEVICES_STRING    "/devices/"
#define SAS_TOKEN_SR            "SharedAccessSignature sr"
#define SAS_TOKEN_SE            "se"
#define SAS_TOKEN_SIG           "sig"
#define SAS_TOKEN_SKN           "skn"

az_result az_iot_sas_token_get_document(az_span iothub_fqdn, az_span device_id, int32_t expiry_time_secs, az_span document, az_span * out_document)
{
    AZ_CONTRACT_ARG_VALID_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_SPAN(document);
    AZ_CONTRACT_ARG_NOT_NULL(out_document);

    az_result result;

    *out_document = document;

    if (az_failed(result = az_span_append(*out_document, iothub_fqdn, out_document)) ||
        az_failed(result = az_span_append(*out_document, AZ_SPAN_FROM_STR(SCOPE_DEVICES_STRING), out_document)) ||
        az_failed(result = az_span_append(*out_document, device_id, out_document)) ||
        az_failed(result = az_span_append_uint8(*out_document, LF, out_document)) ||
        az_failed(result = az_span_append_i32toa(*out_document, expiry_time_secs, out_document)))
    {
        return result;
    }
    else
    {
        return AZ_OK;
    }
}

az_result az_iot_sas_token_generate(az_span iothub_fqdn, az_span device_id, az_span signature, int32_t expiry_time_secs, az_span key_name, az_span sas_token, az_span * out_sas_token)
{
    az_result result;

    AZ_CONTRACT_ARG_VALID_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_SPAN(signature);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(signature);
    AZ_CONTRACT(expiry_time_secs > 0, AZ_ERROR_ARG);
    AZ_CONTRACT_ARG_VALID_SPAN(sas_token);
    AZ_CONTRACT_ARG_NOT_NULL(out_sas_token);

    // Concatenates: "SharedAccessSignature sr=" scope "&sig=" sig  "&se=" expiration_time_secs 
    //               plus, if key_name != NULL, "&skn=" key_name

    *out_sas_token = sas_token;

    // SharedAccessSignature
    if (az_failed(result = az_span_append(*out_sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SR), out_sas_token)) ||
        az_failed(result = az_span_append_uint8(*out_sas_token, EQUAL_SIGN, out_sas_token)) ||
        az_failed(result = az_span_append(*out_sas_token, iothub_fqdn, out_sas_token)) ||
        az_failed(result = az_span_append(*out_sas_token, AZ_SPAN_FROM_STR(SCOPE_DEVICES_STRING), out_sas_token)) ||
        az_failed(result = az_span_append(*out_sas_token, device_id, out_sas_token)) ||
        // Signature
        az_failed(result = az_span_append_uint8(*out_sas_token, AMPERSAND, out_sas_token)) ||
        az_failed(result = az_span_append(*out_sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SIG), out_sas_token)) ||
        az_failed(result = az_span_append_uint8(*out_sas_token, EQUAL_SIGN, out_sas_token)) ||
        az_failed(result = az_span_append(*out_sas_token, signature, out_sas_token)) ||
        // Expiration
        az_failed(result = az_span_append_uint8(*out_sas_token, AMPERSAND, out_sas_token)) ||
        az_failed(result = az_span_append(*out_sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SE), out_sas_token)) ||
        az_failed(result = az_span_append_uint8(*out_sas_token, EQUAL_SIGN, out_sas_token)) ||
        az_failed(result = az_span_append_i32toa(*out_sas_token, expiry_time_secs, out_sas_token)))
    {
        return result;
    }

    if (az_span_ptr(key_name) != NULL && az_span_length(key_name) > 0)
    {
        // Key Name
        if (az_failed(result = az_span_append_uint8(*out_sas_token, AMPERSAND, out_sas_token)) ||
            az_failed(result = az_span_append(*out_sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SKN), out_sas_token)) ||
            az_failed(result = az_span_append_uint8(*out_sas_token, EQUAL_SIGN, out_sas_token)) ||
            az_failed(result = az_span_append(*out_sas_token, key_name, out_sas_token)))
        {
            return result;
        }
    }               

    return AZ_OK;
}

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <az_iot_sas_token.h>
#include <az_contract_internal.h>
#include <az_span.h>

#define LF                      '\n'
#define AMPERSAND               '&'
#define EQUAL_SIGN              '='
#define SCOPE_DEVICES_STRING    "/devices/"
#define SAS_TOKEN_SR            "SharedAccessSignature sr"
#define SAS_TOKEN_SE            "se"
#define SAS_TOKEN_SIG           "sig"
#define SAS_TOKEN_SKN           "skn"

az_result az_iot_sas_token_get_document(az_span iothub_fqdn, az_span device_id, int32_t expiry_time_secs, az_span* document)
{
    AZ_CONTRACT_ARG_VALID_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_NOT_NULL(document);

    az_result result;

    if (az_failed(result = az_span_append(*document, iothub_fqdn, document)) ||
        az_failed(result = az_span_append(*document, AZ_SPAN_FROM_STR(SCOPE_DEVICES_STRING), document)) ||
        az_failed(result = az_span_append(*document, device_id, document)) ||
        az_failed(result = az_span_append_byte(*document, LF, document)) ||
        az_failed(result = az_span_append_int32(document, expiry_time_secs)))
    {
        return result;
    }
    else
    {
        return AZ_OK;
    }
}

az_result az_iot_sas_token_generate(az_span iothub_fqdn, az_span device_id, az_span signature, int32_t expiry_time_secs, az_span key_name, az_span* sas_token)
{
    az_result result;

    AZ_CONTRACT_ARG_VALID_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(device_id);
    AZ_CONTRACT_ARG_VALID_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(iothub_fqdn);
    AZ_CONTRACT_ARG_VALID_SPAN(signature);
    AZ_CONTRACT_ARG_VALID_NON_EMPTY_SPAN(signature);
    AZ_CONTRACT(expiry_time_secs > 0, AZ_ERROR_ARG);
    AZ_CONTRACT_ARG_NOT_NULL(sas_token);

    // Concatenates: "SharedAccessSignature sr=" scope "&sig=" sig  "&se=" expiration_time_secs 
    //               plus, if key_name != NULL, "&skn=" key_name

    // SharedAccessSignature
    if (az_failed(result = az_span_append(*sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SR), sas_token)) ||
        az_failed(result = az_span_append_byte(*sas_token, EQUAL_SIGN, sas_token)) ||
        az_failed(result = az_span_append(*sas_token, iothub_fqdn, sas_token)) ||
        az_failed(result = az_span_append(*sas_token, AZ_SPAN_FROM_STR(SCOPE_DEVICES_STRING), sas_token)) ||
        az_failed(result = az_span_append(*sas_token, device_id, sas_token)) ||
        // Signature
        az_failed(result = az_span_append_byte(*sas_token, AMPERSAND, sas_token)) ||
        az_failed(result = az_span_append(*sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SIG), sas_token)) ||
        az_failed(result = az_span_append_byte(*sas_token, EQUAL_SIGN, sas_token)) ||
        az_failed(result = az_span_append(*sas_token, signature, sas_token)) ||
        // Expiration
        az_failed(result = az_span_append_byte(*sas_token, AMPERSAND, sas_token)) ||
        az_failed(result = az_span_append(*sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SE), sas_token)) ||
        az_failed(result = az_span_append_byte(*sas_token, EQUAL_SIGN, sas_token)) ||
        az_failed(result = az_span_append_int32(sas_token, expiry_time_secs)))
    {
        return result;
    }

    if (az_span_ptr(key_name) != NULL && az_span_length(key_name) > 0)
    {
        // Key Name
        if (az_failed(result = az_span_append_byte(*sas_token, AMPERSAND, sas_token)) ||
            az_failed(result = az_span_append(*sas_token, AZ_SPAN_FROM_STR(SAS_TOKEN_SKN), sas_token)) ||
            az_failed(result = az_span_append_byte(*sas_token, EQUAL_SIGN, sas_token)) ||
            az_failed(result = az_span_append(*sas_token, key_name, sas_token)))
        {
            return result;
        }
    }               

    return AZ_OK;
}

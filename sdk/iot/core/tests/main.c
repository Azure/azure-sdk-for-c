// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include "az_test.h"
#include "az_iot_sas_token_tests.c"

int exit_code = 0;

int main()
{
    az_iot_sas_token_get_document_NULL_document_fails();
    az_iot_sas_token_get_document_NULL_document_span_fails();
    az_iot_sas_token_get_document_empty_device_id_fails();
    az_iot_sas_token_get_document_empty_iothub_fqdn_fails();
    az_iot_sas_token_get_document_document_overflow_fails();
    az_iot_sas_token_get_document_succeeds();
    az_iot_sas_token_generate_empty_device_id_fails();
    az_iot_sas_token_generate_empty_iothub_fqdn_fails();
    az_iot_sas_token_generate_EMPTY_signature_fails();
    az_iot_sas_token_generate_NULL_sas_token_fails();
    az_iot_sas_token_generate_NULL_sas_token_span_fails();
    az_iot_sas_token_generate_sas_token_overflow_fails();
    az_iot_sas_token_generate_succeeds();
    az_iot_sas_token_generate_with_keyname_succeeds();

    return exit_code;
}

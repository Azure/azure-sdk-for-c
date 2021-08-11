// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_test_span.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_provisioning_client.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <azure/core/_az_cfg.h>

#define TEST_REGISTRATION_ID "myRegistrationId"
#define TEST_ID_SCOPE "0neFEEDC0DE"
#define TEST_OPERATION_ID "4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d"

static const az_span test_global_device_hostname
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

static void test_az_iot_provisioning_client_get_request_payload_no_custom_payload()
{
    az_iot_provisioning_client client = { 0 };
    az_result ret = az_iot_provisioning_client_init(
        &client,
        test_global_device_hostname,
        AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
        AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
        NULL);
    assert_int_equal(AZ_OK, ret);

   char expected_payload[]
      = "{\"registrationId\":\"" TEST_REGISTRATION_ID "\"}";

   char payload[128];
   memset(payload, 0xCC, sizeof(payload));
   size_t payload_len = 0xBAADC0DE;

   ret = az_iot_provisioning_client_get_request_payload(&client, AZ_SPAN_EMPTY, NULL, payload, sizeof(payload), &payload_len);
   assert_int_equal(AZ_OK, ret);
   assert_string_equal(expected_payload, payload);
   assert_int_equal((uint8_t)0xCC, (uint8_t)payload[strlen(payload) + 1]);
   assert_int_equal(payload_len, strlen(expected_payload));
}

int test_az_iot_provisioning_client_payload()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_no_custom_payload),
  };

  return cmocka_run_group_tests_name("az_iot_provisioning_client_payload", tests, NULL, NULL);
}

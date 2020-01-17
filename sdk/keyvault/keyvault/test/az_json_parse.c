// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>
#include <az_keyvault_key_bundle.h>
#include <az_pair.h>
#include <az_span.h>

#include "./az_test.h"

#include <_az_cfg.h>

void az_tags_test() {
  {
    uint8_t buffer[200] = { 0 };
    az_keyvault_key_bundle const * data = { 0 };
    az_span const json = AZ_STR("{\"tags\":{\"key\":\"value\", \"key2\":\"value2\"}}");

    az_result parse_result
        = az_keyvault_json_to_key_bundle(json, (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);

    TEST_ASSERT(parse_result == AZ_OK);
    az_pair * pair = (az_pair *)data->tags.data.begin;

    TEST_ASSERT(data->tags.is_present == true);
    TEST_ASSERT(data->tags.data.size == 2);

    TEST_ASSERT(az_span_is_equal(pair->key, AZ_STR("key")));
    TEST_ASSERT(az_span_is_equal(pair->value, AZ_STR("value")));

    pair += 1;
    TEST_ASSERT(az_span_is_equal(pair->key, AZ_STR("key2")));
    TEST_ASSERT(az_span_is_equal(pair->value, AZ_STR("value2")));
  }
  {
    uint8_t buffer[200] = { 0 };
    az_keyvault_key_bundle const * data = { 0 };
    az_span const json = AZ_STR("{\"nOtags\":{\"key\":\"value\", \"key2\":\"value2\"}}");

    az_result parse_result
        = az_keyvault_json_to_key_bundle(json, (az_mut_span)AZ_SPAN_FROM_ARRAY(buffer), &data);

    TEST_ASSERT(parse_result == AZ_ERROR_JSON_INVALID_STATE);
  }
}

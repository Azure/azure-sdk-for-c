// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_get.h>
#include <az_span.h>
#include <az_str.h>

#include "./az_test.h"

#include <_az_cfg.h>

void test_json_get_by_pointer() {
  {
    az_json_value value;
    TEST_ASSERT(az_json_get_by_pointer(AZ_STR("   57  "), AZ_STR(""), &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == 57);
  }
  {
    az_json_value value;
    TEST_ASSERT(
        az_json_get_by_pointer(AZ_STR("   57  "), AZ_STR("/"), &value) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_json_value value;
    TEST_ASSERT(az_json_get_by_pointer(AZ_STR(" {  \"\": true  } "), AZ_STR("/"), &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == true);
  }
  {
    az_json_value value;
    TEST_ASSERT(az_json_get_by_pointer(AZ_STR(" [  { \"\": true }  ] "), AZ_STR("/0/"), &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == true);
  }
  {
    static az_span const sample = AZ_CONST_STR( //
        "{\n"
        "  \"parameters\": {\n"
        "      \"subscriptionId\": \"{subscription-id}\",\n"
        "      \"resourceGroupName\" : \"res4303\",\n"
        "      \"accountName\" : \"sto7280\",\n"
        "      \"containerName\" : \"container8723\",\n"
        "      \"api-version\" : \"2019-04-01\",\n"
        "      \"monitor\" : \"true\",\n"
        "      \"LegalHold\" : {\n"
        "      \"tags\": [\n"
        "          \"tag1\",\n"
        "          \"tag2\",\n"
        "          \"tag3\"\n"
        "      ]\n"
        "    }\n"
        "  },\n"
        "  \"responses\": {\n"
        "    \"200\": {\n"
        "      \"body\": {\n"
        "          \"hasLegalHold\": false,\n"
        "          \"tags\" : []\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}\n");
    az_json_value value;
    TEST_ASSERT(
        az_json_get_by_pointer(sample, AZ_STR("/parameters/tags/2"), &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(az_span_eq(value.data.string, AZ_STR("tag3")));
  }
}

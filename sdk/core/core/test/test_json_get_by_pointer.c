// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>
#include <az_span.h>

#include <az_test.h>

#include <_az_cfg.h>

void test_json_get_by_pointer() {
  {
    az_json_token token;
    TEST_ASSERT(
        az_json_get_by_pointer(AZ_SPAN_FROM_STR("   57  "), AZ_SPAN_FROM_STR(""), &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_NUMBER);
    TEST_ASSERT(token.value.number == 57);
  }
  {
    az_json_token token;
    TEST_ASSERT(
        az_json_get_by_pointer(AZ_SPAN_FROM_STR("   57  "), AZ_SPAN_FROM_STR("/"), &token)
        == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_json_token token;
    TEST_ASSERT(
        az_json_get_by_pointer(
            AZ_SPAN_FROM_STR(" {  \"\": true  } "), AZ_SPAN_FROM_STR("/"), &token)
        == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
    TEST_ASSERT(token.value.boolean == true);
  }
  {
    az_json_token token;
    TEST_ASSERT(
        az_json_get_by_pointer(
            AZ_SPAN_FROM_STR(" [  { \"\": true }  ] "), AZ_SPAN_FROM_STR("/0/"), &token)
        == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
    TEST_ASSERT(token.value.boolean == true);
  }
  {
    az_json_token token;
    TEST_ASSERT(
        az_json_get_by_pointer(
            AZ_SPAN_FROM_STR("{ \"2/00\": true } "), AZ_SPAN_FROM_STR("/2~100"), &token)
        == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
    TEST_ASSERT(token.value.boolean == true);
  }
  {
    static az_span const sample = AZ_SPAN_LITERAL_FROM_STR( //
        "{\n"
        "  \"parameters\": {\n"
        "      \"subscriptionId\": \"{subscription-id}\",\n"
        "      \"resourceGroupName\" : \"res4303\",\n"
        "      \"accountName\" : \"sto7280\",\n"
        "      \"containerName\" : \"container8723\",\n"
        "      \"api-version\" : \"2019-04-01\",\n"
        "      \"monitor\" : \"true\",\n"
        "      \"LegalHold\" : {\n"
        "        \"tags\": [\n"
        "          \"tag1\",\n"
        "          \"tag2\",\n"
        "          \"tag3\"\n"
        "        ]\n"
        "      }\n"
        "  },\n"
        "  \"responses\": {\n"
        "    \"2/00\": {\n"
        "      \"body\": {\n"
        "          \"hasLegalHold\": false,\n"
        "          \"tags\" : []\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}\n");
    {
      az_json_token token;
      TEST_ASSERT(
          az_json_get_by_pointer(sample, AZ_SPAN_FROM_STR("/parameters/LegalHold/tags/2"), &token)
          == AZ_OK);
      TEST_ASSERT(token.kind == AZ_JSON_TOKEN_STRING);
      TEST_ASSERT(az_span_is_equal(token.value.string, AZ_SPAN_FROM_STR("tag3")));
    }
    {
      az_json_token token;
      TEST_ASSERT(
          az_json_get_by_pointer(
              sample, AZ_SPAN_FROM_STR("/responses/2~100/body/hasLegalHold"), &token)
          == AZ_OK);
      TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
      TEST_ASSERT(token.value.boolean == false);
    }
  }
}

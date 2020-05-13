// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include "az_test_definitions.h"
#include <az_json.h>
#include <az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

/** Json **/
static void test_json_token_null(void** state)
{
  (void)state;
  az_json_token token = az_json_token_null();
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NULL);
  assert_false(token._internal.boolean);
  assert_int_equal(token._internal.number, 0);

  assert_ptr_equal(az_span_ptr(token._internal.string), NULL);
  assert_int_equal(az_span_size(token._internal.string), 0);

  assert_ptr_equal(az_span_ptr(token._internal.span), NULL);
  assert_int_equal(az_span_size(token._internal.span), 0);
}

static void test_json_token_boolean(void** state)
{
  (void)state;
  az_json_token token = az_json_token_boolean(true);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_TRUE);
  assert_true(token._internal.boolean);
}

static void test_json_token_number(void** state)
{
  (void)state;
  az_json_token token = az_json_token_number(10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  assert_int_equal(token._internal.number, 10);

  token = az_json_token_number(-10);
  assert_int_equal(token.kind, AZ_JSON_TOKEN_NUMBER);
  // Need to create double and not just use -10 directly or it would fail on x86-intel
  double expected = -10;
  assert_int_equal(token._internal.number, expected);
}

static void test_json_parser_init(void** state)
{
  (void)state;
  az_span span = AZ_SPAN_FROM_STR("");
  az_json_parser parser;
  assert_int_equal(az_json_parser_init(&parser, span), AZ_OK);
}

/**  Json builder **/
static void test_json_builder(void** state)
{
  (void)state;
  {
    uint8_t array[200];
    az_json_builder builder = { 0 };

    TEST_EXPECT_SUCCESS(az_json_builder_init(&builder, AZ_SPAN_FROM_BUFFER(array), NULL));

    // 0___________________________________________________________________________________________________1
    // 0_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
    // {"name":true,"foo":["bar",null,0,-12],"int-max":9007199254740991,"esc":"_\"_\\_\b\f\n\r\t_","u":"a\u001Fb"}
    TEST_EXPECT_SUCCESS(az_json_builder_append_begin_object(&builder));

    TEST_EXPECT_SUCCESS(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("name")));
    TEST_EXPECT_SUCCESS(az_json_builder_append_bool(&builder, true));

    {
      TEST_EXPECT_SUCCESS(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("foo")));
      TEST_EXPECT_SUCCESS(az_json_builder_append_begin_array(&builder));
      az_result e = az_json_builder_append_string(&builder, AZ_SPAN_FROM_STR("bar"));
      TEST_EXPECT_SUCCESS(e);
      TEST_EXPECT_SUCCESS(az_json_builder_append_null(&builder));
      TEST_EXPECT_SUCCESS(az_json_builder_append_int32_number(&builder, 0));
      TEST_EXPECT_SUCCESS(az_json_builder_append_int32_number(&builder, -12));
      TEST_EXPECT_SUCCESS(az_json_builder_append_end_array(&builder));
    }

    TEST_EXPECT_SUCCESS(
        az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("int-max")));
    TEST_EXPECT_SUCCESS(az_json_builder_append_int32_number(&builder, 2147483647));

    TEST_EXPECT_SUCCESS(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("esc")));
    TEST_EXPECT_SUCCESS(
        az_json_builder_append_string(&builder, AZ_SPAN_FROM_STR("_\"_\\_\b\f\n\r\t_")));

    TEST_EXPECT_SUCCESS(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("u")));
    TEST_EXPECT_SUCCESS(az_json_builder_append_string(
        &builder,
        AZ_SPAN_FROM_STR( //
            "a"
            "\x1f"
            "b")));

    TEST_EXPECT_SUCCESS(az_json_builder_append_end_object(&builder));

    az_span_to_str((char*)array, 200, az_json_builder_get_json(&builder));

    assert_string_equal(
        array,
        "{"
        "\"name\":true,"
        "\"foo\":[\"bar\",null,0,-12],"
        "\"int-max\":2147483647,"
        "\"esc\":\"_\\\"_\\\\_\\b\\f\\n\\r\\t_\","
        "\"u\":\"a\\u001Fb\""
        "}");
  }
  {
    // json with AZ_JSON_TOKEN_STRING
    uint8_t array[200];
    az_json_builder builder = { 0 };
    TEST_EXPECT_SUCCESS(az_json_builder_init(&builder, AZ_SPAN_FROM_BUFFER(array), NULL));

    // this json { "span": "\" } would be scaped to { "span": "\\"" }
    uint8_t single_char[1] = { '\\' }; // char = '\'
    az_span single_span = AZ_SPAN_FROM_BUFFER(single_char);

    TEST_EXPECT_SUCCESS(az_json_builder_append_begin_object(&builder));

    TEST_EXPECT_SUCCESS(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("span")));
    TEST_EXPECT_SUCCESS(az_json_builder_append_string(&builder, single_span));

    TEST_EXPECT_SUCCESS(az_json_builder_append_end_object(&builder));

    az_span expected = AZ_SPAN_FROM_STR("{"
                                        "\"span\":\"\\\\\""
                                        "}");

    assert_true(az_span_is_content_equal(az_json_builder_get_json(&builder), expected));
  }
  {
    // json with array and object inside
    uint8_t array[200];
    az_json_builder builder = { 0 };
    TEST_EXPECT_SUCCESS(az_json_builder_init(&builder, AZ_SPAN_FROM_BUFFER(array), NULL));

    // this json { "array": [1, 2, {}, 3 ] }
    TEST_EXPECT_SUCCESS(az_json_builder_append_begin_object(&builder));

    TEST_EXPECT_SUCCESS(az_json_builder_append_property_name(&builder, AZ_SPAN_FROM_STR("array")));
    TEST_EXPECT_SUCCESS(az_json_builder_append_begin_array(&builder));

    TEST_EXPECT_SUCCESS(az_json_builder_append_int32_number(&builder, 1));
    TEST_EXPECT_SUCCESS(az_json_builder_append_int32_number(&builder, 2));

    TEST_EXPECT_SUCCESS(az_json_builder_append_begin_object(&builder));
    TEST_EXPECT_SUCCESS(az_json_builder_append_end_object(&builder));

    TEST_EXPECT_SUCCESS(az_json_builder_append_int32_number(&builder, 3));

    TEST_EXPECT_SUCCESS(az_json_builder_append_end_array(&builder));
    TEST_EXPECT_SUCCESS(az_json_builder_append_end_object(&builder));

    assert_true(az_span_is_content_equal(
        az_json_builder_get_json(&builder),
        AZ_SPAN_FROM_STR( //
            "{"
            "\"array\":[1,2,{},3]"
            "}")));
  }
  {
    uint8_t nested_object_array[200];
    az_json_builder nested_object_builder = { 0 };
    {
      // 0___________________________________________________________________________________________________1
      // 0_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0
      // 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
      // {"bar":true}
      TEST_EXPECT_SUCCESS(az_json_builder_init(
          &nested_object_builder, AZ_SPAN_FROM_BUFFER(nested_object_array), NULL));
      TEST_EXPECT_SUCCESS(az_json_builder_append_begin_object(&nested_object_builder));
      TEST_EXPECT_SUCCESS(
          az_json_builder_append_property_name(&nested_object_builder, AZ_SPAN_FROM_STR("bar")));
      TEST_EXPECT_SUCCESS(az_json_builder_append_bool(&nested_object_builder, true));
      TEST_EXPECT_SUCCESS(az_json_builder_append_end_object(&nested_object_builder));

      assert_true(az_span_is_content_equal(
          az_json_builder_get_json(&nested_object_builder),
          AZ_SPAN_FROM_STR( //
              "{"
              "\"bar\":true"
              "}")));
    }
  }
}

/** Json get by pointer **/
static void test_json_get_by_pointer(void** state)
{
  (void)state;
  {
    az_json_token token;
    assert_true(
        az_json_parse_by_pointer(AZ_SPAN_FROM_STR("   57  "), AZ_SPAN_FROM_STR(""), &token)
        == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = 57;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
  }
  {
    az_json_token token;
    assert_true(
        az_json_parse_by_pointer(AZ_SPAN_FROM_STR("   57  "), AZ_SPAN_FROM_STR("/"), &token)
        == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_json_token token;
    assert_true(
        az_json_parse_by_pointer(
            AZ_SPAN_FROM_STR(" {  \"\": true  } "), AZ_SPAN_FROM_STR("/"), &token)
        == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_TRUE);
    assert_true(token._internal.boolean == true);
  }
  {
    az_json_token token;
    assert_true(
        az_json_parse_by_pointer(
            AZ_SPAN_FROM_STR(" [  { \"\": true }  ] "), AZ_SPAN_FROM_STR("/0/"), &token)
        == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_TRUE);
    assert_true(token._internal.boolean == true);
  }
  {
    az_json_token token;
    assert_true(
        az_json_parse_by_pointer(
            AZ_SPAN_FROM_STR("{ \"2/00\": true } "), AZ_SPAN_FROM_STR("/2~100"), &token)
        == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_TRUE);
    assert_true(token._internal.boolean == true);
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
      assert_true(
          az_json_parse_by_pointer(sample, AZ_SPAN_FROM_STR("/parameters/LegalHold/tags/2"), &token)
          == AZ_OK);
      assert_true(token.kind == AZ_JSON_TOKEN_STRING);
      assert_true(az_span_is_content_equal(token._internal.string, AZ_SPAN_FROM_STR("tag3")));
    }
    {
      az_json_token token;
      assert_true(
          az_json_parse_by_pointer(
              sample, AZ_SPAN_FROM_STR("/responses/2~100/body/hasLegalHold"), &token)
          == AZ_OK);
      assert_true(token.kind == AZ_JSON_TOKEN_FALSE);
      assert_true(token._internal.boolean == false);
    }
  }
}

/** Json parser **/
az_result read_write(az_span input, az_span* output, int32_t* o);
az_result read_write_token(
    az_span* output,
    int32_t* written,
    int32_t* o,
    az_json_parser* state,
    az_json_token token);
az_result write_str(az_span span, az_span s, az_span* out, int32_t* written);

static az_span const sample1 = AZ_SPAN_LITERAL_FROM_STR( //
    "{\n"
    "  \"parameters\": {\n"
    "    \"subscriptionId\": \"{subscription-id}\",\n"
    "      \"resourceGroupName\" : \"res4303\",\n"
    "      \"accountName\" : \"sto7280\",\n"
    "      \"containerName\" : \"container8723\",\n"
    "      \"api-version\" : \"2019-04-01\",\n"
    "      \"monitor\" : \"true\",\n"
    "      \"LegalHold\" : {\n"
    "      \"tags\": [\n"
    "        \"tag1\",\n"
    "          \"tag2\",\n"
    "          \"tag3\"\n"
    "      ]\n"
    "    }\n"
    "  },\n"
    "    \"responses\": {\n"
    "    \"200\": {\n"
    "      \"body\": {\n"
    "        \"hasLegalHold\": false,\n"
    "          \"tags\" : []\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n");

static void test_json_parser(void** state)
{
  (void)state;
  /* {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("    ")));
    assert_true(az_json_parser_parse_token(&parser, NULL) == AZ_ERROR_ARG);
  }
  {
    az_json_token token;
    assert_true(az_json_parser_parse_token(NULL, &token) == AZ_ERROR_ARG);
  } */
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("    ")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&parser, &token) == AZ_ERROR_EOF);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  null  ")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&parser, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NULL);
    assert_true(az_json_parser_done(&parser) == AZ_OK);
  }
  {
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("  nul")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_ERROR_EOF);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  false")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&parser, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_FALSE);
    assert_true(token._internal.boolean == false);
    assert_true(az_json_parser_done(&parser) == AZ_OK);
  }
  {
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("  falsx  ")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("true ")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_TRUE);
    assert_true(token._internal.boolean == true);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("  truem")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR(" \"tr\\\"ue\\t\" ");
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, s));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_STRING);
    assert_true(az_span_ptr(token._internal.string) == (az_span_ptr(s) + 2));
    assert_true(az_span_size(token._internal.string) == 8);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0F\"");
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, s));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_STRING);
    assert_true(az_span_ptr(token._internal.string) == az_span_ptr(s) + 1);
    assert_true(az_span_size(token._internal.string) == 6);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0\"");
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, s));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  /* Testing parsing number and converting to double (_az_json_number_to_double) */
  {
    // no exp number, decimal only
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR(" 23 ")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = 23;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    // negative number with decimals
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR(" -23.56")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = -23.56;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    // negative + decimals + exp
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR(" -23.56e-3")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = -0.02356;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    // exp
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("1e50")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);
  }
  {
    // big decimal + exp
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(
        az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("10000000000000000000000e17")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);
  }
  {
    // exp inf -> Any value above double MAX range would be translated to positive inf
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("1e309")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    // Create inf number with  IEEE 754 standard
    // floating point number containing all zeroes in the mantissa (first twenty-three bits), and
    // all ones in the exponent (next eight bits)
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    unsigned int p = 0x7F800000; // 0xFF << 23
    float positiveInfinity = *(float*)&p;

    double const expected = positiveInfinity;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);

    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    // exp inf -> Any value below double MIN range would be translated 0
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("1e-400")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = 0;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    // negative exp
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR("1e-18")));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = 0.000000000000000001;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  /* end of Testing parsing number and converting to double */
  {
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, AZ_SPAN_FROM_STR(" [ true, 0.25 ]")));
    az_json_token token = { 0 };
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_BEGIN_ARRAY);
    assert_true(az_json_parser_parse_array_item(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_TRUE);
    assert_true(token._internal.boolean == true);
    assert_true(az_json_parser_parse_array_item(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_NUMBER);

    double const expected = 0.25;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&token._internal.number;

    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
    assert_true(az_json_parser_parse_array_item(&json_state, &token) == AZ_ERROR_ITEM_NOT_FOUND);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    az_span const json = AZ_SPAN_FROM_STR("{\"a\":\"Hello world!\"}");
    az_json_parser json_state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&json_state, json));
    az_json_token token;
    assert_true(az_json_parser_parse_token(&json_state, &token) == AZ_OK);
    assert_true(token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT);
    az_json_token_member token_member;
    assert_true(az_json_parser_parse_token_member(&json_state, &token_member) == AZ_OK);
    assert_true(az_span_ptr(token_member.name) == az_span_ptr(json) + 2);
    assert_true(az_span_size(token_member.name) == 1);
    assert_true(token_member.token.kind == AZ_JSON_TOKEN_STRING);
    assert_true(az_span_ptr(token_member.token._internal.string) == az_span_ptr(json) + 6);
    assert_true(az_span_size(token_member.token._internal.string) == 12);
    assert_true(
        az_json_parser_parse_token_member(&json_state, &token_member) == AZ_ERROR_ITEM_NOT_FOUND);
    assert_true(az_json_parser_done(&json_state) == AZ_OK);
  }
  {
    uint8_t buffer[1000];
    az_span output = AZ_SPAN_FROM_BUFFER(buffer);
    {
      int32_t o = 0;
      assert_true(
          read_write(AZ_SPAN_FROM_STR("{ \"a\" : [ true, { \"b\": [{}]}, 15 ] }"), &output, &o)
          == AZ_OK);

      assert_true(
          az_span_is_content_equal(output, AZ_SPAN_FROM_STR("{\"a\":[true,{\"b\":[{}]},0]}")));
    }
    {
      int32_t o = 0;
      output = AZ_SPAN_FROM_BUFFER(buffer);
      az_span const json = AZ_SPAN_FROM_STR(
          // 0           1           2           3           4           5 6
          // 01234 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234
          // 56789 0123
          "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
          "[[[[[ [[[[");
      az_result const result = read_write(json, &output, &o);
      assert_true(result == AZ_ERROR_JSON_NESTING_OVERFLOW);
    }
    {
      int32_t o = 0;
      output = AZ_SPAN_FROM_BUFFER(buffer);
      az_span const json = AZ_SPAN_FROM_STR(
          // 0           1           2           3           4           5 6 01234
          // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
          "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
          "[[[[[ [[[");
      az_result const result = read_write(json, &output, &o);
      assert_true(result == AZ_ERROR_EOF);
    }
    {
      int32_t o = 0;
      az_span const json = AZ_SPAN_FROM_STR(
          // 0           1           2           3           4           5 6 01234
          // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
          "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
          "[[[[[ [[{"
          "   \"\\t\\n\": \"\\u0abc\"   "
          "}]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] "
          "]]]]] ]]]");
      output = AZ_SPAN_FROM_BUFFER(buffer);
      az_result const result = read_write(json, &output, &o);
      assert_true(result == AZ_OK);

      assert_true(az_span_is_content_equal(
          output,
          AZ_SPAN_FROM_STR( //
              "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[{"
              "\"\\t\\n\":\"\\u0abc\""
              "}]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
              "]")));
    }
    //
    {
      int32_t o = 0;
      output = AZ_SPAN_FROM_BUFFER(buffer);
      az_result const result = read_write(sample1, &output, &o);
      assert_true(result == AZ_OK);
    }
  }
}

// Aux funtions
az_result read_write_token(
    az_span* output,
    int32_t* written,
    int32_t* o,
    az_json_parser* state,
    az_json_token token)
{
  switch (token.kind)
  {
    case AZ_JSON_TOKEN_NULL:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 4);
      *output = az_span_copy(*output, AZ_SPAN_FROM_STR("null"));
      *written += 4;
      return AZ_OK;
    }
    case AZ_JSON_TOKEN_TRUE:
    {
      int32_t required_length = 4;
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, required_length);
      *output = az_span_copy(*output, AZ_SPAN_FROM_STR("true"));
      *written += required_length;
      return AZ_OK;
    }
    case AZ_JSON_TOKEN_FALSE:
    {
      int32_t required_length = 5;
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, required_length);
      *output = az_span_copy(*output, AZ_SPAN_FROM_STR("false"));
      *written += required_length;
      return AZ_OK;
    }
    case AZ_JSON_TOKEN_NUMBER:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
      *output = az_span_copy_u8(*output, '0');
      *written += 1;
      return AZ_OK;
    }
    case AZ_JSON_TOKEN_STRING:
    {
      return write_str(*output, token._internal.string, output, written);
    }
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
      *output = az_span_copy_u8(*output, '{');
      *written += 1;
      bool need_comma = false;
      while (true)
      {
        az_json_token_member member;
        az_result const result = az_json_parser_parse_token_member(state, &member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        if (need_comma)
        {
          AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
          *output = az_span_copy_u8(*output, ',');
          *written += 1;
        }
        else
        {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(write_str(*output, member.name, output, written));
        AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
        *output = az_span_copy_u8(*output, ':');
        *written += 1;
        AZ_RETURN_IF_FAILED(read_write_token(output, written, o, state, member.token));
      }
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
      *output = az_span_copy_u8(*output, '}');
      *written += 1;
      return AZ_OK;
    }
    case AZ_JSON_TOKEN_BEGIN_ARRAY:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
      *output = az_span_copy_u8(*output, '[');
      *written += 1;
      bool need_comma = false;
      while (true)
      {
        az_json_token element;
        az_result const result = az_json_parser_parse_array_item(state, &element);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        if (need_comma)
        {
          AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
          *output = az_span_copy_u8(*output, ',');
          *written += 1;
        }
        else
        {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(read_write_token(output, written, o, state, element));
      }
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
      *output = az_span_copy_u8(*output, ']');
      *written += 1;
      return AZ_OK;
    }
    default:
      break;
  }
  return AZ_ERROR_JSON_INVALID_STATE;
}

az_result read_write(az_span input, az_span* output, int32_t* o)
{
  az_json_parser parser = { 0 };
  TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, input));
  az_json_token token;
  AZ_RETURN_IF_FAILED(az_json_parser_parse_token(&parser, &token));
  int32_t written = 0;
  az_span output_copy = *output;
  AZ_RETURN_IF_FAILED(read_write_token(&output_copy, &written, o, &parser, token));
  *output = az_span_slice(*output, 0, written);
  return az_json_parser_done(&parser);
}

az_result write_str(az_span span, az_span s, az_span* out, int32_t* written)
{
  *out = span;
  int32_t required_length = az_span_size(s) + 2;

  AZ_RETURN_IF_NOT_ENOUGH_SIZE(*out, required_length);
  *out = az_span_copy_u8(*out, '"');
  *out = az_span_copy(*out, s);
  *out = az_span_copy_u8(*out, '"');

  *written += required_length;
  return AZ_OK;
}

/** Json pointer **/
static void test_json_pointer(void** state)
{
  (void)state;
  {
    az_span parser = AZ_SPAN_FROM_STR("");
    az_span p;
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("Hello");
    az_span p;
    assert_true(
        _az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/abc");
    az_span p;
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("abc")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        assert_true(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i);
      assert_true(az_span_is_content_equal(b, AZ_SPAN_FROM_STR("abc")));
    }
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/abc//dffgg21");
    az_span p;
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("abc")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        assert_true(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i);
      assert_true(az_span_is_content_equal(b, AZ_SPAN_FROM_STR("abc")));
    }
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("")));
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("dffgg21")));
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/ab~1c/dff~0x");
    az_span p;
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("ab~1c")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        assert_true(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i);
      assert_true(az_span_is_content_equal(b, AZ_SPAN_FROM_STR("ab/c")));
    }
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("dff~0x")));
    // test az_json_pointer_token_parser_get
    {
      az_span token_parser = p;
      uint8_t buffer[10];
      int i = 0;
      while (true)
      {
        uint32_t code_point;
        az_result const result
            = _az_span_reader_read_json_pointer_token_char(&token_parser, &code_point);
        if (result == AZ_ERROR_ITEM_NOT_FOUND)
        {
          break;
        }
        assert_true(result == AZ_OK);
        buffer[i] = (uint8_t)code_point;
        ++i;
      }
      az_span const b = az_span_init(buffer, i);
      assert_true(az_span_is_content_equal(b, AZ_SPAN_FROM_STR("dff~x")));
    }
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/ab~1c/dff~x");
    az_span p;
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("ab~1c")));
    assert_true(
        _az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span parser = AZ_SPAN_FROM_STR("/ab~1c/dff~");
    az_span p;
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_OK);
    assert_true(az_span_is_content_equal(p, AZ_SPAN_FROM_STR("ab~1c")));
    assert_true(_az_span_reader_read_json_pointer_token(&parser, &p) == AZ_ERROR_EOF);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("~");
    uint32_t c;
    assert_true(_az_span_reader_read_json_pointer_token_char(&token_parser, &c) == AZ_ERROR_EOF);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("");
    uint32_t c;
    assert_true(
        _az_span_reader_read_json_pointer_token_char(&token_parser, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("/");
    uint32_t c;
    assert_true(
        _az_span_reader_read_json_pointer_token_char(&token_parser, &c)
        == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  // test az_json_pointer_token_parser_get
  {
    az_span token_parser = AZ_SPAN_FROM_STR("~2");
    uint32_t c;
    assert_true(
        _az_span_reader_read_json_pointer_token_char(&token_parser, &c)
        == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
}

/** Json String **/
static void test_json_string(void** state)
{
  (void)state;
  {
    az_span const s = AZ_SPAN_FROM_STR("tr\\\"ue\\t");
    az_span reader = s;
    uint32_t c;
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 't');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 'r');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == '\"');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 'u');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 'e');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == '\t');
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\\uFf0F");
    az_span reader = s;
    uint32_t c = { 0 };
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_OK);
    assert_true(c == 0xFF0F);
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_ITEM_NOT_FOUND);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\\uFf0");
    az_span reader = s;
    uint32_t c;
    assert_true(_az_span_reader_read_json_string_char(&reader, &c) == AZ_ERROR_EOF);
  }
}

/** Json Value **/
static void test_json_value(void** state)
{
  (void)state;

  az_json_token const json_boolean = az_json_token_boolean(true);
  az_json_token const json_number = az_json_token_number(-42.3);
  az_json_token const json_string = az_json_token_string(AZ_SPAN_FROM_STR("Hello"));

  // boolean from boolean
  {
    bool boolean_value = false;
    assert_true(az_json_token_get_boolean(&json_boolean, &boolean_value) == AZ_OK);
    assert_true(boolean_value);
  }
  // boolean from number
  {
    bool boolean_value = false;
    assert_true(az_json_token_get_boolean(&json_number, &boolean_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // string from string
  {
    az_span string_value = { 0 };
    assert_true(az_json_token_get_string(&json_string, &string_value) == AZ_OK);
    assert_true(az_span_is_content_equal(string_value, AZ_SPAN_FROM_STR("Hello")));
  }
  // string from boolean
  {
    az_span string_value = { 0 };
    assert_true(az_json_token_get_string(&json_boolean, &string_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }

  // number from number
  {
    double number_value = 0.79;
    uint64_t const* const number_value_bin_rep_view = (uint64_t*)&number_value;

    assert_true(az_json_token_get_number(&json_number, &number_value) == AZ_OK);

    double const expected_value = -42.3;
    uint64_t const* const expected_value_bin_rep_view = (uint64_t const*)&expected_value;

    assert_true(*number_value_bin_rep_view == *expected_value_bin_rep_view);
  }
  // number from string
  {
    double number_value = 0.79;
    assert_true(az_json_token_get_number(&json_string, &number_value) == AZ_ERROR_ITEM_NOT_FOUND);
  }
}

int test_az_json()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_json_token_null),   cmocka_unit_test(test_json_token_boolean),
    cmocka_unit_test(test_json_token_number), cmocka_unit_test(test_json_parser_init),
    cmocka_unit_test(test_json_builder),      cmocka_unit_test(test_json_get_by_pointer),
    cmocka_unit_test(test_json_parser),       cmocka_unit_test(test_json_pointer),
    cmocka_unit_test(test_json_string),       cmocka_unit_test(test_json_value),
  };
  return cmocka_run_group_tests_name("az_core_json", tests, NULL, NULL);
}

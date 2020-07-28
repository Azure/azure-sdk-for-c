// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_json.h>
#include <azure/core/az_span.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <azure/core/_az_cfg.h>

#define TEST_EXPECT_SUCCESS(exp) assert_true(az_succeeded(exp))

static void test_json_token_helper(
    az_json_token token,
    az_json_token_kind expected_token_kind,
    az_span expected_token_slice)
{
  assert_int_equal(token.kind, expected_token_kind);
  assert_true(az_span_is_content_equal(token.slice, expected_token_slice));
}

static void test_json_parser_init(void** state)
{
  (void)state;

  az_json_parser_options options = az_json_parser_options_default();

  az_json_parser parser = { 0 };

  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("{}"), NULL), AZ_OK);
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("{}"), &options), AZ_OK);

  // Verify that initialization doesn't process any JSON text, even if it is invalid or incomplete.
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" "), NULL), AZ_OK);
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" "), &options), AZ_OK);
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("a"), NULL), AZ_OK);
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("a"), &options), AZ_OK);
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("\""), NULL), AZ_OK);
  assert_int_equal(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("\""), &options), AZ_OK);

  test_json_token_helper(parser.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
}

/**  Json builder **/
static void test_json_builder(void** state)
{
  (void)state;
  {
    uint8_t array[200] = { 0 };
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
    uint8_t array[200] = { 0 };
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
    uint8_t array[200] = { 0 };
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
    uint8_t nested_object_array[200] = { 0 };
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
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("    "), NULL));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_EOF);
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  null  "), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NULL, AZ_SPAN_FROM_STR("null"));
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  nul"), NULL));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_EOF);
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  false"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_FALSE, AZ_SPAN_FROM_STR("false"));
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  falsx  "), NULL));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("true "), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_TRUE, AZ_SPAN_FROM_STR("true"));
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  truem"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_TRUE, AZ_SPAN_FROM_STR("true"));
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  123a"), NULL));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR(" \"tr\\\"ue\\t\" ");
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, s, NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_STRING, AZ_SPAN_FROM_STR("tr\\\"ue\\t"));
    assert_true(az_span_ptr(parser.token.slice) == (az_span_ptr(s) + 2));
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0F\"");
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, s, NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_STRING, AZ_SPAN_FROM_STR("\\uFf0F"));
    assert_true(az_span_ptr(parser.token.slice) == az_span_ptr(s) + 1);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0\"");
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, s, NULL));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  /* Testing parsing number and converting to double (_az_json_number_to_double) */
  {
    // no exp number, no decimal, integer only
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" 23 "), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("23"));

    uint64_t const expected_64 = 23;
    uint64_t token_value_number_bin_rep_view = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_uint64(&parser.token, &token_value_number_bin_rep_view));
    assert_true(token_value_number_bin_rep_view == expected_64);

    uint32_t const expected_32 = (uint32_t)expected_64;
    uint32_t actual_number = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_uint32(&parser.token, &actual_number));
    assert_true(actual_number == expected_32);
  }
  {
    // no exp number, no decimal, negative integer only
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" -23 "), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("-23"));
  }
  {
    // negative number with decimals
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" -23.56"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("-23.56"));

    // TODO: Add back tests that validate az_json_token_get_double result when double support is
    // enabled.
  }
  {
    // negative + decimals + exp
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" -23.56e-3"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("-23.56e-3"));
  }
  {
    // exp
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("1e50"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e50"));
  }
  {
    // big decimal + exp
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(
        az_json_parser_init(&parser, AZ_SPAN_FROM_STR("10000000000000000000000e17"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(
        parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("10000000000000000000000e17"));
  }
  {
    // exp inf -> Any value above double MAX range would be translated to positive inf
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("1e309"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e309"));
  }
  {
    // exp inf -> Any value below double MIN range would be translated 0
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("1e-400"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e-400"));
  }
  {
    // negative exp
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("1e-18"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e-18"));
  }
  /* end of Testing parsing number and converting to double */
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR(" [ true, 0.25 ]"), NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_BEGIN_ARRAY, AZ_SPAN_FROM_STR("["));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_TRUE, AZ_SPAN_FROM_STR("true"));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("0.25"));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_END_ARRAY, AZ_SPAN_FROM_STR("]"));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_JSON_PARSING_DONE);
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_END_ARRAY, AZ_SPAN_FROM_STR("]"));
  }
  {
    az_span const json = AZ_SPAN_FROM_STR("{\"a\":\"Hello world!\"}");
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, json, NULL));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_BEGIN_OBJECT, AZ_SPAN_FROM_STR("{"));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_PROPERTY_NAME, AZ_SPAN_FROM_STR("a"));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_STRING, AZ_SPAN_FROM_STR("Hello world!"));
    TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_END_OBJECT, AZ_SPAN_FROM_STR("}"));
    assert_true(az_json_parser_next_token(&parser) == AZ_ERROR_JSON_PARSING_DONE);
    test_json_token_helper(parser.token, AZ_JSON_TOKEN_END_OBJECT, AZ_SPAN_FROM_STR("}"));
  }
  {
    uint8_t buffer[1000] = { 0 };
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
          "[[[[[ [[[[[");
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
          "[[[[[ [[[[");
      az_result const result = read_write(json, &output, &o);
      assert_int_equal(result, AZ_ERROR_JSON_PARSING_DONE);
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
      return write_str(*output, token.slice, output, written);
    }
    case AZ_JSON_TOKEN_BEGIN_OBJECT:
    {
      AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
      *output = az_span_copy_u8(*output, '{');
      *written += 1;
      bool need_comma = false;
      while (true)
      {
        az_result const result = az_json_parser_next_token(state);
        AZ_RETURN_IF_FAILED(result);
        if (state->token.kind != AZ_JSON_TOKEN_PROPERTY_NAME)
        {
          break;
        }
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
        AZ_RETURN_IF_FAILED(write_str(*output, state->token.slice, output, written));
        AZ_RETURN_IF_NOT_ENOUGH_SIZE(*output, 1);
        *output = az_span_copy_u8(*output, ':');
        *written += 1;

        AZ_RETURN_IF_FAILED(az_json_parser_next_token(state));
        AZ_RETURN_IF_FAILED(read_write_token(output, written, o, state, state->token));
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
        az_result const result = az_json_parser_next_token(state);
        AZ_RETURN_IF_FAILED(result);
        if (state->token.kind == AZ_JSON_TOKEN_END_ARRAY)
        {
          break;
        }
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
        AZ_RETURN_IF_FAILED(read_write_token(output, written, o, state, state->token));
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
  TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, input, NULL));
  AZ_RETURN_IF_FAILED(az_json_parser_next_token(&parser));
  int32_t written = 0;
  az_span output_copy = *output;
  AZ_RETURN_IF_FAILED(read_write_token(&output_copy, &written, o, &parser, parser.token));
  *output = az_span_slice(*output, 0, written);
  return AZ_OK;
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

// Using a macro instead of a helper method to retain line number
// in call stack to help debug which line/test case failed.
#define test_json_parser_invalid_helper(json, expected_result) \
  do \
  { \
    az_json_parser parser = { 0 }; \
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, json, NULL)); \
    az_result result = AZ_OK; \
    while (result == AZ_OK) \
    { \
      result = az_json_parser_next_token(&parser); \
    } \
    assert_int_equal(result, expected_result); \
  } while (0)

static void test_json_parser_invalid(void** state)
{
  (void)state;

  // Invalid nesting
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("{]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("{[]}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("{[,]}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[[{,}]]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("{{}}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[[{{}}]]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"age\":30,\"ints\":[1, 2, 3}}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("[[[[{\r\n\"a\":[[[[{\"b\":[}]]]]}]]]]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("[[[[{\r\n\"a\":[[[[{\"b\":[]},[}]]]]}]]]]"),
      AZ_ERROR_PARSER_UNEXPECTED_CHAR);

  // Invalid trailing commas
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR(","), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("   ,   "), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("{},"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[],"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("1,"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("true,"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("false,"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("null,"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("{,}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"a\": 1,,}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"a\": 1,,\"b\":2,}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[,]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[1,2,]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[1,,]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[1,,2,]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"a\":1,\"b\":[],}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);

  // Invalid literals and single tokens
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("nulz"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("truz"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("falsz"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("nul "), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("tru "), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("fals "), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("NULL"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("trUe"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("False"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("age"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\"age\":"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);

  // Invalid numbers
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("12345.1."), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("-f"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("1.f"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("0.1f"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("0.1e1f"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("123f"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("0-"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("1-"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("1.1-"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("123,"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("+0"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("+1"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("01"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("-01"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("0.e"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("-0.e"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("0.1e+,"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("-0.1e- "), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("0.1e+}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("-0.1e-]"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("1, 2"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("1, \"age\":"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("001"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("00h"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("[01"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("10.5e-f"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("10.5e-0.2"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"age\":30, \"ints\":[1, 2, 3, 4, 5.1e7.3]}"),
      AZ_ERROR_PARSER_UNEXPECTED_CHAR);

  // Invalid strings
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\"hel\rlo\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\"hel\nlo\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\"\\uABCX\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\"\\uXABC\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\uABCXlo\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(AZ_SPAN_FROM_STR("\"hel\\lo\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\\\\\lo\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\\tlo\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("\"hello\\\\\"\""), AZ_ERROR_PARSER_UNEXPECTED_CHAR);

  //  Invalid property names
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\rlo\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\nlo\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"\\uABCX\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"\\uXABC\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\\uABCXlo\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\lo\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\\\\\\lo\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\\\tlo\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hello\\\\\"\":1}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);

  test_json_parser_invalid_helper(
      AZ_SPAN_FROM_STR("{\r\n\"isActive\":false \"\r\n}"), AZ_ERROR_PARSER_UNEXPECTED_CHAR);
}

static void test_json_skip_children(void** state)
{
  (void)state;

  az_json_parser parser = { 0 };

  TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("{\"foo\":1}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_BEGIN_OBJECT);
  TEST_EXPECT_SUCCESS(az_json_parser_skip_children(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(parser._internal.bit_stack._internal.current_depth, 0);

  TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("{\"foo\":{}}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_BEGIN_OBJECT);
  TEST_EXPECT_SUCCESS(az_json_parser_skip_children(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(parser._internal.bit_stack._internal.current_depth, 0);

  TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("{\"foo\":{}}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_PROPERTY_NAME);
  TEST_EXPECT_SUCCESS(az_json_parser_skip_children(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(parser._internal.bit_stack._internal.current_depth, 1);

  TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("{\"foo\":{}}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  TEST_EXPECT_SUCCESS(az_json_parser_next_token(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_BEGIN_OBJECT);
  TEST_EXPECT_SUCCESS(az_json_parser_skip_children(&parser));
  assert_int_equal(parser.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(parser._internal.bit_stack._internal.current_depth, 1);
}

/** Json Value **/
static void test_json_value(void** state)
{
  (void)state;

  az_json_token const json_boolean = (az_json_token){
      .kind = AZ_JSON_TOKEN_TRUE,
      .slice = AZ_SPAN_FROM_STR("true"),
      ._internal = {
        .string_has_escaped_chars = false,
      },
    };
  az_json_token const json_number = (az_json_token){
      .kind = AZ_JSON_TOKEN_NUMBER,
      .slice = AZ_SPAN_FROM_STR("42"),
      ._internal = {
        .string_has_escaped_chars = false,
      },
    };
  az_json_token const json_string = (az_json_token){
      .kind = AZ_JSON_TOKEN_STRING,
      .slice = AZ_SPAN_FROM_STR("Hello"),
      ._internal = {
        .string_has_escaped_chars = false,
      },
    };
  az_json_token const json_property_name = (az_json_token){
      .kind = AZ_JSON_TOKEN_PROPERTY_NAME,
      .slice = AZ_SPAN_FROM_STR("Name"),
      ._internal = {
        .string_has_escaped_chars = false,
      },
    };

  // boolean from boolean
  {
    bool boolean_value = false;
    TEST_EXPECT_SUCCESS(az_json_token_get_boolean(&json_boolean, &boolean_value));
    assert_true(boolean_value);
  }
  // boolean from number
  {
    bool boolean_value = false;
    assert_true(
        az_json_token_get_boolean(&json_number, &boolean_value) == AZ_ERROR_JSON_INVALID_STATE);
  }

  // string from string
  {
    char string_value[10] = { 0 };
    TEST_EXPECT_SUCCESS(az_json_token_get_string(&json_string, string_value, 10, NULL));
    assert_true(
        az_span_is_content_equal(az_span_from_str(string_value), AZ_SPAN_FROM_STR("Hello")));

    TEST_EXPECT_SUCCESS(az_json_token_get_string(&json_property_name, string_value, 10, NULL));
    assert_true(az_span_is_content_equal(az_span_from_str(string_value), AZ_SPAN_FROM_STR("Name")));
  }
  // string from boolean
  {
    char string_value[10] = { 0 };
    assert_true(
        az_json_token_get_string(&json_boolean, string_value, 10, NULL)
        == AZ_ERROR_JSON_INVALID_STATE);
  }

  // number from number
  {
    uint64_t number_value = 1;
    TEST_EXPECT_SUCCESS(az_json_token_get_uint64(&json_number, &number_value));

    uint64_t const expected_value_bin_rep_view = 42;
    assert_true(number_value == expected_value_bin_rep_view);
  }
  // number from string
  {
    uint64_t number_value = 1;
    assert_true(
        az_json_token_get_uint64(&json_string, &number_value) == AZ_ERROR_JSON_INVALID_STATE);
  }
}

int test_az_json()
{
  const struct CMUnitTest tests[]
      = { cmocka_unit_test(test_json_parser_init),   cmocka_unit_test(test_json_builder),
          cmocka_unit_test(test_json_parser),        cmocka_unit_test(test_json_parser_invalid),
          cmocka_unit_test(test_json_skip_children), cmocka_unit_test(test_json_value) };
  return cmocka_run_group_tests_name("az_core_json", tests, NULL, NULL);
}

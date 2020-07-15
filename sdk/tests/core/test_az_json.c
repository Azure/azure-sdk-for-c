// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_json.h>
#include <azure/core/az_span.h>

#include <math.h>
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

static void test_json_reader_init(void** state)
{
  (void)state;

  az_json_reader_options options = az_json_reader_options_default();

  az_json_reader reader = { 0 };

  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("{}"), NULL), AZ_OK);
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("{}"), &options), AZ_OK);

  // Verify that initialization doesn't process any JSON text, even if it is invalid or incomplete.
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" "), NULL), AZ_OK);
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" "), &options), AZ_OK);
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("a"), NULL), AZ_OK);
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("a"), &options), AZ_OK);
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("\""), NULL), AZ_OK);
  assert_int_equal(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("\""), &options), AZ_OK);

  test_json_token_helper(reader.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
}

/**  Json writer **/
static void test_json_writer(void** state)
{
  (void)state;
  {
    uint8_t array[200] = { 0 };
    az_json_writer writer = { 0 };

    TEST_EXPECT_SUCCESS(az_json_writer_init(&writer, AZ_SPAN_FROM_BUFFER(array), NULL));

    // 0___________________________________________________________________________________________________1
    // 0_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0
    // 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
    // {"name":true,"foo":["bar",null,0,-12,12,9007199254740991],"int-max":2147483647,"esc":"_\"_\\_\b\f\n\r\t_","u":"a\u001Fb"}
    TEST_EXPECT_SUCCESS(az_json_writer_append_begin_object(&writer));

    TEST_EXPECT_SUCCESS(az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("name")));
    TEST_EXPECT_SUCCESS(az_json_writer_append_bool(&writer, true));

    {
      TEST_EXPECT_SUCCESS(az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("foo")));
      TEST_EXPECT_SUCCESS(az_json_writer_append_begin_array(&writer));
      az_result e = az_json_writer_append_string(&writer, AZ_SPAN_FROM_STR("bar"));
      TEST_EXPECT_SUCCESS(e);
      TEST_EXPECT_SUCCESS(az_json_writer_append_null(&writer));
      TEST_EXPECT_SUCCESS(az_json_writer_append_int32(&writer, 0));
      TEST_EXPECT_SUCCESS(az_json_writer_append_int32(&writer, -12));
      TEST_EXPECT_SUCCESS(az_json_writer_append_double(&writer, 12.1, 0));
      TEST_EXPECT_SUCCESS(az_json_writer_append_double(&writer, 9007199254740991ull, 0));
      TEST_EXPECT_SUCCESS(az_json_writer_append_end_array(&writer));
    }

    TEST_EXPECT_SUCCESS(
        az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("int-max")));
    TEST_EXPECT_SUCCESS(az_json_writer_append_int32(&writer, 2147483647));

    TEST_EXPECT_SUCCESS(az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("esc")));
    TEST_EXPECT_SUCCESS(
        az_json_writer_append_string(&writer, AZ_SPAN_FROM_STR("_\"_\\_\b\f\n\r\t_")));

    TEST_EXPECT_SUCCESS(az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("u")));
    TEST_EXPECT_SUCCESS(az_json_writer_append_string(
        &writer,
        AZ_SPAN_FROM_STR( //
            "a"
            "\x1f"
            "b")));

    TEST_EXPECT_SUCCESS(az_json_writer_append_end_object(&writer));

    az_span_to_str((char*)array, 200, az_json_writer_get_json(&writer));

    assert_string_equal(
        array,
        "{"
        "\"name\":true,"
        "\"foo\":[\"bar\",null,0,-12,12,9007199254740991],"
        "\"int-max\":2147483647,"
        "\"esc\":\"_\\\"_\\\\_\\b\\f\\n\\r\\t_\","
        "\"u\":\"a\\u001Fb\""
        "}");
  }
  {
    uint8_t array[33] = { 0 };
    az_json_writer writer = { 0 };
    {
      TEST_EXPECT_SUCCESS(az_json_writer_init(&writer, AZ_SPAN_FROM_BUFFER(array), NULL));

      TEST_EXPECT_SUCCESS(az_json_writer_append_double(&writer, 0.000000000000001, 15));

      az_span_to_str((char*)array, 33, az_json_writer_get_json(&writer));
      assert_string_equal(array, "0.000000000000001");
    }
    {
      TEST_EXPECT_SUCCESS(az_json_writer_init(&writer, AZ_SPAN_FROM_BUFFER(array), NULL));

      TEST_EXPECT_SUCCESS(az_json_writer_append_double(&writer, 1e-300, 15));

      az_span_to_str((char*)array, 33, az_json_writer_get_json(&writer));
      assert_string_equal(array, "0");
    }
  }
  {
    // json with AZ_JSON_TOKEN_STRING
    uint8_t array[200] = { 0 };
    az_json_writer writer = { 0 };
    TEST_EXPECT_SUCCESS(az_json_writer_init(&writer, AZ_SPAN_FROM_BUFFER(array), NULL));

    // this json { "span": "\" } would be scaped to { "span": "\\"" }
    uint8_t single_char[1] = { '\\' }; // char = '\'
    az_span single_span = AZ_SPAN_FROM_BUFFER(single_char);

    TEST_EXPECT_SUCCESS(az_json_writer_append_begin_object(&writer));

    TEST_EXPECT_SUCCESS(az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("span")));
    TEST_EXPECT_SUCCESS(az_json_writer_append_string(&writer, single_span));

    TEST_EXPECT_SUCCESS(az_json_writer_append_end_object(&writer));

    az_span expected = AZ_SPAN_FROM_STR("{"
                                        "\"span\":\"\\\\\""
                                        "}");

    assert_true(az_span_is_content_equal(az_json_writer_get_json(&writer), expected));
  }
  {
    // json with array and object inside
    uint8_t array[200] = { 0 };
    az_json_writer writer = { 0 };
    TEST_EXPECT_SUCCESS(az_json_writer_init(&writer, AZ_SPAN_FROM_BUFFER(array), NULL));

    // this json { "array": [1, 2, {}, 3, -12.3 ] }
    TEST_EXPECT_SUCCESS(az_json_writer_append_begin_object(&writer));

    TEST_EXPECT_SUCCESS(az_json_writer_append_property_name(&writer, AZ_SPAN_FROM_STR("array")));
    TEST_EXPECT_SUCCESS(az_json_writer_append_begin_array(&writer));

    TEST_EXPECT_SUCCESS(az_json_writer_append_int32(&writer, 1));
    TEST_EXPECT_SUCCESS(az_json_writer_append_int32(&writer, 2));

    TEST_EXPECT_SUCCESS(az_json_writer_append_begin_object(&writer));
    TEST_EXPECT_SUCCESS(az_json_writer_append_end_object(&writer));

    TEST_EXPECT_SUCCESS(az_json_writer_append_int32(&writer, 3));

    TEST_EXPECT_SUCCESS(az_json_writer_append_double(&writer, -1.234e1, 1));

    TEST_EXPECT_SUCCESS(az_json_writer_append_end_array(&writer));
    TEST_EXPECT_SUCCESS(az_json_writer_append_end_object(&writer));

    assert_true(az_span_is_content_equal(
        az_json_writer_get_json(&writer),
        AZ_SPAN_FROM_STR( //
            "{"
            "\"array\":[1,2,{},3,-12.3]"
            "}")));
  }
  {
    uint8_t nested_object_array[200] = { 0 };
    az_json_writer nested_object_writer = { 0 };
    {
      // 0___________________________________________________________________________________________________1
      // 0_________1_________2_________3_________4_________5_________6_________7_________8_________9_________0
      // 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
      // {"bar":true}
      TEST_EXPECT_SUCCESS(az_json_writer_init(
          &nested_object_writer, AZ_SPAN_FROM_BUFFER(nested_object_array), NULL));
      TEST_EXPECT_SUCCESS(az_json_writer_append_begin_object(&nested_object_writer));
      TEST_EXPECT_SUCCESS(
          az_json_writer_append_property_name(&nested_object_writer, AZ_SPAN_FROM_STR("bar")));
      TEST_EXPECT_SUCCESS(az_json_writer_append_bool(&nested_object_writer, true));
      TEST_EXPECT_SUCCESS(az_json_writer_append_end_object(&nested_object_writer));

      assert_true(az_span_is_content_equal(
          az_json_writer_get_json(&nested_object_writer),
          AZ_SPAN_FROM_STR( //
              "{"
              "\"bar\":true"
              "}")));
    }
  }
}

/** Json reader **/
az_result read_write(az_span input, az_span* output, int32_t* o);
az_result read_write_token(
    az_span* output,
    int32_t* written,
    int32_t* o,
    az_json_reader* state,
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

static bool _is_double_equal(double actual, double expected, double error)
{
  return fabs(actual - expected) < error;
}

static void test_json_reader(void** state)
{
  (void)state;
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("    "), NULL));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_EOF);
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("  null  "), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NULL, AZ_SPAN_FROM_STR("null"));
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("  nul"), NULL));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_EOF);
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("  false"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_FALSE, AZ_SPAN_FROM_STR("false"));
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("  falsx  "), NULL));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_UNEXPECTED_CHAR);
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("true "), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_TRUE, AZ_SPAN_FROM_STR("true"));
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("  truem"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_TRUE, AZ_SPAN_FROM_STR("true"));
  }
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("  123a"), NULL));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_UNEXPECTED_CHAR);
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NONE, AZ_SPAN_NULL);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR(" \"tr\\\"ue\\t\" ");
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, s, NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_STRING, AZ_SPAN_FROM_STR("tr\\\"ue\\t"));
    assert_true(az_span_ptr(reader.token.slice) == (az_span_ptr(s) + 2));
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0F\"");
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, s, NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_STRING, AZ_SPAN_FROM_STR("\\uFf0F"));
    assert_true(az_span_ptr(reader.token.slice) == az_span_ptr(s) + 1);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0\"");
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, s, NULL));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_UNEXPECTED_CHAR);
  }
  /* Testing reading number and converting to double */
  {
    // no exp number, no decimal, integer only
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" 23 "), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("23"));

    uint64_t actual_u64 = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_uint64(&reader.token, &actual_u64));
    assert_int_equal(actual_u64, 23);

    uint32_t actual_u32 = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_uint32(&reader.token, &actual_u32));
    assert_int_equal(actual_u32, 23);

    int64_t actual_i64 = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_int64(&reader.token, &actual_i64));
    assert_int_equal(actual_i64, 23);

    int32_t actual_i32 = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_int32(&reader.token, &actual_i32));
    assert_int_equal(actual_i32, 23);

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, 23, 1e-2));
  }
  {
    // no exp number, no decimal, negative integer only
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" -23 "), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("-23"));

    int64_t actual_i64 = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_int64(&reader.token, &actual_i64));
    assert_int_equal(actual_i64, -23);

    int32_t actual_i32 = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_int32(&reader.token, &actual_i32));
    assert_int_equal(actual_i32, -23);

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, -23, 1e-2));
  }
  {
    // negative number with decimals
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" -23.56"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("-23.56"));

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, -23.56, 1e-2));
  }
  {
    // negative + decimals + exp
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" -23.56e-3"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("-23.56e-3"));

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, -0.02356, 1e-5));
  }
  {
    // exp
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("1e50"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e50"));

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, 1e50, 1e-2));
  }
  {
    // big decimal + exp
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(
        az_json_reader_init(&reader, AZ_SPAN_FROM_STR("10000000000000000000000e17"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(
        reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("10000000000000000000000e17"));

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, 10000000000000000000000e17, 1e-2));
  }
  {
    // exp inf -> Any value above double MAX range would be translated to positive inf
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("1e309"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e309"));

    double actual_d = 0;

    // https://github.com/Azure/azure-sdk-for-c/issues/893
    // The result of this depends on the compiler.
#ifdef _MSC_VER
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(!isfinite(actual_d));

    // Create inf number with  IEEE 754 standard
    // floating point number containing all zeroes in the mantissa (first twenty-three bits), and
    // all ones in the exponent (next eight bits)
    unsigned int p = 0x7F800000; // 0xFF << 23
    float positiveInfinity = *(float*)&p;
    double const expected = positiveInfinity;
    uint64_t const* const expected_bin_rep_view = (uint64_t const*)&expected;
    uint64_t const* const token_value_number_bin_rep_view = (uint64_t*)&actual_d;
    assert_true(*token_value_number_bin_rep_view == *expected_bin_rep_view);
#else
    assert_int_equal(
        az_json_token_get_double(&reader.token, &actual_d), AZ_ERROR_UNEXPECTED_CHAR);
#endif // _MSC_VER
  }
  {
    // exp inf -> Any value below double MIN range would be translated 0
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("1e-400"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e-400"));

    double actual_d = 0;

    // https://github.com/Azure/azure-sdk-for-c/issues/893
    // The result of this depends on the compiler.
#ifdef _MSC_VER
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, 0.0, 1e-2));
#else
    assert_int_equal(
        az_json_token_get_double(&reader.token, &actual_d), AZ_ERROR_UNEXPECTED_CHAR);
#endif // _MSC_VER
  }
  {
    // negative exp
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("1e-18"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("1e-18"));

    double actual_d = 0;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&reader.token, &actual_d));
    assert_true(_is_double_equal(actual_d, 0.000000000000000001, 1e-17));
  }
  /* end of Testing reading number and converting to double */
  {
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR(" [ true, 0.25 ]"), NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_BEGIN_ARRAY, AZ_SPAN_FROM_STR("["));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_TRUE, AZ_SPAN_FROM_STR("true"));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_NUMBER, AZ_SPAN_FROM_STR("0.25"));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_END_ARRAY, AZ_SPAN_FROM_STR("]"));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_JSON_READER_DONE);
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_END_ARRAY, AZ_SPAN_FROM_STR("]"));
  }
  {
    az_span const json = AZ_SPAN_FROM_STR("{\"a\":\"Hello world!\"}");
    az_json_reader reader = { 0 };
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, json, NULL));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_BEGIN_OBJECT, AZ_SPAN_FROM_STR("{"));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_PROPERTY_NAME, AZ_SPAN_FROM_STR("a"));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_STRING, AZ_SPAN_FROM_STR("Hello world!"));
    TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_END_OBJECT, AZ_SPAN_FROM_STR("}"));
    assert_true(az_json_reader_next_token(&reader) == AZ_ERROR_JSON_READER_DONE);
    test_json_token_helper(reader.token, AZ_JSON_TOKEN_END_OBJECT, AZ_SPAN_FROM_STR("}"));
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
      assert_int_equal(result, AZ_ERROR_JSON_READER_DONE);
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
    az_json_reader* state,
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
        az_result const result = az_json_reader_next_token(state);
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

        AZ_RETURN_IF_FAILED(az_json_reader_next_token(state));
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
        az_result const result = az_json_reader_next_token(state);
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
  az_json_reader reader = { 0 };
  TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, input, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&reader));
  int32_t written = 0;
  az_span output_copy = *output;
  AZ_RETURN_IF_FAILED(read_write_token(&output_copy, &written, o, &reader, reader.token));
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
#define test_json_reader_invalid_helper(json, expected_result) \
  do \
  { \
    az_json_reader reader = { 0 }; \
    TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, json, NULL)); \
    az_result result = AZ_OK; \
    while (result == AZ_OK) \
    { \
      result = az_json_reader_next_token(&reader); \
    } \
    assert_int_equal(result, expected_result); \
  } while (0)

static void test_json_reader_invalid(void** state)
{
  (void)state;

  // Invalid nesting
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("{]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("{[]}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("{[,]}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[[{,}]]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("{{}}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[[{{}}]]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"age\":30,\"ints\":[1, 2, 3}}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("[[[[{\r\n\"a\":[[[[{\"b\":[}]]]]}]]]]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("[[[[{\r\n\"a\":[[[[{\"b\":[]},[}]]]]}]]]]"),
      AZ_ERROR_UNEXPECTED_CHAR);

  // Invalid trailing commas
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR(","), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("   ,   "), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("{},"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[],"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("1,"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("true,"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("false,"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("null,"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("{,}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"a\": 1,,}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"a\": 1,,\"b\":2,}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[,]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[1,2,]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[1,,]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[1,,2,]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"a\":1,\"b\":[],}"), AZ_ERROR_UNEXPECTED_CHAR);

  // Invalid literals and single tokens
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("nulz"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("truz"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("falsz"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("nul "), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("tru "), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("fals "), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("NULL"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("trUe"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("False"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("age"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\"age\":"), AZ_ERROR_UNEXPECTED_CHAR);

  // Invalid numbers
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("12345.1."), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("-f"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("1.f"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("0.1f"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("0.1e1f"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("123f"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("0-"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("1-"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("1.1-"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("123,"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("+0"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("+1"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("01"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("-01"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("0.e"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("-0.e"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("0.1e+,"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("-0.1e- "), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("0.1e+}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("-0.1e-]"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("1, 2"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("1, \"age\":"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("001"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("00h"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("[01"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("10.5e-f"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("10.5e-0.2"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"age\":30, \"ints\":[1, 2, 3, 4, 5.1e7.3]}"),
      AZ_ERROR_UNEXPECTED_CHAR);

  // Invalid strings
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\"hel\rlo\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\"hel\nlo\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\"\\uABCX\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\"\\uXABC\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\uABCXlo\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(AZ_SPAN_FROM_STR("\"hel\\lo\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\\\\\lo\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\\tlo\""), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("\"hello\\\\\"\""), AZ_ERROR_UNEXPECTED_CHAR);

  //  Invalid property names
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\rlo\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\nlo\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"\\uABCX\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"\\uXABC\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\\uABCXlo\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("\"hel\\lo\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\\\\\\lo\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hel\\\tlo\":1}"), AZ_ERROR_UNEXPECTED_CHAR);
  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\"hello\\\\\"\":1}"), AZ_ERROR_UNEXPECTED_CHAR);

  test_json_reader_invalid_helper(
      AZ_SPAN_FROM_STR("{\r\n\"isActive\":false \"\r\n}"), AZ_ERROR_UNEXPECTED_CHAR);
}

static void test_json_skip_children(void** state)
{
  (void)state;

  az_json_reader reader = { 0 };

  TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("{\"foo\":1}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_BEGIN_OBJECT);
  TEST_EXPECT_SUCCESS(az_json_reader_skip_children(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(reader._internal.bit_stack._internal.current_depth, 0);

  TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("{\"foo\":{}}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_BEGIN_OBJECT);
  TEST_EXPECT_SUCCESS(az_json_reader_skip_children(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(reader._internal.bit_stack._internal.current_depth, 0);

  TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("{\"foo\":{}}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_PROPERTY_NAME);
  TEST_EXPECT_SUCCESS(az_json_reader_skip_children(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(reader._internal.bit_stack._internal.current_depth, 1);

  TEST_EXPECT_SUCCESS(az_json_reader_init(&reader, AZ_SPAN_FROM_STR("{\"foo\":{}}"), NULL));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  TEST_EXPECT_SUCCESS(az_json_reader_next_token(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_BEGIN_OBJECT);
  TEST_EXPECT_SUCCESS(az_json_reader_skip_children(&reader));
  assert_int_equal(reader.token.kind, AZ_JSON_TOKEN_END_OBJECT);
  assert_int_equal(reader._internal.bit_stack._internal.current_depth, 1);
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
  az_json_token const json_negative_number = (az_json_token){
      .kind = AZ_JSON_TOKEN_NUMBER,
      .slice = AZ_SPAN_FROM_STR("-42"),
      ._internal = {
        .string_has_escaped_chars = false,
      },
    };
  az_json_token const json_decimal_number = (az_json_token){
      .kind = AZ_JSON_TOKEN_NUMBER,
      .slice = AZ_SPAN_FROM_STR("123.456"),
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
  // unsigned number from negative number
  {
    uint32_t number_value_u32 = 0;
    assert_int_equal(
        az_json_token_get_uint32(&json_negative_number, &number_value_u32),
        AZ_ERROR_UNEXPECTED_CHAR);
    uint64_t number_value_u64 = 0;
    assert_int_equal(
        az_json_token_get_uint64(&json_negative_number, &number_value_u64),
        AZ_ERROR_UNEXPECTED_CHAR);
  }
  // integer number from double number
  {
    uint32_t number_value_u32 = 0;
    assert_int_equal(
        az_json_token_get_uint32(&json_decimal_number, &number_value_u32),
        AZ_ERROR_UNEXPECTED_CHAR);
    uint64_t number_value_u64 = 0;
    assert_int_equal(
        az_json_token_get_uint64(&json_decimal_number, &number_value_u64),
        AZ_ERROR_UNEXPECTED_CHAR);
    int32_t number_value_i32 = 0;
    assert_int_equal(
        az_json_token_get_int32(&json_decimal_number, &number_value_i32),
        AZ_ERROR_UNEXPECTED_CHAR);
    int64_t number_value_i64 = 0;
    assert_int_equal(
        az_json_token_get_int64(&json_decimal_number, &number_value_i64),
        AZ_ERROR_UNEXPECTED_CHAR);
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
  // double number from decimal and negative number
  {
    double number_value = 1;
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&json_number, &number_value));
    assert_true(_is_double_equal(number_value, 42, 1e-2));
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&json_negative_number, &number_value));
    assert_true(_is_double_equal(number_value, -42, 1e-2));
    TEST_EXPECT_SUCCESS(az_json_token_get_double(&json_decimal_number, &number_value));
    assert_true(_is_double_equal(number_value, 123.456, 1e-2));

    int int32_number = -1;
    TEST_EXPECT_SUCCESS(az_json_token_get_int32(&json_negative_number, &int32_number));
    assert_int_equal(int32_number, -42);
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
      = { cmocka_unit_test(test_json_reader_init),   cmocka_unit_test(test_json_writer),
          cmocka_unit_test(test_json_reader),        cmocka_unit_test(test_json_reader_invalid),
          cmocka_unit_test(test_json_skip_children), cmocka_unit_test(test_json_value) };
  return cmocka_run_group_tests_name("az_core_json", tests, NULL, NULL);
}

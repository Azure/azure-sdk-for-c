// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_json.h>

#include <az_http_private.h>
#include <az_span.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <az_test.h>

#include <_az_cfg.h>

void test_http_response();
void test_json_builder();
void test_json_get_by_pointer();
void test_json_pointer();
void test_json_string();
void test_span_builder_replace();
void test_mut_span();
void test_log();

int exit_code = 0;

az_result write_str(az_span span, az_span s, az_span* out)
{
  *out = span;
  AZ_RETURN_IF_FAILED(az_span_append(*out, AZ_SPAN_FROM_STR("\""), out));
  AZ_RETURN_IF_FAILED(az_span_append(*out, s, out));
  AZ_RETURN_IF_FAILED(az_span_append(*out, AZ_SPAN_FROM_STR("\""), out));
  return AZ_OK;
}

az_result read_write_token(az_span* output, int32_t* o, az_json_parser* state, az_json_token token)
{
  switch (token.kind)
  {
    case AZ_JSON_TOKEN_NULL:
      return az_span_append(*output, AZ_SPAN_FROM_STR("null"), output);
    case AZ_JSON_TOKEN_BOOLEAN:
      return az_span_append(
          *output,
          token.value.boolean ? AZ_SPAN_FROM_STR("true") : AZ_SPAN_FROM_STR("false"),
          output);
    case AZ_JSON_TOKEN_NUMBER:
      return az_span_append(*output, AZ_SPAN_FROM_STR("0"), output);
    case AZ_JSON_TOKEN_STRING:
      return write_str(*output, token.value.string, output);
    case AZ_JSON_TOKEN_OBJECT:
    {
      AZ_RETURN_IF_FAILED(az_span_append(*output, AZ_SPAN_FROM_STR("{"), output));
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
          AZ_RETURN_IF_FAILED(az_span_append(*output, AZ_SPAN_FROM_STR(","), output));
        }
        else
        {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(write_str(*output, member.name, output));
        AZ_RETURN_IF_FAILED(az_span_append(*output, AZ_SPAN_FROM_STR(":"), output));
        AZ_RETURN_IF_FAILED(read_write_token(output, o, state, member.token));
      }
      return az_span_append(*output, AZ_SPAN_FROM_STR("}"), output);
    }
    case AZ_JSON_TOKEN_ARRAY:
    {
      AZ_RETURN_IF_FAILED(az_span_append(*output, AZ_SPAN_FROM_STR("["), output));
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
          AZ_RETURN_IF_FAILED(az_span_append(*output, AZ_SPAN_FROM_STR(","), output));
        }
        else
        {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(read_write_token(output, o, state, element));
      }
      return az_span_append(*output, AZ_SPAN_FROM_STR("]"), output);
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
  AZ_RETURN_IF_FAILED(read_write_token(output, o, &parser, token));
  return az_json_parser_done(&parser);
}

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

static az_span uri_encoded = AZ_SPAN_LITERAL_FROM_STR(
    "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%"
    "1F%20%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%"
    "40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~%7F%80%81%82%"
    "83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F%A0%A1%"
    "A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF%C0%"
    "C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%"
    "E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%"
    "FF");

static uint8_t uri_decoded_buf[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static az_span uri_decoded = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(uri_decoded_buf);

static az_span hrb_url
    = AZ_SPAN_LITERAL_FROM_STR("https://antk-keyvault.vault.azure.net/secrets/Password");

static az_span hrb_param_api_version_name = AZ_SPAN_LITERAL_FROM_STR("api-version");
static az_span hrb_param_api_version_token = AZ_SPAN_LITERAL_FROM_STR("7.0");

static az_span hrb_url2 = AZ_SPAN_LITERAL_FROM_STR(
    "https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0");

static az_span hrb_param_test_param_name = AZ_SPAN_LITERAL_FROM_STR("test-param");
static az_span hrb_param_test_param_token = AZ_SPAN_LITERAL_FROM_STR("token");

static az_span hrb_url3 = AZ_SPAN_LITERAL_FROM_STR(
    "https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0&test-param=token");

static az_span hrb_header_content_type_name = AZ_SPAN_LITERAL_FROM_STR("Content-Type");
static az_span hrb_header_content_type_token
    = AZ_SPAN_LITERAL_FROM_STR("application/x-www-form-urlencoded");

static az_span hrb_header_authorization_name = AZ_SPAN_LITERAL_FROM_STR("authorization");
static az_span hrb_header_authorization_token1 = AZ_SPAN_LITERAL_FROM_STR("Bearer 123456789");
static az_span hrb_header_authorization_token2
    = AZ_SPAN_LITERAL_FROM_STR("Bearer 99887766554433221100");

int main()
{
  test_json_get_by_pointer();
  test_json_pointer();
  test_json_builder();
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("    ")));
    TEST_ASSERT(az_json_parser_parse_token(&parser, NULL) == AZ_ERROR_ARG);
  }
  {
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(NULL, &token) == AZ_ERROR_ARG);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("    ")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&parser, &token) == AZ_ERROR_EOF);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  null  ")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&parser, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_NULL);
    TEST_ASSERT(az_json_parser_done(&parser) == AZ_OK);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR("  nul")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_ERROR_EOF);
  }
  {
    az_json_parser parser = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&parser, AZ_SPAN_FROM_STR("  false")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&parser, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
    TEST_ASSERT(token.value.boolean == false);
    TEST_ASSERT(az_json_parser_done(&parser) == AZ_OK);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR("  falsx  ")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR("true ")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
    TEST_ASSERT(token.value.boolean == true);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR("  truem")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR(" \"tr\\\"ue\\t\" ");
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, s));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_STRING);
    TEST_ASSERT(az_span_ptr(token.value.string) == (az_span_ptr(s) + 2));
    TEST_ASSERT(az_span_length(token.value.string) == 8);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0F\"");
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, s));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_STRING);
    TEST_ASSERT(az_span_ptr(token.value.string) == az_span_ptr(s) + 1);
    TEST_ASSERT(az_span_length(token.value.string) == 6);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_span const s = AZ_SPAN_FROM_STR("\"\\uFf0\"");
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, s));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR(" 23 ")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_NUMBER);
    TEST_ASSERT(token.value.number == 23);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR(" -23.56")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_NUMBER);
    TEST_ASSERT(token.value.number == -23.56);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR(" -23.56e-3")));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_NUMBER);
    TEST_ASSERT(token.value.number == -0.02356);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, AZ_SPAN_FROM_STR(" [ true, 0.3 ]")));
    az_json_token token = { 0 };
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_ARRAY);
    TEST_ASSERT(az_json_parser_parse_array_item(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_BOOLEAN);
    TEST_ASSERT(token.value.boolean == true);
    TEST_ASSERT(az_json_parser_parse_array_item(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_NUMBER);
    // TEST_ASSERT(token.value.number == 0.3);  TODO:  why do we get 0.30000004 ??
    TEST_ASSERT(az_json_parser_parse_array_item(&state, &token) == AZ_ERROR_ITEM_NOT_FOUND);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_span const json = AZ_SPAN_FROM_STR("{\"a\":\"Hello world!\"}");
    az_json_parser state = { 0 };
    TEST_EXPECT_SUCCESS(az_json_parser_init(&state, json));
    az_json_token token;
    TEST_ASSERT(az_json_parser_parse_token(&state, &token) == AZ_OK);
    TEST_ASSERT(token.kind == AZ_JSON_TOKEN_OBJECT);
    az_json_token_member token_member;
    TEST_ASSERT(az_json_parser_parse_token_member(&state, &token_member) == AZ_OK);
    TEST_ASSERT(az_span_ptr(token_member.name) == az_span_ptr(json) + 2);
    TEST_ASSERT(az_span_length(token_member.name) == 1);
    TEST_ASSERT(token_member.token.kind == AZ_JSON_TOKEN_STRING);
    TEST_ASSERT(az_span_ptr(token_member.token.value.string) == az_span_ptr(json) + 6);
    TEST_ASSERT(az_span_length(token_member.token.value.string) == 12);
    TEST_ASSERT(
        az_json_parser_parse_token_member(&state, &token_member) == AZ_ERROR_ITEM_NOT_FOUND);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    uint8_t buffer[1000];
    az_span output = AZ_SPAN_FROM_BUFFER(buffer);
    {
      int32_t o = 0;
      TEST_ASSERT(
          read_write(AZ_SPAN_FROM_STR("{ \"a\" : [ true, { \"b\": [{}]}, 15 ] }"), &output, &o)
          == AZ_OK);

      TEST_ASSERT(az_span_is_equal(output, AZ_SPAN_FROM_STR("{\"a\":[true,{\"b\":[{}]},0]}")));
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
      TEST_ASSERT(result == AZ_ERROR_JSON_NESTING_OVERFLOW);
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
      TEST_ASSERT(result == AZ_ERROR_EOF);
    }
    {
      int32_t o = 0;
      output = AZ_SPAN_FROM_BUFFER(buffer);
      az_span const json = AZ_SPAN_FROM_STR(
          // 0           1           2           3           4           5 6 01234
          // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
          "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
          "[[[[[ [[{"
          "   \"\\t\\n\": \"\\u0abc\"   "
          "}]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] "
          "]]]]] ]]]");
      output._internal.length = 0;
      output = AZ_SPAN_FROM_BUFFER(buffer);
      az_result const result = read_write(json, &output, &o);
      TEST_ASSERT(result == AZ_OK);

      TEST_ASSERT(az_span_is_equal(
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
      TEST_ASSERT(result == AZ_OK);
    }

    {
      uint8_t buf[256 * 3];
      az_span builder = AZ_SPAN_FROM_BUFFER(buf);

      TEST_EXPECT_SUCCESS(
          az_span_copy_url_encode(builder, AZ_SPAN_FROM_STR("https://vault.azure.net"), &builder));
      TEST_ASSERT(az_span_is_equal(builder, AZ_SPAN_FROM_STR("https%3A%2F%2Fvault.azure.net")));

      builder = AZ_SPAN_FROM_BUFFER(buffer);
      TEST_EXPECT_SUCCESS(az_span_copy_url_encode(builder, uri_decoded, &builder));
      TEST_ASSERT(az_span_is_equal(builder, uri_encoded));
    }
    {
      uint8_t buf[100];
      uint8_t header_buf[(2 * sizeof(az_pair))];
      memset(buf, 0, sizeof(buf));
      memset(header_buf, 0, sizeof(header_buf));

      az_span url_span = AZ_SPAN_FROM_BUFFER(buf);
      TEST_EXPECT_SUCCESS(az_span_append(url_span, hrb_url, &url_span));
      az_span header_span = AZ_SPAN_FROM_BUFFER(header_buf);
      _az_http_request hrb;

      TEST_EXPECT_SUCCESS(
          az_http_request_init(&hrb, az_http_method_get(), url_span, header_span, az_span_null()));
      TEST_ASSERT(az_span_is_equal(hrb._internal.method, az_http_method_get()));
      TEST_ASSERT(az_span_is_equal(hrb._internal.url, url_span));
      TEST_ASSERT(az_span_capacity(hrb._internal.url) == 100);
      TEST_ASSERT(hrb._internal.max_headers == 2);
      TEST_ASSERT(hrb._internal.retry_headers_start_byte_offset == 0);

      TEST_EXPECT_SUCCESS(az_http_request_set_query_parameter(
          &hrb, hrb_param_api_version_name, hrb_param_api_version_token));
      TEST_ASSERT(az_span_is_equal(hrb._internal.url, hrb_url2));

      TEST_EXPECT_SUCCESS(az_http_request_set_query_parameter(
          &hrb, hrb_param_test_param_name, hrb_param_test_param_token));
      TEST_ASSERT(az_span_is_equal(hrb._internal.url, hrb_url3));

      TEST_EXPECT_SUCCESS(az_http_request_append_header(
          &hrb, hrb_header_content_type_name, hrb_header_content_type_token));

      TEST_ASSERT(hrb._internal.retry_headers_start_byte_offset == 0);

      TEST_EXPECT_SUCCESS(_az_http_request_mark_retry_headers_start(&hrb));
      TEST_ASSERT(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

      TEST_EXPECT_SUCCESS(az_http_request_append_header(
          &hrb, hrb_header_authorization_name, hrb_header_authorization_token1));

      TEST_ASSERT(az_span_length(hrb._internal.headers) / sizeof(az_pair) == 2);
      TEST_ASSERT(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

      az_pair expected_headers1[2] = {
        { .key = hrb_header_content_type_name, .value = hrb_header_content_type_token },
        { .key = hrb_header_authorization_name, .value = hrb_header_authorization_token1 },
      };
      for (uint16_t i = 0; i < az_span_length(hrb._internal.headers) / sizeof(az_pair); ++i)
      {
        az_pair header = { 0 };
        TEST_EXPECT_SUCCESS(az_http_request_get_header(&hrb, i, &header));

        TEST_ASSERT(az_span_is_equal(header.key, expected_headers1[i].key));
        TEST_ASSERT(az_span_is_equal(header.value, expected_headers1[i].value));
      }

      TEST_EXPECT_SUCCESS(_az_http_request_remove_retry_headers(&hrb));
      TEST_ASSERT(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

      TEST_EXPECT_SUCCESS(az_http_request_append_header(
          &hrb, hrb_header_authorization_name, hrb_header_authorization_token2));
      TEST_ASSERT(az_span_length(hrb._internal.headers) / sizeof(az_pair) == 2);
      TEST_ASSERT(hrb._internal.retry_headers_start_byte_offset == sizeof(az_pair));

      az_pair expected_headers2[2] = {
        { .key = hrb_header_content_type_name, .value = hrb_header_content_type_token },
        { .key = hrb_header_authorization_name, .value = hrb_header_authorization_token2 },
      };
      for (uint16_t i = 0; i < az_span_length(hrb._internal.headers) / sizeof(az_pair); ++i)
      {
        az_pair header = { 0 };
        TEST_EXPECT_SUCCESS(az_http_request_get_header(&hrb, i, &header));

        TEST_ASSERT(az_span_is_equal(header.key, expected_headers2[i].key));
        TEST_ASSERT(az_span_is_equal(header.value, expected_headers2[i].value));
      }
    }
  }

  test_http_response();
  test_span_builder_replace();
  test_mut_span();

  test_json_string();
  test_log();
  return exit_code;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_base64.h>
#include <az_http_request.h>
#include <az_json_read.h>
#include <az_span_reader.h>
#include <az_write_span_iter.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

int exit_code = 0;

#define TEST_ASSERT(c) \
  do { \
    if (c) { \
      printf("  - `%s`: succeeded\n", #c); \
    } else { \
      fprintf(stderr, "  - `%s`: failed\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while (false);

az_result write(az_span const output, size_t * const o, az_const_span const s) {
  for (size_t i = 0; i != s.size; ++i, ++*o) {
    if (*o == output.size) {
      return 1;
    }
    output.begin[*o] = s.begin[i];
  }
  return 0;
}

az_result write_str(az_span const output, size_t * o, az_const_span const s) {
  AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("\"")));
  AZ_RETURN_IF_FAILED(write(output, o, s));
  AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("\"")));
  return AZ_OK;
}

az_result read_write_value(
    az_span const output,
    size_t * o,
    az_json_state * const state,
    az_json_value const value) {
  switch (value.kind) {
    case AZ_JSON_VALUE_NULL:
      return write(output, o, AZ_STR("null"));
    case AZ_JSON_VALUE_BOOLEAN:
      return write(output, o, value.data.boolean ? AZ_STR("true") : AZ_STR("false"));
    case AZ_JSON_VALUE_NUMBER:
      return write(output, o, AZ_STR("0"));
    case AZ_JSON_VALUE_STRING:
      return write_str(output, o, value.data.string);
    case AZ_JSON_VALUE_OBJECT: {
      AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("{")));
      bool need_comma = false;
      while (true) {
        az_json_member member;
        az_result const result = az_json_read_object_member(state, &member);
        if (result == AZ_JSON_ERROR_NO_MORE_ITEMS) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        if (need_comma) {
          AZ_RETURN_IF_FAILED(write(output, o, AZ_STR(",")));
        } else {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(write_str(output, o, member.name));
        AZ_RETURN_IF_FAILED(write(output, o, AZ_STR(":")));
        AZ_RETURN_IF_FAILED(read_write_value(output, o, state, member.value));
      }
      return write(output, o, AZ_STR("}"));
    }
    case AZ_JSON_VALUE_ARRAY: {
      AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("[")));
      bool need_comma = false;
      while (true) {
        az_json_value element;
        az_result const result = az_json_read_array_element(state, &element);
        if (result == AZ_JSON_ERROR_NO_MORE_ITEMS) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        if (need_comma) {
          AZ_RETURN_IF_FAILED(write(output, o, AZ_STR(",")));
        } else {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(read_write_value(output, o, state, element));
      }
      return write(output, o, AZ_STR("]"));
    }
    default:
      break;
  }
  return AZ_JSON_ERROR_INVALID_STATE;
}

az_result read_write(az_const_span const input, az_span const output, size_t * const o) {
  az_json_state state = az_json_state_create(input);
  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_read(&state, &value));
  AZ_RETURN_IF_FAILED(read_write_value(output, o, &state, value));
  return az_json_state_done(&state);
}

static az_const_span const sample1 = AZ_CONST_STR( //
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

static az_const_span const b64_decoded0 = AZ_CONST_STR("");
static az_const_span const b64_decoded1 = AZ_CONST_STR("1");
static az_const_span const b64_decoded2 = AZ_CONST_STR("12");
static az_const_span const b64_decoded3 = AZ_CONST_STR("123");
static az_const_span const b64_decoded4 = AZ_CONST_STR("1234");
static az_const_span const b64_decoded5 = AZ_CONST_STR("12345");
static az_const_span const b64_decoded6 = AZ_CONST_STR("123456");

static az_const_span const b64_encoded0 = AZ_CONST_STR("");
static az_const_span const b64_encoded1 = AZ_CONST_STR("MQ==");
static az_const_span const b64_encoded2 = AZ_CONST_STR("MTI=");
static az_const_span const b64_encoded3 = AZ_CONST_STR("MTIz");
static az_const_span const b64_encoded4 = AZ_CONST_STR("MTIzNA==");
static az_const_span const b64_encoded5 = AZ_CONST_STR("MTIzNDU=");
static az_const_span const b64_encoded6 = AZ_CONST_STR("MTIzNDU2");

static az_const_span const b64_encoded0u = AZ_CONST_STR("");
static az_const_span const b64_encoded1u = AZ_CONST_STR("MQ");
static az_const_span const b64_encoded2u = AZ_CONST_STR("MTI");
static az_const_span const b64_encoded3u = AZ_CONST_STR("MTIz");
static az_const_span const b64_encoded4u = AZ_CONST_STR("MTIzNA");
static az_const_span const b64_encoded5u = AZ_CONST_STR("MTIzNDU");
static az_const_span const b64_encoded6u = AZ_CONST_STR("MTIzNDU2");

static az_const_span const b64_encoded_bin1
    = AZ_CONST_STR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

static az_const_span const b64_encoded_bin1u
    = AZ_CONST_STR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");

static uint8_t const b64_decoded_bin1_buf[]
    = { 0x00, 0x10, 0x83, 0x10, 0x51, 0x87, 0x20, 0x92, 0x8B, 0x30, 0xD3, 0x8F,
        0x41, 0x14, 0x93, 0x51, 0x55, 0x97, 0x61, 0x96, 0x9B, 0x71, 0xD7, 0x9F,
        0x82, 0x18, 0xA3, 0x92, 0x59, 0xA7, 0xA2, 0x9A, 0xAB, 0xB2, 0xDB, 0xAF,
        0xC3, 0x1C, 0xB3, 0xD3, 0x5D, 0xB7, 0xE3, 0x9E, 0xBB, 0xF3, 0xDF, 0xBF };

static az_const_span const b64_decoded_bin1
    = { .begin = b64_decoded_bin1_buf, .size = sizeof(b64_decoded_bin1_buf) };

static az_const_span const b64_encoded_bin2
    = AZ_CONST_STR("/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+zQ==");

static az_const_span const b64_encoded_bin2u
    = AZ_CONST_STR("_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-zQ");

static uint8_t const b64_decoded_bin2_buf[]
    = { 0xFC, 0x00, 0x42, 0x0C, 0x41, 0x46, 0x1C, 0x82, 0x4A, 0x2C, 0xC3, 0x4E, 0x3D,
        0x04, 0x52, 0x4D, 0x45, 0x56, 0x5D, 0x86, 0x5A, 0x6D, 0xC7, 0x5E, 0x7E, 0x08,
        0x62, 0x8E, 0x49, 0x66, 0x9E, 0x8A, 0x6A, 0xAE, 0xCB, 0x6E, 0xBF, 0x0C, 0x72,
        0xCF, 0x4D, 0x76, 0xDF, 0x8E, 0x7A, 0xEF, 0xCF, 0x7E, 0xCD };

static az_const_span const b64_decoded_bin2
    = { .begin = b64_decoded_bin2_buf, .size = sizeof(b64_decoded_bin2_buf) };

static az_const_span const b64_encoded_bin3
    = AZ_CONST_STR("V/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+zQ=");

static az_const_span const b64_encoded_bin3u
    = AZ_CONST_STR("V_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-zQ");

static uint8_t const b64_decoded_bin3_buf[]
    = { 0x57, 0xF0, 0x01, 0x08, 0x31, 0x05, 0x18, 0x72, 0x09, 0x28, 0xB3, 0x0D, 0x38,
        0xF4, 0x11, 0x49, 0x35, 0x15, 0x59, 0x76, 0x19, 0x69, 0xB7, 0x1D, 0x79, 0xF8,
        0x21, 0x8A, 0x39, 0x25, 0x9A, 0x7A, 0x29, 0xAA, 0xBB, 0x2D, 0xBA, 0xFC, 0x31,
        0xCB, 0x3D, 0x35, 0xDB, 0x7E, 0x39, 0xEB, 0xBF, 0x3D, 0xFB, 0x34 };

static az_const_span const b64_decoded_bin3
    = { .begin = b64_decoded_bin3_buf, .size = sizeof(b64_decoded_bin3_buf) };

int main() {
  {
    az_json_state state = az_json_state_create(AZ_STR("    "));
    TEST_ASSERT(az_json_read(&state, NULL) == AZ_ERROR_ARG);
  }
  {
    az_json_value value;
    TEST_ASSERT(az_json_read(NULL, &value) == AZ_ERROR_ARG);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("    "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_JSON_ERROR_UNEXPECTED_END);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("  null  "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NULL);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("  nul"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_JSON_ERROR_UNEXPECTED_END);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("  false"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == false);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("  falsx  "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_JSON_ERROR_UNEXPECTED_CHAR);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("true "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == true);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR("  truem"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_JSON_ERROR_UNEXPECTED_CHAR);
  }
  {
    az_const_span const s = AZ_STR(" \"tr\\\"ue\\t\" ");
    az_json_state state = az_json_state_create(s);
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(value.data.string.begin == s.begin + 2);
    TEST_ASSERT(value.data.string.size == 8);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_const_span const s = AZ_STR("\"\\uFf0F\"");
    az_json_state state = az_json_state_create(s);
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(value.data.string.begin == s.begin + 1);
    TEST_ASSERT(value.data.string.size == 6);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_const_span const s = AZ_STR("\"\\uFf0\"");
    az_json_state state = az_json_state_create(s);
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_JSON_ERROR_UNEXPECTED_CHAR);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR(" 23 "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == 23);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR(" -23.56"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == -23.56);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR(" -23.56e-3"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == -0.02356);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_STR(" [ true, 0.3 ]"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_ARRAY);
    TEST_ASSERT(az_json_read_array_element(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == true);
    TEST_ASSERT(az_json_read_array_element(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    // TEST_ASSERT(value.val.number == 0.3);
    TEST_ASSERT(az_json_read_array_element(&state, &value) == AZ_JSON_ERROR_NO_MORE_ITEMS);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_const_span const json = AZ_STR("{\"a\":\"Hello world!\"}");
    az_json_state state = az_json_state_create(json);
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_OBJECT);
    az_json_member member;
    TEST_ASSERT(az_json_read_object_member(&state, &member) == AZ_OK);
    TEST_ASSERT(member.name.begin == json.begin + 2);
    TEST_ASSERT(member.name.size == 1);
    TEST_ASSERT(member.value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(member.value.data.string.begin == json.begin + 6);
    TEST_ASSERT(member.value.data.string.size == 12);
    TEST_ASSERT(az_json_read_object_member(&state, &member) == AZ_JSON_ERROR_NO_MORE_ITEMS);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  uint8_t buffer[1000];
  az_span const output = { .begin = buffer, .size = 1000 };
  {
    size_t o = 0;
    TEST_ASSERT(
        read_write(AZ_STR("{ \"a\" : [ true, { \"b\": [{}]}, 15 ] }"), output, &o) == AZ_OK);
    az_const_span x = az_const_span_sub(az_span_to_const_span(output), 0, o);
    TEST_ASSERT(az_const_span_eq(x, AZ_STR("{\"a\":[true,{\"b\":[{}]},0]}")));
  }
  {
    size_t o = 0;
    az_const_span const json = AZ_STR(
        // 0           1           2           3           4           5 6
        // 01234 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234
        // 56789 0123
        "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
        "[[[[[ [[[[");
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_JSON_ERROR_STACK_OVERFLOW);
  }
  {
    size_t o = 0;
    az_const_span const json = AZ_STR(
        // 0           1           2           3           4           5 6 01234
        // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
        "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
        "[[[[[ [[[");
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_JSON_ERROR_UNEXPECTED_END);
  }
  {
    size_t o = 0;
    az_const_span const json = AZ_STR(
        // 0           1           2           3           4           5 6 01234
        // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
        "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
        "[[[[[ [[{"
        "   \"\\t\\n\": \"\\u0abc\"   "
        "}]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] "
        "]]]]] ]]]");
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_OK);
    az_const_span x = az_const_span_sub(az_span_to_const_span(output), 0, o);
    TEST_ASSERT(az_const_span_eq(
        x,
        AZ_STR( //
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[{"
            "\"\\t\\n\":\"\\u0abc\""
            "}]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]")));
  }
  //
  {
    size_t o = 0;
    az_result const result = read_write(sample1, output, &o);
    TEST_ASSERT(result == AZ_OK);
  }

  // HTTP Builder
  {
    az_pair const query_array[] = {
      { .key = AZ_STR("hello"), .value = AZ_STR("world!") },
      { .key = AZ_STR("x"), .value = AZ_STR("42") },
    };
    az_pair_span const query = AZ_SPAN(query_array);
    //
    az_pair const headers_array[] = {
      { .key = AZ_STR("some"), .value = AZ_STR("xml") },
      { .key = AZ_STR("xyz"), .value = AZ_STR("very_long") },
    };
    az_pair_span const headers = AZ_SPAN(headers_array);
    //
    az_http_request const request = {
      .method = AZ_STR("GET"),
      .path = AZ_STR("/foo"),
      .query = az_pair_span_to_seq(&query),
      .headers = az_pair_span_to_seq(&headers),
      .body = AZ_STR("{ \"somejson\": true }"),
    };
    uint8_t buffer[1024];
    {
      az_write_span_iter wi = az_write_span_iter_create((az_span)AZ_SPAN(buffer));
      az_span_visitor sv = az_write_span_iter_to_span_visitor(&wi);
      az_const_span const expected = AZ_STR( //
          "GET /foo?hello=world!&x=42 HTTP/1.1\r\n"
          "some: xml\r\n"
          "xyz: very_long\r\n"
          "\r\n"
          "{ \"somejson\": true }");
      az_result const result = az_http_request_to_spans(&request, sv);
      TEST_ASSERT(result == AZ_OK);
      az_span out = az_write_span_iter_result(&wi);
      TEST_ASSERT(az_const_span_eq(az_span_to_const_span(out), expected));
    }
    // HTTP Builder with policies.
    {
      az_write_span_iter wi = az_write_span_iter_create((az_span)AZ_SPAN(buffer));
      az_span_visitor sv = az_write_span_iter_to_span_visitor(&wi);
      az_const_span const expected = AZ_STR( //
          "GET /foo?hello=world!&x=42 HTTP/1.1\r\n"
          "ContentType: text/plain; charset=utf-8\r\n"
          "some: xml\r\n"
          "xyz: very_long\r\n"
          "\r\n"
          "{ \"somejson\": true }");
      az_http_request new_request = request;
      az_http_standard_policy s;
      {
        az_result const result = az_http_standard_policy_create(&new_request, &s);
        TEST_ASSERT(result == AZ_OK);
      }
      {
        az_result const result = az_http_request_to_spans(&new_request, sv);
        TEST_ASSERT(result == AZ_OK);
      }
      az_span out = az_write_span_iter_result(&wi);
      TEST_ASSERT(az_const_span_eq(az_span_to_const_span(out), expected));
    }
  }
  {
    az_const_span const expected = AZ_STR("@###copy#copy#make some zero-terminated strings#make "
                                          "some\0zero-terminated\0strings\0####@");

    uint8_t buf[87];
    assert(expected.size == sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); ++i) {
      buf[i] = '@';
    }

    az_span actual = { .begin = buf, .size = sizeof(buf) };
    az_span_set((az_span){ .begin = actual.begin + 1, .size = actual.size - 2 }, '#');

    az_span result;

    char const phrase1[] = "copy";
    memcpy(actual.begin + 4, phrase1, sizeof(phrase1) - 1);
    az_span_copy(
        (az_span){ .begin = actual.begin + 9, .size = 4 },
        (az_const_span){ .begin = actual.begin + 4, .size = 4 },
        &result);

    char const phrase2[] = "make some zero-terminated strings";
    memcpy(actual.begin + 14, phrase2, sizeof(phrase2) - 1);

    az_const_span const make_some = (az_const_span){ .begin = actual.begin + 14, .size = 9 };
    az_const_span const zero_terminated = (az_const_span){ .begin = actual.begin + 24, .size = 15 };
    az_const_span const strings = (az_const_span){ .begin = actual.begin + 40, .size = 7 };

    az_span_to_c_str((az_span){ .begin = actual.begin + 48, .size = 10 }, make_some, &result);
    az_span_to_c_str((az_span){ .begin = actual.begin + 58, .size = 16 }, zero_terminated, &result);
    az_span_to_c_str((az_span){ .begin = actual.begin + 74, .size = 8 }, strings, &result);

    result.begin[result.size - 1] = '$';
    az_span_to_c_str(result, strings, &result);

    TEST_ASSERT(az_const_span_eq(az_span_to_const_span(actual), expected));
  }
  {
    uint8_t buf[68];
    az_span const buffer = { .begin = buf, .size = sizeof(buf) };
    az_const_span result;

    az_const_span const * const decoded_input[]
        = { &b64_decoded0, &b64_decoded1, &b64_decoded2,     &b64_decoded3,     &b64_decoded4,
            &b64_decoded5, &b64_decoded6, &b64_decoded_bin1, &b64_decoded_bin2, &b64_decoded_bin3 };

    az_const_span const * const encoded_input[]
        = { &b64_encoded0, &b64_encoded1, &b64_encoded2,     &b64_encoded3,     &b64_encoded4,
            &b64_encoded5, &b64_encoded6, &b64_encoded_bin1, &b64_encoded_bin2, &b64_encoded_bin3 };

    az_const_span const * const url_encoded_input[]
        = { &b64_encoded0u, &b64_encoded1u, &b64_encoded2u,     &b64_encoded3u,     &b64_encoded4u,
            &b64_encoded5u, &b64_encoded6u, &b64_encoded_bin1u, &b64_encoded_bin2u, &b64_encoded_bin3u };

    for (size_t i = 0; i < 10; ++i) {
      az_base64_encode(false, buffer, *decoded_input[i], &result);
      TEST_ASSERT(az_const_span_eq(result, *encoded_input[i]));

      az_base64_decode(buffer, *encoded_input[i], &result);
      TEST_ASSERT(az_const_span_eq(result, *decoded_input[i]));

      az_base64_encode(true, buffer, *decoded_input[i], &result);
      TEST_ASSERT(az_const_span_eq(result, *url_encoded_input[i]));

      az_base64_decode(buffer, *url_encoded_input[i], &result);
      TEST_ASSERT(az_const_span_eq(result, *decoded_input[i]));
    }
  }
  return exit_code;
}

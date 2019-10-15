// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_request.h>
#include <az_json_read.h>
#include <az_span_reader.h>

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
    //
    az_pair const headers_array[] = {
      { .key = AZ_STR("some"), .value = AZ_STR("xml") },
      { .key = AZ_STR("xyz"), .value = AZ_STR("very_long") },
    };
    //
    az_http_request const request = {
      .method = AZ_STR("GET"),
      .path = AZ_STR("/foo"),
      .query = az_pair_span_to_iter((az_pair_span)AZ_SPAN(query_array)),
      .headers = az_pair_span_to_iter((az_pair_span)AZ_SPAN(headers_array)),
      .body = AZ_STR("{ \"somejson\": true }"),
    };
    uint8_t buffer[1024];
    {
      az_span out;
      az_const_span const expected = AZ_STR( //
          "GET /foo?hello=world!&x=42 HTTP/1.1\r\n"
          "some: xml\r\n"
          "xyz: very_long\r\n"
          "\r\n"
          "{ \"somejson\": true }");
      az_result const result = az_http_request_to_buffer(&request, (az_span)AZ_SPAN(buffer), &out);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(az_const_span_eq(az_span_to_const_span(out), expected));
    }
    // HTTP Builder with policies.
    {
      az_span out;
      az_const_span const expected = AZ_STR( //
          "GET /foo?hello=world!&x=42 HTTP/1.1\r\n"
          "ContentType: text/plain; charset=utf-8\r\n"
          "some: xml\r\n"
          "xyz: very_long\r\n"
          "\r\n"
          "{ \"somejson\": true }");
      az_http_request new_request = request;
      az_http_standard_headers_data s;
      { 
        az_result const result = az_http_standard_headers_policy(&new_request, &s);
        TEST_ASSERT(result == AZ_OK);
      }
      {
        az_result const result
            = az_http_request_to_buffer(&new_request, (az_span)AZ_SPAN(buffer), &out);
        TEST_ASSERT(result == AZ_OK);
      }
      TEST_ASSERT(az_const_span_eq(az_span_to_const_span(out), expected));
    }
  }
  return exit_code;
}

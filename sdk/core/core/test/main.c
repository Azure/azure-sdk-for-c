// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_read.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int exit_code = 0;

#define TEST_ASSERT(c) \
  do { \
    if(c) { \
      printf("  - `%s`: succeeded\n", #c); \
    } else { \
      fprintf(stderr, "  - `%s`: failed\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while(false);

az_result write(az_str const output, size_t *const o, az_const_str const s) {
  for (size_t i = 0; i != s.size; ++i, ++*o) {
    if (*o == output.size) {
      return 1;
    }
    output.begin[*o] = s.begin[i];
  }
  return 0;
}

az_result write_str(az_str const output, size_t* o, az_json_string const s) {
  AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR("\"")));
  AZ_RETURN_ON_ERROR(write(output, o, s));
  AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR("\"")));
  return AZ_OK;
}

az_result read_write_value(az_str const output, size_t *o, az_json_state *const state, az_json_value const value) {
  switch (value.tag) {
    case AZ_JSON_NULL:
      return write(output, o, AZ_CONST_STR("null"));
    case AZ_JSON_BOOLEAN:
      return write(output, o, value.boolean ? AZ_CONST_STR("true") : AZ_CONST_STR("false"));
    case AZ_JSON_NUMBER:
      return write(output, o, AZ_CONST_STR("0"));
    case AZ_JSON_STRING:
      return write_str(output, o, value.string);
    case AZ_JSON_OBJECT:
    {
      AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR("{")));
      bool need_comma = false;
      while (true) {
        az_json_member member;
        az_result const result = az_json_read_object_member(state, &member);
        if (result == AZ_JSON_NO_MORE_ITEMS) {
          break;
        }
        if (result != AZ_OK) {
          return result;
        }
        if (need_comma) {
          AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR(",")));
        }
        else {
          need_comma = true;
        }
        AZ_RETURN_ON_ERROR(write_str(output, o, member.name));
        AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR(":")));
        AZ_RETURN_ON_ERROR(read_write_value(output, o, state, member.value));
      }
      return write(output, o, AZ_CONST_STR("}"));
    }
    case AZ_JSON_ARRAY:
    {
      AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR("[")));
      bool need_comma = false;
      while (true) {
        az_json_value element;
        az_result const result = az_json_read_array_element(state, &element);
        if (result == AZ_JSON_NO_MORE_ITEMS) {
          break;
        }
        if (result != AZ_OK) {
          return result;
        }
        if (need_comma) {
          AZ_RETURN_ON_ERROR(write(output, o, AZ_CONST_STR(",")));
        }
        else {
          need_comma = true;
        }
        AZ_RETURN_ON_ERROR(read_write_value(output, o, state, element));
      }
      return write(output, o, AZ_CONST_STR("]"));
    }
  }
}

az_result read_write(az_const_str const input, az_str const output, size_t *const o) {
  az_json_state state = az_json_state_create(input);
  az_json_value value;
  AZ_RETURN_ON_ERROR(az_json_read(&state, &value));
  AZ_RETURN_ON_ERROR(read_write_value(output, o, &state, value));
  return az_json_state_done(&state);
}

int main() {
  {
	az_json_state state = az_json_state_create(AZ_CONST_STR("    "));
	az_json_value value;
	TEST_ASSERT(az_json_read(&state, &value) == AZ_JSON_ERROR_UNEXPECTED_END);
  }
  {
    az_json_state state = az_json_state_create(AZ_CONST_STR("  null  "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NULL);
	TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
	  az_json_state state = az_json_state_create(AZ_CONST_STR("  false"));
	  az_json_value value;
	  TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
	  TEST_ASSERT(value.tag == AZ_JSON_BOOLEAN);
	  TEST_ASSERT(value.boolean == false);
	  TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
	  az_json_state state = az_json_state_create(AZ_CONST_STR("true "));
	  az_json_value value;
	  TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
	  TEST_ASSERT(value.tag == AZ_JSON_BOOLEAN);
	  TEST_ASSERT(value.boolean == true);
	  TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_const_str const s = AZ_CONST_STR(" \"tr\\\"ue\" ");
	  az_json_state state = az_json_state_create(s);
	  az_json_value value;
	  TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
	  TEST_ASSERT(value.tag == AZ_JSON_STRING);
    TEST_ASSERT(value.string.begin == s.begin + 2);
    TEST_ASSERT(value.string.size == 6);
	  TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_CONST_STR(" 23 "));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NUMBER);
    TEST_ASSERT(value.number == 23);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_CONST_STR(" -23.56"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NUMBER);
    TEST_ASSERT(value.number == -23.56);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_CONST_STR(" -23.56e-3"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NUMBER);
    TEST_ASSERT(value.number == -0.02356);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_json_state state = az_json_state_create(AZ_CONST_STR(" [ true, 0.3 ]"));
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_ARRAY);
    TEST_ASSERT(az_json_read_array_element(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_BOOLEAN);
    TEST_ASSERT(value.boolean == true);
    TEST_ASSERT(az_json_read_array_element(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_NUMBER);
    //TEST_ASSERT(value.number == 0.3);
    TEST_ASSERT(az_json_read_array_element(&state, &value) == AZ_JSON_NO_MORE_ITEMS);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  {
    az_const_str const json = AZ_CONST_STR("{\"a\":\"Hello world!\"}");
    az_json_state state = az_json_state_create(json);
    az_json_value value;
    TEST_ASSERT(az_json_read(&state, &value) == AZ_OK);
    TEST_ASSERT(value.tag == AZ_JSON_OBJECT);
    az_json_member member;
    TEST_ASSERT(az_json_read_object_member(&state, &member) == AZ_OK);
    TEST_ASSERT(member.name.begin == json.begin + 2);
    TEST_ASSERT(member.name.size == 1);
    TEST_ASSERT(member.value.tag == AZ_JSON_STRING);
    TEST_ASSERT(member.value.string.begin == json.begin + 6);
    TEST_ASSERT(member.value.string.size == 12);
    TEST_ASSERT(az_json_read_object_member(&state, &member) == AZ_JSON_NO_MORE_ITEMS);
    TEST_ASSERT(az_json_state_done(&state) == AZ_OK);
  }
  char buffer[1000];
  az_str output = { .begin = buffer, .size = 1000 };
  {
    size_t o = 0;
    TEST_ASSERT(read_write(AZ_CONST_STR("{ \"a\" : [ true, { \"b\": [{}]}, 15 ] }"), output, &o) == AZ_OK);
    az_const_str x = az_const_substr(az_to_const_str(output), 0, o);
  }
  {
    size_t o = 0;
    az_const_str const json = AZ_CONST_STR(
    // 0           1           2           3           4           5           6
    // 01234 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 0123
      "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[["
    );
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_JSON_ERROR_STACK_OVERFLOW);
  }
  {
    size_t o = 0;
    az_const_str const json = AZ_CONST_STR(
    // 0           1           2           3           4           5           6
    // 01234 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
      "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[["
    );
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_JSON_ERROR_UNEXPECTED_END);
  }
  {
    size_t o = 0;
    az_const_str const json = AZ_CONST_STR(
    // 0           1           2           3           4           5           6
    // 01234 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
      "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[{"
      "}]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]"
    );
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_OK);
    az_const_str x = az_const_substr(az_to_const_str(output), 0, o);
    TEST_ASSERT(az_const_str_eq(x, AZ_CONST_STR(
      "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[{"
      "}]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]")));
  }
  return exit_code;
}

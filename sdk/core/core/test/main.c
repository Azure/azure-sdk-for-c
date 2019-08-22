#include <az_json_state.h>

#include <stdio.h>

/*
int result = 0;

#define ASSERT(c) \
  do { \
    if(c) { printf("- `%s`: succeeded\n", #c); } else { fprintf(stderr, "- `%s`: failed\n", #c); result = 1; } \
  } while(false);

az_error json_parse(az_cstr s) {
  size_t i = 0;
  az_json_value value;
  return az_json_parse_value(s, &i, &value);
}

AZ_CSTR(json, "");
AZ_CSTR(json_ws, "   \r \n \t ");
AZ_CSTR(json_null, "null");
AZ_CSTR(json_nulx, "nulx");
AZ_CSTR(json_false, "false");
AZ_CSTR(json_true, "true");
AZ_CSTR(json_null_space, "null  ");
AZ_CSTR(json_string, "\"Hello world!\"");
AZ_CSTR(json_invalid_string, "\"Hello world\n\"");
AZ_CSTR(json_no_end_string, "\"Hello");

int main() {
  ASSERT(json_parse(json) == AZ_JSON_ERROR_UNEXPECTED_END);
  ASSERT(json_parse(json_ws) == AZ_JSON_ERROR_UNEXPECTED_END);
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_null, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_NULL);
  }
  ASSERT(json_parse(json_nulx) == AZ_JSON_ERROR_UNEXPECTED_SYMBOL);
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_false, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_FALSE);
  }
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_true, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_TRUE);
  }
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_null_space, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_NULL);
  }
  {
    size_t i = 0;
    az_json_value value;
    const e = az_json_parse_value(json_string, &i, &value);
    ASSERT(e == AZ_OK);
    ASSERT(value.type == AZ_JSON_VALUE_STRING);
    ASSERT(value.string.begin == 1);
    // "Hello world!"
    // 01234567890123
    ASSERT(value.string.end == 13);
  }
  ASSERT(json_parse(json_invalid_string) == AZ_JSON_ERROR_UNEXPECTED_SYMBOL);
  ASSERT(json_parse(json_no_end_string) == AZ_JSON_ERROR_UNEXPECTED_END);
  return result;
}
*/

int main() {}

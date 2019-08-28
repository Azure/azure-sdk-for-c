// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_keyword.h>

#include <az_cstr.h>

az_json_keyword az_json_keyword_initial(az_json_keyword_type type) {
  return az_json_keyword_create_char((az_json_keyword_char){ .type = type, .size = 0 });
}

az_json_keyword az_json_keyword_none_parse(char const c) {
  switch (c) {
    case 'n':
      return az_json_keyword_initial(AZ_JSON_KEYWORD_NULL);
    case 'f':
      return az_json_keyword_initial(AZ_JSON_KEYWORD_FALSE);
    case 't':
      return az_json_keyword_initial(AZ_JSON_KEYWORD_TRUE);
  }
  return az_json_keyword_create_none();
}

az_json_keyword az_json_keyword_check(az_json_keyword_char const state, char c, az_cstr value) {
  int8_t const i = state.size + 1;
  if (value.size <= i) {
    return az_json_keyword_create_done((az_json_keyword_done){ .type = state.type, .next = c });
  }
  if (value.begin == c) {
    return az_json_keyword_create_char((az_json_keyword_char){ .type = state.type, .size = i });
  }
  return az_json_keyword_create_error();
}

az_json_keyword az_json_keyword_char_parse(az_json_keyword_char const state, char c) {
  switch (state.type) {
    case AZ_JSON_KEYWORD_NULL:
      return az_json_keyword_check(state, c, AZ_CSTR("null"));
    case AZ_JSON_KEYWORD_FALSE:
      return az_json_keyword_check(state, c, AZ_CSTR("false"));
    case AZ_JSON_KEYWORD_TRUE:
      return az_json_keyword_check(state, c, AZ_CSTR("true"));
  }
  return az_json_keyword_create_error();
}

az_json_keyword az_json_keyword_parse(az_json_keyword const state, char const c) {
  switch (state.tag) {
    case AZ_JSON_KEYWORD_NONE:
      return az_json_keyword_none_parse(c);
    case AZ_JSON_KEYWORD_CHAR:
      return az_json_keyword_char_parse(state.char_, c);
  }
  return az_json_keyword_create_error();
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "../inc/az_json.h"

AZ_DEFINE_STRING(null_str, "null");

AZ_DEFINE_STRING(true_str, "true");

AZ_DEFINE_STRING(false_str, "false");

static inline az_error write_string(az_json_write_string const f, az_string const s) {
  return s.begin != s.end ? f.write(f.context, s) : AZ_OK;
}

static inline az_error write_char(az_json_write_string const f, char const c) {
  az_string const s = { .begin = &c, .end = &c + 1 };
  return write_string(f, s);
}

static inline az_error write_json_number(az_json_write_string const f, double const number) {
  // TODO: implement :-)
  return write_char(f, '0');
}

#define RETURN_ON_ERROR(E) { az_error const result = (E); if (result != AZ_OK) { return result; } }

az_string const empty = { .begin = NULL, .end = NULL };

AZ_DEFINE_STRING(escape_quotation_mark, "\\\"");

AZ_DEFINE_STRING(escape_reverse_solidus, "\\\\");

AZ_DEFINE_STRING(escape_backspace, "\\b");

AZ_DEFINE_STRING(escape_formfeed, "\\f");

AZ_DEFINE_STRING(escape_horizontal_tab, "\\t");

AZ_DEFINE_STRING(escape_linefeed, "\\n");

AZ_DEFINE_STRING(escape_carriage_return, "\\r");

static inline az_string escape(char const c) {
  switch (c) {
    case '"':
      return escape_quotation_mark;
    case '\\':
      return escape_reverse_solidus;
    case '\b':
      return escape_backspace;
    case '\f':
      return escape_formfeed;
    case '\t':
      return escape_horizontal_tab;
    case '\n':
      return escape_linefeed;
    case '\r':
      return escape_carriage_return;
  }
  return empty;
}

static az_error write_json_string(az_json_write_string const f, az_string s) {
  RETURN_ON_ERROR(write_char(f, '"'));
  char const *last = s.begin;
  for (; s.begin != s.end; ++s.begin) {
    char const c = *s.begin;
    az_string const e = escape(c);
    if (s.begin != s.end) {
      const az_string previous = { .begin = last, .end = s.begin };
      RETURN_ON_ERROR(write_string(f, previous));
      RETURN_ON_ERROR(write_string(f, e));
      last = s.begin + 1;
    }
  }
  {
    const az_string previous = { .begin = last, .end = s.begin };
    RETURN_ON_ERROR(write_string(f, previous));
  }
  RETURN_ON_ERROR(write_char(f, '"'));
  return AZ_OK;
}

static inline az_error write_json_property(az_json_write_string const f, az_json_property const p) {
  RETURN_ON_ERROR(write_json_string(f, p.name));
  RETURN_ON_ERROR(write_char(f, ':'));
  RETURN_ON_ERROR(az_json_write(f, p.value));
  return AZ_OK;
}

static inline az_error write_json_object(az_json_write_string const f, az_json_object a) {
  RETURN_ON_ERROR(write_char(f, '{'));
  if (a.begin != a.end) {
    ++a.begin;
    RETURN_ON_ERROR(write_json_property(f, *a.begin));
    for (; a.begin != a.end; ++a.begin) {
      RETURN_ON_ERROR(write_char(f, ','));
      RETURN_ON_ERROR(write_json_property(f, *a.begin));
    }
  }
  RETURN_ON_ERROR(write_char(f, '}'));
  return AZ_OK;
}

static inline az_error write_json_array(az_json_write_string const f, az_json_array a) {
  RETURN_ON_ERROR(write_char(f, '['));
  if (a.begin != a.end) {
    RETURN_ON_ERROR(az_json_write(f, *a.begin));
    for (; a.begin != a.end; ++a.begin) {
      RETURN_ON_ERROR(write_char(f, ','));
      RETURN_ON_ERROR(az_json_write(f, *a.begin));
    }
  }
  RETURN_ON_ERROR(write_char(f, ']'));
  return AZ_OK;
}

static inline az_error write_json_null(az_json_write_string const f) {
  return write_string(f, null_str);
}

static inline az_error write_json_boolean(az_json_write_string const f, bool const b) {
  return write_string(f, b ? true_str : false_str);
}

az_error az_json_write(az_json_write_string const f, az_json const json) {
  switch (json.type) {
    case AZ_JSON_TYPE_BOOLEAN:
      return write_json_boolean(f, json.boolean);
    case AZ_JSON_TYPE_NUMBER:
      return write_json_number(f, json.number);
    case AZ_JSON_TYPE_STRING:
      return write_json_string(f, json.string);
    case AZ_JSON_TYPE_OBJECT:
      return write_json_object(f, json.object);
    case AZ_JSON_TYPE_ARRAY:
      return write_json_array(f, json.array);
    case AZ_JSON_TYPE_NULL:
    default:
      return write_json_null(f);
  }
}

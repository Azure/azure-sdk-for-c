// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "../inc/az_json.h"

az_string const null_string = AZ_STRING("null");

az_string const true_string = AZ_STRING("true");

az_string const false_string = AZ_STRING("false");

static inline az_error write_string(az_json_write_string const f, az_string const s) {
  return s.size > 0 ? f.write(f.context, s) : AZ_OK;
}

static inline az_error write_char(az_json_write_string const f, char const c) {
  az_string const s = { .begin = &c, .size = 1 };
  return write_string(f, s);
}

static inline az_error write_json_number(az_json_write_string const f, double const numer) {
  return write_char(f, '0');
}

#define RETURN_ON_ERROR(E) { az_error const result = (E); if (result != AZ_OK) { return result; } }

az_string const empty = { .begin = NULL, .size = 0 };

az_string const escape_quotation_mark = AZ_STRING("\\\"");

az_string const escape_reverse_solidus = AZ_STRING("\\\\");

az_string const escape_backspace = AZ_STRING("\\b");

az_string const escape_formfeed = AZ_STRING("\\f");

az_string const escape_horizontal_tab = AZ_STRING("\\t");

az_string  const escape_linefeed = AZ_STRING("\\n");

az_string const escape_carriage_return = AZ_STRING("\\r");

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

static inline az_error write_sub_string(
  az_json_write_string const f,
  az_string const s,
  ptrdiff_t begin,
  ptrdiff_t end
) {
  if (begin < end) {
    const az_string sub = AZ_SUB_RANGE(s, begin, end);
    return write_string(f, sub);
  }
  return AZ_OK;
}

static az_error write_json_string(az_json_write_string const f, az_string const s) {
  RETURN_ON_ERROR(write_char(f, '"'));
  size_t last = 0;
  for (size_t i = 0; i < s.size; ++i) {
    char const c = s.begin[i];
    az_string const e = escape(c);
    if (e.size > 0) {
      const az_string previous = AZ_SUB_RANGE(s, last, i);
      RETURN_ON_ERROR(write_string(f, previous));
      RETURN_ON_ERROR(write_string(f, e));
      last = i + 1;
    }
  }
  {
    const az_string previous = AZ_SUB_RANGE(s, last, s.size);
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

static inline az_error write_json_object(az_json_write_string const f, az_json_object const a) {
  RETURN_ON_ERROR(write_char(f, '{'));
  if (a.size > 0) {
    RETURN_ON_ERROR(write_json_property(f, a.begin[0]));
    for (size_t i = 1; i < a.size; ++i) {
      RETURN_ON_ERROR(write_char(f, ','));
      RETURN_ON_ERROR(write_json_property(f, a.begin[i]));
    }
  }
  RETURN_ON_ERROR(write_char(f, '}'));
  return AZ_OK;
}

static inline az_error write_json_array(az_json_write_string const f, az_json_array const a) {
  RETURN_ON_ERROR(write_char(f, '['));
  if (a.size > 0) {
    RETURN_ON_ERROR(az_json_write(f, a.begin[0]));
    for (size_t i = 1; i < a.size; ++i) {
      RETURN_ON_ERROR(write_char(f, ','));
      RETURN_ON_ERROR(az_json_write(f, a.begin[i]));
    }
  }
  RETURN_ON_ERROR(write_char(f, ']'));
  return AZ_OK;
}

static inline az_error write_json_null(az_json_write_string const f) {
  return write_string(f, null_string);
}

static inline az_error write_json_boolean(az_json_write_string const f, bool const b) {
  return write_string(f, b ? true_string : false_string);
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
      return write_json_null(f);
  }
}

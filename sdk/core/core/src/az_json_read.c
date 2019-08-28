#include <az_json_read.h>

inline bool az_is_whitespace(char const c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline az_error az_json_result(az_error const error, size_t *const p_index, size_t i) {
  *p_index = i;
  return error;
}

inline az_error az_json_value_ok(
  size_t *const p_index,
  size_t const i,
  az_json_value *const out_value,
  az_json_value const value
) {
  *out_value = value;
  return az_json_result(AZ_OK, p_index, i);
}

az_error az_json_read_whitespace(az_cstr const buffer, size_t *const p_index) {
  size_t i = *p_index;
  for (; i < buffer.size; ++i) {
    if (!az_is_whitespace(az_cstr_item(buffer, i))) {
      return az_json_result(p_index, i, AZ_OK);
    }
  }
  return az_json_result(p_index, i, AZ_JSON_ERROR_UNEXPECTED_END);
}

az_error az_json_read_value(
  az_cstr const buffer,
  size_t *const p_index,
  az_json_value *const out_value,
  az_cstr const keyword,
  az_json_value const value
) {
  size_t i = *p_index + 1;
  for (size_t keyword_i = 0; keyword_i < keyword.size; ++keyword_i, ++i) {
    if (buffer.size <= i) {
      return az_json_result(p_index, i, AZ_JSON_ERROR_UNEXPECTED_END);
    }
    if (az_cstr_item(keyword, keyword_i) != az_cstr_item(buffer, i)) {
      return az_json_result(p_index, i, AZ_JSON_ERROR_UNEXPECTED_CHAR);
    }
  }
  return az_json_value_ok(p_index, i, out_value, value);
}

az_error az_json_read_string(az_cstr const buffer, size_t *const p_index, az_json_value *const out_value) {
  size_t const begin = *p_index + 1;
  size_t i = begin;
  for (; i < buffer.size; ++i) {
    char const c = az_cstr_item(buffer, i);
    switch (c) {
      case '"':
        return az_json_value_ok(
          p_index,
          i,
          out_value,
          az_json_value_create_string((az_json_string){ .begin = begin, .end = i - 1 })
        );
      case '\\':
        {
          ++i;
          if (buffer.size <= i) {
            return az_json_result(p_index, i, AZ_JSON_ERROR_UNEXPECTED_END);
          }
          char const esc = az_cstr_item(buffer, i);
          switch (c) {
            case '\t':
          }
        }
        break;
    }
  }
  return az_json_result(p_index, i, AZ_JSON_ERROR_UNEXPECTED_END);
}

az_error az_json_read_value(az_cstr const buffer, size_t *const p_index, az_json_value *const out_value) {
  AZ_RETURN_ON_ERROR(az_json_read_whitespace(buffer, p_index));
  char const c = az_cstr_item(buffer, *p_index);
  switch (c) {
    case 'n':
      return az_json_read_keyword(
        buffer,
        p_index,
        out_value,
        AZ_CSTR("ull"),
        az_json_value_create_null()
      );
    case 'f':
      return az_json_read_keyword(
        buffer,
        p_index,
        out_value,
        AZ_CSTR("alse"),
        az_json_value_create_boolean(false)
      );
    case 't':
      return az_json_read_keyword(
        buffer,
        p_index,
        out_value,
        AZ_CSTR("rue"),
        az_json_value_create_boolean(true)
      );
    case '"':
      return az_json_read_string(buffer, p_index, out_value);
    case '-': return AZ_OK;
    case '0': return AZ_OK;
    case '{': return AZ_OK;
    case '[': return AZ_OK;
  }
  if (az_is_digit(c)) {
    return AZ_OK;
  }
  return AZ_JSON_ERROR_UNEXPECTED_CHAR;
}

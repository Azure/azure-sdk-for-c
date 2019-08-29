#include <az_json_read.h>

#include <az_json_state.h>

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

az_error az_json_read_value(az_cstr const buffer, size_t *const p_index, az_json_value *const out_value) {
  az_json_state state = AZ_JSON_STATE_NONE;
  size_t string_open;
  size_t i = 0;
  for (; i < buffer.size; ++i) {
    char const c = az_cstr_item(buffer, i);
    state = az_json_state_value_parse(state, c);
    switch (state) {
      case AZ_JSON_STATE_ERROR:
        *p_index = i;
        return AZ_JSON_ERROR_UNEXPECTED_CHAR;
      case AZ_JSON_STATE_TRUE:
        *out_value = az_json_value_create_boolean(true);
        *p_index = i;
        return AZ_OK;
      case AZ_JSON_STATE_FALSE:
        *out_value = az_json_value_create_boolean(false);
        *p_index = i;
        return AZ_OK;
      case AZ_JSON_STATE_NULL:
        *out_value = az_json_value_create_null();
        *p_index = i;
        return AZ_OK;
      case AZ_JSON_STATE_STRING_OPEN:
        string_open = i;
        break;
      case AZ_JSON_STATE_STRING:
        *out_value = az_json_value_create_string((az_json_string){ .begin = string_open, .end = i });
        *p_index = i;
        return AZ_OK;
      case AZ_JSON_STATE_NUMBER:
        *out_value = az_json_value_create_number(0);
        *p_index = i - 1;
        return AZ_OK;
      case AZ_JSON_STATE_OBJECT_EMPTY:
        *out_value = az_json_value_create_object(true);
        *p_index = i;
        return AZ_OK;
      case AZ_JSON_STATE_OBJECT_PROPERTY:
        *out_value = az_json_value_create_object(false);
        *p_index = i - 1;
        return AZ_OK;
      case AZ_JSON_STATE_ARRAY_EMPTY:
        *out_value = az_json_value_create_array(true);
        *p_index = i;
        return AZ_OK;
      case AZ_JSON_STATE_ARRAY_ITEM:
        *out_value = az_json_value_create_array(false);
        *p_index = i - 1;
        return AZ_OK;
    }
  }
  *p_index = i;
  return AZ_JSON_ERROR_UNEXPECTED_END;
}

az_error az_json_read_property(az_cstr const buffer, size_t *const p_position, az_json_property *const out_property) {
  az_json_value name;
  AZ_RETURN_ON_ERROR(az_json_read_value(buffer, p_position, &name));
  if (name.tag != AZ_JSON_STRING) {
    return AZ_JSON_ERROR_UNEXPECTED_CHAR;
  }
  out_property->name = name.string;
  // read until ':'
  AZ_RETURN_ON_ERROR(az_json_read_value(buffer, p_position, &out_property->value));
  return AZ_OK;
}

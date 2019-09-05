// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_read.h>

/*
inline bool az_json_stack_is_empty(az_json_state const *const p_state) {
  return p_state->stack == 1;
}

inline az_json_stack_item az_json_stack_last(az_json_state const *const p_state) {
  return p_state->stack & 1;
}

az_result az_json_state_init(az_json_state *const out_state, az_const_str const buffer) {
  *out_state = (az_json_state){
    .buffer = buffer,
    .no_more_items = false,
    .stack = 1,
  };
}

az_result az_json_stack_last_check(az_json_state *const p_state, az_json_stack_item const expected) {
  return !az_json_stack_is_empty(p_state) && az_json_stack_last(p_state) == expected
    ? AZ_OK
    : AZ_JSON_ERROR_INVALID_STATE;
}

void az_json_read_white_space(az_json_state *const p_state) {
  size_t size = p_state->buffer.size;
  size_t i = 0;
  for (; i < size; ++i) {
    switch (az_const_str_item(p_state->buffer, i)) {
      case ' ': case '\t': case '\n': case '\r':
        continue;
    }
    break;
  }
  p_state->buffer = az_const_str_from(p_state->buffer, i);
}

az_result az_json_state_no_more_items_check() {

}

az_result az_json_read_value(az_json_state *const p_state, az_json_value *const out_value) {
  if (!az_json_stack_is_empty(p_state) || p_state->no_more_items) {
    return AZ_JSON_ERROR_INVALID_STATE;
  }
  az_json_read_white_space(p_state);

}

az_result az_json_read_object_member(az_json_state *const p_state, az_json_member *const out_member) {
  AZ_RETURN_ON_ERROR(az_json_stack_last_check(p_state, AZ_JSON_STACK_OBJECT));
  if (p_state->no_more_items) {
    // p_state->stack >>= 1;
    // az_json_read_white_space(p_state);

    // TODO: move to the parent item.
    return AZ_JSON_NO_MORE_ITEMS;
  }
}

az_result az_json_read_array_element(az_json_state *const p_state, az_json_value *const out_value) {
  AZ_RETURN_ON_ERROR(az_json_stack_last_check(p_state, AZ_JSON_STACK_ARRAY));
  if (p_state->no_more_items) {
    // TODO: move to the parent item.
    return AZ_JSON_NO_MORE_ITEMS;
  }
}

az_result az_json_state_done(az_json_state const *const p_state) {
  return p_state->buffer.size == 0 && az_json_stack_is_empty(p_state) && p_state->no_more_items == true
    ? AZ_OK
    : AZ_JSON_ERROR_INVALID_STATE;
}
*/
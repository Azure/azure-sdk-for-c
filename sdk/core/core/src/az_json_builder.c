// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include "az_span_private.h"
#include <az_hex_internal.h>
#include <az_json_builder.h>
#include <az_json_string.h>
#include <az_span_action.h>
#include <az_span_reader.h>
#include "az_str_private.h"

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_json_builder_init(az_json_builder * const out, az_span_action const write) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  *out = (az_json_builder){ .write = write };

  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_builder_write_str(
    az_json_builder * const self,
    az_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_action const write = self->write;
  AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_SPAN_FROM_STR("\"")));
  AZ_RETURN_IF_FAILED(az_span_action_do(write, value));
  AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_SPAN_FROM_STR("\"")));
  return AZ_OK;
}

typedef struct {
  az_span_action write;
  az_span value;
  size_t begin;
  size_t i;
} az_json_value_span_state;

AZ_NODISCARD az_result az_json_value_span_state_flush(az_json_value_span_state * const state) {
  AZ_CONTRACT_ARG_NOT_NULL(state);

  az_span span = { 0 };
  AZ_RETURN_IF_FAILED(az_span_slice(state->value, state->begin, state->i, &span));
  if (!az_span_is_empty(span)) {
    return az_span_action_do(state->write, span);
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_value_span_state_replace(
    az_json_value_span_state * const state,
    az_span const replacement) {
  AZ_RETURN_IF_FAILED(az_json_value_span_state_flush(state));
  AZ_RETURN_IF_FAILED(az_span_action_do(state->write, replacement));
  size_t const new_i = state->i + 1;
  state->i = new_i;
  state->begin = new_i;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_write_span(az_json_builder * const self, az_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_json_value_span_state state = {
    .write = self->write,
    .value = value,
    .begin = 0,
    .i = 0,
  };
  AZ_RETURN_IF_FAILED(az_span_action_do(state.write, AZ_SPAN_FROM_STR("\"")));
  while (true) {
    az_result_byte const c = az_span_get(state.value, state.i);
    if (c == AZ_ERROR_EOF) {
      break;
    }
    // check if the character has to be escaped.
    {
      az_span const esc = az_json_esc_encode(c);
      if (!az_span_is_empty(esc)) {
        AZ_RETURN_IF_FAILED(az_json_value_span_state_replace(&state, esc));
        continue;
      }
    }
    // check if the character has to be escaped as a UNICODE escape sequence.
    if (0 <= c && c < 0x20) {
      uint8_t array[6] = {
        '\\',
        'u',
        '0',
        '0',
        az_number_to_upper_hex((uint8_t)(c / 16)),
        az_number_to_upper_hex((uint8_t)(c % 16)),
      };
      AZ_RETURN_IF_FAILED(
          az_json_value_span_state_replace(&state, AZ_SPAN_FROM_INITIALIZED_BUFFER(array)));
      continue;
    }
    // no need to escape, process to the next character.
    state.i += 1;
  }
  AZ_RETURN_IF_FAILED(az_json_value_span_state_flush(&state));
  AZ_RETURN_IF_FAILED(az_span_action_do(state.write, AZ_SPAN_FROM_STR("\"")));
  return AZ_OK;
}

// 2^53 - 1
#define AZ_JSON_UINT_MAX 0x1FFFFFFFFFFFFFull

AZ_NODISCARD static az_result az_json_builder_write_double(
    az_json_builder * const self,
    double value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_action const write = self->write;

  if (value == 0) {
    return az_span_action_do(write, AZ_SPAN_FROM_STR("0"));
  }

  if (value < 0) {
    AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_SPAN_FROM_STR("-")));
    value = -value;
  }

  if (value > (double)AZ_JSON_UINT_MAX) {
    // D.*De+*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  {
    uint64_t u = (uint64_t)value;
    if (value == (double)u) {
      uint64_t base = 1;
      {
        uint64_t i = u;
        while (10 <= i) {
          i /= 10;
          base *= 10;
        }
      }
      do {
        uint8_t dec = (uint8_t)(u / base) + '0';
        u %= base;
        base /= 10;
        AZ_RETURN_IF_FAILED(az_span_action_do(write, az_span_from_single_item(&dec)));
      } while (1 <= base);
      return AZ_OK;
    }
  }

  // eg. 0.0012
  if (value < 0.001) {
    // D.*De-*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  // eg. 1.2e-4
  {
    // *D.*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

AZ_NODISCARD az_result
az_json_builder_write(az_json_builder * const self, az_json_token const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_action const write = self->write;
  switch (value.kind) {
    case AZ_JSON_TOKEN_NULL: {
      self->need_comma = true;
      return az_span_action_do(write, AZ_SPAN_FROM_STR("null"));
    }
    case AZ_JSON_TOKEN_BOOLEAN: {
      self->need_comma = true;
      return az_span_action_do(
          write, value.data.boolean ? AZ_SPAN_FROM_STR("true") : AZ_SPAN_FROM_STR("false"));
    }
    case AZ_JSON_TOKEN_STRING: {
      self->need_comma = true;
      return az_json_builder_write_str(self, value.data.string);
    }
    case AZ_JSON_TOKEN_NUMBER: {
      self->need_comma = true;
      return az_json_builder_write_double(self, value.data.number);
    }
    case AZ_JSON_TOKEN_OBJECT: {
      self->need_comma = false;
      return az_span_action_do(write, AZ_SPAN_FROM_STR("{"));
    }
    case AZ_JSON_TOKEN_ARRAY: {
      self->need_comma = false;
      return az_span_action_do(write, AZ_SPAN_FROM_STR("["));
    }
    case AZ_JSON_TOKEN_SPAN: {
      self->need_comma = true;
      return az_json_builder_write_span(self, value.data.span);
    }
    default: { return AZ_ERROR_ARG; }
  }
}

AZ_NODISCARD static az_result az_json_builder_write_comma(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  if (self->need_comma) {
    return az_span_action_do(self->write, AZ_SPAN_FROM_STR(","));
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_object_member(
    az_json_builder * const self,
    az_span const name,
    az_json_token const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(self));
  AZ_RETURN_IF_FAILED(az_json_builder_write_str(self, name));
  AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_SPAN_FROM_STR(":")));
  AZ_RETURN_IF_FAILED(az_json_builder_write(self, value));
  return AZ_OK;
}

AZ_NODISCARD static az_result az_json_builder_write_close(
    az_json_builder * const self,
    az_span const close) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_action_do(self->write, close));
  self->need_comma = true;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_object_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  return az_json_builder_write_close(self, AZ_SPAN_FROM_STR("}"));
}

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * const self, az_json_token const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(self));
  AZ_RETURN_IF_FAILED(az_json_builder_write(self, value));
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  return az_json_builder_write_close(self, AZ_SPAN_FROM_STR("]"));
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>

#include <az_str.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_json_builder_init(az_json_builder * const out, az_span_action const write) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  *out = (az_json_builder){ .write = write };

  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_write_str(az_json_builder * const self, az_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_action const write = self->write;
  AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_STR("\"")));
  AZ_RETURN_IF_FAILED(az_span_action_do(write, value));
  AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_STR("\"")));
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_write(az_json_builder * const self, az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_action const write = self->write;
  switch (value.kind) {
    case AZ_JSON_VALUE_NULL: {
      self->need_comma = true;
      return az_span_action_do(write, AZ_STR("null"));
    }
    case AZ_JSON_VALUE_BOOLEAN: {
      self->need_comma = true;
      return az_span_action_do(write, value.data.boolean ? AZ_STR("true") : AZ_STR("false"));
    }
    case AZ_JSON_VALUE_STRING: {
      self->need_comma = true;
      return az_json_builder_write_str(self, value.data.string);
    }
    case AZ_JSON_VALUE_NUMBER: {
      self->need_comma = true;
      // TODO:
      return AZ_ERROR_NOT_IMPLEMENTED;
    }
    case AZ_JSON_VALUE_OBJECT: {
      self->need_comma = false;
      return az_span_action_do(write, AZ_STR("{"));
    }
    case AZ_JSON_VALUE_ARRAY: {
      self->need_comma = false;
      return az_span_action_do(write, AZ_STR("["));
    }
    default: {
      return AZ_ERROR_JSON_INVALID_STATE;
    }
  }
}

AZ_NODISCARD az_result az_json_builder_write_object_member(
    az_json_builder * const self,
    az_span const name,
    az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  if (self->need_comma) {
    AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_STR(",")));
  }
  AZ_RETURN_IF_FAILED(az_json_builder_write_str(self, name));
  AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_STR(":")));
  AZ_RETURN_IF_FAILED(az_json_builder_write(self, value));
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_object_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_STR("}")));
  self->need_comma = true;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * const self, az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  if (self->need_comma) {
    AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_STR(",")));
  }
  AZ_RETURN_IF_FAILED(az_json_builder_write(self, value));
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_STR("]")));
  self->need_comma = true;
  return AZ_OK;
}

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

AZ_NODISCARD static az_result az_json_builder_write_str(
    az_json_builder * const self,
    az_span const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_action const write = self->write;
  AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_STR("\"")));
  AZ_RETURN_IF_FAILED(az_span_action_do(write, value));
  AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_STR("\"")));
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
    return az_span_action_do(write, AZ_STR("0"));
  }

  if (value < 0) {
    AZ_RETURN_IF_FAILED(az_span_action_do(write, AZ_STR("-")));
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
        uint8_t const dec = (uint8_t)(u / base) + '0';
        u %= base;
        base /= 10;
        AZ_RETURN_IF_FAILED(az_span_action_do(write, az_span_from_one(&dec)));
      } while (1 <= base); 
      return AZ_OK;
    }
  }

  if (value < 1) {
    // D.*De-*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  {
    // *D.*D
    // TODO:
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
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
      return az_json_builder_write_double(self, value.data.number);
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
      return AZ_ERROR_ARG;
    }
  }
}

AZ_NODISCARD static az_result az_json_builder_write_comma(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  if (self->need_comma) {
    return az_span_action_do(self->write, AZ_STR(","));
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_object_member(
    az_json_builder * const self,
    az_span const name,
    az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(self));
  AZ_RETURN_IF_FAILED(az_json_builder_write_str(self, name));
  AZ_RETURN_IF_FAILED(az_span_action_do(self->write, AZ_STR(":")));
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

  return az_json_builder_write_close(self, AZ_STR("}"));
}

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * const self, az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_json_builder_write_comma(self));
  AZ_RETURN_IF_FAILED(az_json_builder_write(self, value));
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  return az_json_builder_write_close(self, AZ_STR("]"));
}

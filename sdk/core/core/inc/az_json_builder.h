// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_JSON_BUILDER_H
#define _az_JSON_BUILDER_H

#include <az_json_token.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_action.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef struct {
  struct {
    az_span json;
    bool need_comma;
  } _internal;
} az_json_builder;

AZ_NODISCARD AZ_INLINE az_result az_json_builder_init(az_json_builder * self, az_span json_buffer) {
  *self = (az_json_builder){ ._internal = { .json = json_buffer, .need_comma = false } };
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_span az_json_builder_span_get(az_json_builder self) {
  return self._internal.json;
}

AZ_NODISCARD az_result az_json_builder_write(az_json_builder * self, az_json_token token);

AZ_NODISCARD az_result
az_json_builder_write_object_member(az_json_builder * self, az_span name, az_json_token value);

AZ_NODISCARD az_result az_json_builder_write_object_close(az_json_builder * self);

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * self, az_json_token value);

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * self);

#include <_az_cfg_suffix.h>

#endif

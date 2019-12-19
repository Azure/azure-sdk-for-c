// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_BUILDER_H
#define AZ_JSON_BUILDER_H

#include <az_json_value.h>
#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span_action write;
  bool need_comma;
  // TODO: add a JSON stack for validations.
} az_json_builder;

AZ_NODISCARD az_result
az_json_builder_init(az_json_builder * const out, az_span_action const write);

AZ_NODISCARD az_result
az_json_builder_write(az_json_builder * const self, az_json_value const value);

AZ_NODISCARD az_result az_json_builder_write_object_member(
    az_json_builder * const self,
    az_span const name,
    az_json_value const value);

AZ_NODISCARD az_result az_json_builder_write_object_close(az_json_builder * const self);

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * const self, az_json_value const value);

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * const self);

#include <_az_cfg_suffix.h>

#endif // !AZ_JSON_BUILDER_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_JSON_BUILDER_H
#define AZ_JSON_BUILDER_H

#include <az_json_value.h>
#include <az_span_builder.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span_builder buffer;
} az_json_builder;

AZ_NODISCARD az_result az_json_builder_init(az_json_builder * const out, az_mut_span const buffer);

AZ_NODISCARD az_result az_json_builder_result(az_json_builder const self, az_span * const out);

AZ_NODISCARD az_result
az_json_builder_write(az_json_builder * const self, az_json_value const value);

AZ_NODISCARD az_result
az_json_builder_write_object_member(az_json_builder * const self, az_json_member const member);

AZ_NODISCARD az_result az_json_builder_write_object_close(az_json_builder * const self);

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * const self, az_json_value const value);

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * const self);

#include <_az_cfg_suffix.h>

#endif // !AZ_JSON_BUILDER_H

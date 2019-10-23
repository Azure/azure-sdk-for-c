// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_BUILDER_H
#define AZ_SPAN_BUILDER_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span span;
  size_t i;
} az_span_builder;

AZ_INLINE az_span_builder az_span_builder_create(az_span const span) {
  return (az_span_builder){
    .span = span,
    .i = 0,
  };
}

AZ_INLINE az_span az_span_builder_result(az_span_builder const * const p_i) {
  return az_span_take(p_i->span, p_i->i);
}

az_result az_span_builder_append(az_span_builder * const p_builder, az_const_span const span);

AZ_CALLBACK_FUNC(az_span_builder_append, az_span_builder *, az_span_visitor)

#include <_az_cfg_suffix.h>

#endif

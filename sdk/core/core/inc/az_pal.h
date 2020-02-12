// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PAL_H
#define _az_PAL_H

#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

// TODO - move this into the PAL layer
AZ_INLINE AZ_NODISCARD az_result _az_pal_os_get(az_span * out_os) {

  *out_os = AZ_SPAN_FROM_STR("Unknown OS");
  // Add specific implementations for each PAL implementation
  //
  // os = AZ_STR("Unix 1.0.0.0");
  // os = AZ_STR("Windows_NT")

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
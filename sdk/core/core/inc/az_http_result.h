// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESULT_H
#define AZ_HTTP_RESULT_H

#include <az_result.h>

#include <_az_cfg_prefix.h>

enum {
  // error codes
  AZ_HTTP_ERROR_PAL = AZ_MAKE_ERROR(AZ_HTTP_FACILITY, 1),
  AZ_HTTP_ERROR_INVALID_STATE = AZ_MAKE_ERROR(AZ_HTTP_FACILITY, 2),
  AZ_HTTP_ERROR_NO_MORE_HEADERS = AZ_MAKE_ERROR(AZ_HTTP_FACILITY, 3),
};

#include <_az_cfg_suffix.h>

#endif

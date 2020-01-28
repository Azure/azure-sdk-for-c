// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_str_private.h"

#include <_az_cfg.h>

uint8_t zerobyte[] = { 0 };
az_span const AZ_SPAN_ZEROBYTE = AZ_SPAN_LITERAL_FROM_INITIALIZED_BUFFER(zerobyte);

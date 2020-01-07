// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_SPAN_CONTRACT_H
#define AZ_SPAN_CONTRACT_H

#include "_az_mut_span.h"
#include "_az_span.h"
#include "az_contract.h"

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT_ARG_VALID_MUT_SPAN(span) AZ_CONTRACT(az_mut_span_is_valid(span), AZ_ERROR_ARG)
#define AZ_CONTRACT_ARG_VALID_SPAN(span) AZ_CONTRACT(az_span_is_valid(span), AZ_ERROR_ARG)

#include <_az_cfg_suffix.h>

#endif

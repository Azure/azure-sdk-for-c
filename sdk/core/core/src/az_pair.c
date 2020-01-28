// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_pair.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_pair_as_writer(az_pair self, az_pair_action const write_pair, az_pair * out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);
  (void)write_pair;
  (void)self;

  // az_pair const * i = &self;
  // az_pair const * const end = i + out->size;
  /* for (; i < end; ++i) {
    AZ_RETURN_IF_FAILED(az_pair_action_do(write_pair, *i));
  } */
  return AZ_OK;
}

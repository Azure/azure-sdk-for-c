// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample_pnp_component_mqtt.h"

#include <stdint.h>

#include <azure/core/az_span.h>

static int32_t request_id_int;
static char request_id_buf[8];

// Create request id span which increments request id integer each call. Capable of holding 8 digit
// number.
az_span get_request_id(void)
{
  az_span remainder;
  az_span out_span = az_span_init((uint8_t*)request_id_buf, sizeof(request_id_buf));
  az_result result = az_span_i32toa(out_span, request_id_int++, &remainder);
  (void)remainder;
  (void)result;
  return out_span;
}

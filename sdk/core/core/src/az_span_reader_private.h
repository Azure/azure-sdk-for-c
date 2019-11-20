// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPAN_READER_PRIVATE_H
#define _az_SPAN_READER_PRIVATE_H

#include "az_span_private.h"
#include <az_span.h>
#include <az_span_reader.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_result_byte
az_span_reader_current(az_span_reader const * const p_reader) {
  return az_span_get(p_reader->span, p_reader->i);
}

AZ_NODISCARD az_result_byte az_span_reader_peek_next(az_span_reader* const self) {
  size_t curr_pos = self->i;
  az_span_reader_next(self);
  az_result_byte result = az_span_reader_current(self);
  AZ_RETURN_IF_FAILED(az_span_reader_set_pos(self, curr_pos));
  return result;
}

#include <_az_cfg_suffix.h>

#endif

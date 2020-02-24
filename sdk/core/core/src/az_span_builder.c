// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_builder.h>

#include <assert.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_span_builder_append(az_span_builder* self, az_span span)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_mut_span const remainder = az_mut_span_drop(self->buffer, self->length);
  az_mut_span result;
  AZ_RETURN_IF_FAILED(az_mut_span_move(remainder, span, &result));
  self->length += result.size;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_builder_append_byte(az_span_builder* self, uint8_t c)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  AZ_RETURN_IF_FAILED(az_mut_span_set(self->buffer, self->length, c));
  self->length += 1;
  return AZ_OK;
}

AZ_INLINE uint8_t _az_decimal_to_ascii(uint8_t d)
{
  assert(d < 10);
  return '0' + d;
}

static AZ_NODISCARD az_result _az_span_builder_append_uint64(az_span_builder* self, uint64_t n)
{
  if (n == 0)
  {
    return az_span_builder_append_byte(self, '0');
  }

  uint64_t div = 10000000000000000000ull;
  uint64_t nn = n;
  while (nn / div == 0)
  {
    div /= 10;
  }

  while (div > 1)
  {
    AZ_RETURN_IF_FAILED(
        az_span_builder_append_byte(self, _az_decimal_to_ascii((uint8_t)(nn / div))));

    nn %= div;
    div /= 10;
  }

  return az_span_builder_append_byte(self, _az_decimal_to_ascii((uint8_t)nn));
}

AZ_NODISCARD az_result az_span_builder_append_uint64(az_span_builder* self, uint64_t n)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);
  return _az_span_builder_append_uint64(self, n);
}

AZ_NODISCARD az_result az_span_builder_append_int64(az_span_builder* self, int64_t n)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  if (n < 0)
  {
    AZ_RETURN_IF_FAILED(az_span_builder_append_byte(self, '-'));
    return _az_span_builder_append_uint64(self, -n);
  }

  return _az_span_builder_append_uint64(self, n);
}

AZ_NODISCARD az_result
az_span_builder_replace(az_span_builder* self, size_t start, size_t end, az_span span)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  size_t const current_size = self->length;
  size_t const replaced_size = end - start;
  size_t const size_after_replace = current_size - replaced_size + span.size;

  // replaced size must be less or equal to current builder size. Can't replace more than what
  // current is available
  AZ_CONTRACT(replaced_size <= current_size, AZ_ERROR_ARG);
  // start and end position must be before the end of current builder size
  AZ_CONTRACT(start <= current_size && end <= current_size, AZ_ERROR_ARG);
  // Start position must be less or equal than end position
  AZ_CONTRACT(start <= end, AZ_ERROR_ARG);
  // size after replacing must be less o equal than buffer size
  AZ_CONTRACT(size_after_replace <= self->buffer.size, AZ_ERROR_ARG);

  // get the span then need to be moved before adding a new span
  az_mut_span const dst = az_mut_span_drop(self->buffer, start + span.size);
  // get the span where to move content
  az_span const src = az_span_drop(az_span_builder_result(self), end);
  {
    // use a dummy result to use span_move
    az_mut_span r = { 0 };
    // move content left or right so new span can be added
    AZ_RETURN_IF_FAILED(az_mut_span_move(dst, src, &r));
    // add the new span
    AZ_RETURN_IF_FAILED(az_mut_span_move(az_mut_span_drop(self->buffer, start), span, &r));
  }

  // update builder size
  self->length = size_after_replace;
  return AZ_OK;
}

AZ_NODISCARD az_result az_span_builder_append_zeros(az_span_builder* self, size_t size)
{
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_mut_span const span = az_mut_span_take(az_mut_span_drop(self->buffer, self->length), size);
  if (span.size != size)
  {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  az_mut_span_fill(span, 0);
  self->length += size;
  return AZ_OK;
}

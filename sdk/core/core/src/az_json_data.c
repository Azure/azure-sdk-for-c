// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>

#include <az_json_parser.h>
#include <az_span_builder.h>

#include <_az_cfg.h>

typedef struct {
  az_span span;
  size_t align;
} az_data;

#define AZ_DATA(V) \
  ((az_data){ \
      .span = (az_span){ .begin = (uint8_t const *)&(V), .size = sizeof(V) }, \
      .align = AZ_ALIGNOF(V), \
  })

uint8_t * _az_align_floor(uint8_t * const p, size_t const align) {
  return (uint8_t *)(((size_t)p) / align * align);
}

uint8_t * _az_align_ceil(uint8_t * const p, size_t const align) {
  return _az_align_floor(p + (align - 1), align);
}

AZ_NODISCARD az_result _az_span_builder_aligned_append(
    az_span_builder * const builder,
    az_data const data,
    uint8_t ** const out) {
  // alignment.
  {
    uint8_t * const p = builder->buffer.begin + builder->size;
    AZ_RETURN_IF_FAILED(az_span_builder_append_zeros(builder, _az_align_ceil(p, data.align) - p));
  }
  *out = builder->buffer.begin + builder->size;
  return az_span_builder_append(builder, data.span);
}

AZ_NODISCARD az_result
_az_span_builder_top_aligned_append(az_span_builder * const builder, az_data const data) {
  size_t const size = builder->buffer.size;
  if (size < data.span.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  uint8_t * const begin = builder->buffer.begin;
  uint8_t * const end = _az_align_floor(begin + size - data.span.size, data.align);
  if (end < begin + builder->size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  size_t const new_size = end - begin;
  {
    az_mut_span result = { 0 };
    AZ_RETURN_IF_FAILED(
        az_mut_span_move(az_mut_span_drop(builder->buffer, new_size), data.span, &result));
  }
  builder->buffer.size = new_size;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_span_builder_top_array_revert(
    az_span_builder * const builder,
    size_t const item_size,
    size_t const item_align,
    size_t const new_size,
    void ** const out_array_begin,
    size_t * const out_array_size) {
  AZ_CONTRACT_ARG_NOT_NULL(builder);
  AZ_CONTRACT_ARG_NOT_NULL(out_array_begin);
  AZ_CONTRACT_ARG_NOT_NULL(out_array_size);

  // 3. restore buffer size
  size_t const offset = builder->buffer.size;
  builder->buffer.size = new_size;
  az_mut_span const buffer = builder->buffer;

  // 1. revert the array.
  size_t const item_count = (new_size - offset) / item_size;
  for (size_t i = 0; i < item_count / 2; ++i) {
    az_mut_span const a
        = az_mut_span_take(az_mut_span_drop(buffer, offset + i * item_size), item_size);
    az_mut_span const b = az_mut_span_take(
        az_mut_span_drop(buffer, offset + (item_count - i - 1) * item_size), item_size);
    AZ_RETURN_IF_FAILED(az_mut_span_swap(a, b));
  }

  // 2. move it to front.
  az_data const data = {
    .span = az_span_take(az_span_drop(az_mut_span_to_span(buffer), offset), item_count * item_size),
    .align = item_align,
  };
  AZ_RETURN_IF_FAILED(_az_span_builder_top_aligned_append(builder, data));

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_span_builder_append_json_data(
    az_span_builder * const builder,
    az_json_data const data,
    az_json_data const ** const out) {
  uint8_t * p = { 0 };
  AZ_RETURN_IF_FAILED(_az_span_builder_aligned_append(builder, AZ_DATA(data), &p));
  *out = (az_json_data const *)p;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_span_builder_write_json_value(
    az_span_builder * const builder,
    az_json_parser * const parser,
    az_json_token const token,
    az_json_data * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(builder);
  AZ_CONTRACT_ARG_NOT_NULL(parser);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  switch (token.kind) {
    case AZ_JSON_TOKEN_NULL: {
      *out = az_json_data_null();
      break;
    }
    case AZ_JSON_TOKEN_BOOLEAN: {
      *out = az_json_data_boolean(token.data.boolean);
      break;
    }
    case AZ_JSON_TOKEN_NUMBER: {
      *out = az_json_data_number(token.data.number);
      break;
    }
    case AZ_JSON_TOKEN_STRING: {
      // TODO: escape symbols.
      *out = az_json_data_string(
          (az_span){ .begin = builder->buffer.begin + builder->size, token.data.string.size });
      AZ_RETURN_IF_FAILED(az_span_builder_append(builder, token.data.string));
      break;
    }
    case AZ_JSON_TOKEN_OBJECT: {
      size_t const buffer_size = builder->buffer.size;
      while (true) {
        az_json_token_member member;
        az_result const result = az_json_parser_read_object_member(parser, &member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        az_json_object_member om = { 0 };
        // TODO: read member `om`.
        AZ_RETURN_IF_FAILED(_az_span_builder_top_aligned_append(builder, AZ_DATA(om)));
      }
      void * p = { 0 };
      size_t i = 0;
      AZ_RETURN_IF_FAILED(_az_span_builder_top_array_revert(
          builder,
          sizeof(az_json_object_member),
          AZ_ALIGNOF(az_json_object_member),
          buffer_size,
          &p,
          &i));
      *out = az_json_data_object((az_json_object){ .begin = p, .size = i });
      break;
    }
    case AZ_JSON_TOKEN_ARRAY: {
      // TODO: copy array elements.
      *out = az_json_data_array((az_json_array){ .begin = NULL, .size = 0 });
      AZ_RETURN_IF_FAILED(az_json_parser_skip(parser, token));
      break;
    }
    default: {
      return AZ_ERROR_JSON_INVALID_STATE;
    }
  }
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_to_data(az_span const json, az_mut_span const buffer, az_json_data const ** const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_json_parser parser = az_json_parser_create(json);
  az_span_builder builder = az_span_builder_create(buffer);
  az_json_token token = { 0 };
  AZ_RETURN_IF_FAILED(az_json_parser_read(&parser, &token));
  az_json_data data = { 0 };
  AZ_RETURN_IF_FAILED(_az_span_builder_write_json_value(&builder, &parser, token, &data));
  return _az_span_builder_append_json_data(&builder, data, out);
}

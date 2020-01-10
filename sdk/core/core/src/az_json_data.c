// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>

#include <az_json_parser.h>
#include <az_span_builder.h>

#include <_az_cfg.h>

uint8_t * _az_align_floor(uint8_t * const p, size_t const align) {
  return (uint8_t *)(((size_t)p) / align * align);
}

uint8_t * _az_align_ceil(uint8_t * const p, size_t const align) {
  return _az_align_floor(p + (align - 1), align);
}

/**
 * Run-time type properties.
 */
typedef struct {
  size_t size;
  size_t align;
} _az_type;

/**
 * Creates @_az_type from the given type/variable.
 */
#define _AZ_TYPE(DATA_TYPE) \
  ((_az_type){ \
      .size = sizeof(DATA_TYPE), \
      .align = AZ_ALIGNOF(DATA_TYPE), \
  })

/**
 * Run-time data properties.
 */
typedef struct {
  void const * p;
  _az_type type;
} _az_data;

/**
 * Creates @_az_data from the given variable.
 */
#define _AZ_DATA(DATA) ((_az_data){ .p = &DATA, .type = _AZ_TYPE(DATA) })

AZ_NODISCARD AZ_INLINE az_span _az_data_get_span(_az_data const data) {
  return (az_span){ .begin = data.p, .size = data.type.size };
}

AZ_NODISCARD az_result _az_span_builder_aligned_append(
    az_span_builder * const builder,
    _az_data const data,
    void ** const out) {
  // alignment.
  {
    uint8_t * const p = builder->buffer.begin + builder->length;
    AZ_RETURN_IF_FAILED(
        az_span_builder_append_zeros(builder, _az_align_ceil(p, data.type.align) - p));
  }
  *out = builder->buffer.begin + builder->length;
  return az_span_builder_append(builder, _az_data_get_span(data));
}

AZ_NODISCARD az_result
_az_span_builder_top_aligned_append(az_span_builder * const builder, _az_data const data) {
  size_t const size = builder->buffer.size;
  if (size < data.type.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  uint8_t * const begin = builder->buffer.begin;
  uint8_t * const end = _az_align_floor(begin + size - data.type.size, data.type.align);
  if (end < begin + builder->length) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  size_t const new_size = end - begin;
  {
    az_mut_span result = { 0 };
    AZ_RETURN_IF_FAILED(az_mut_span_move(
        az_mut_span_drop(builder->buffer, new_size), _az_data_get_span(data), &result));
  }
  builder->buffer.size = new_size;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_span_builder_top_array_revert(
    az_span_builder * const builder,
    _az_type const item_type,
    size_t const new_size,
    void ** const out_array_begin,
    size_t * const out_array_size) {
  AZ_CONTRACT_ARG_NOT_NULL(builder);
  AZ_CONTRACT_ARG_NOT_NULL(out_array_begin);
  AZ_CONTRACT_ARG_NOT_NULL(out_array_size);

  // 1. restore buffer size
  size_t const offset = builder->buffer.size;
  builder->buffer.size = new_size;
  az_mut_span const buffer = builder->buffer;

  // 2. revert the array.
  size_t const item_count = (new_size - offset) / item_type.size;
  for (size_t i = 0; i < item_count / 2; ++i) {
    az_mut_span const a
        = az_mut_span_take(az_mut_span_drop(buffer, offset + i * item_type.size), item_type.size);
    az_mut_span const b = az_mut_span_take(
        az_mut_span_drop(buffer, offset + (item_count - i - 1) * item_type.size), item_type.size);
    az_mut_span_swap(a, b);
  }

  // 3. move it to front.
  az_span const result_array = az_span_take(
      az_span_drop(az_mut_span_to_span(buffer), offset), item_count * item_type.size);
  _az_data const data = {
    .p = result_array.begin,
    .type = { .size = result_array.size, .align = item_type.align },
  };
  AZ_RETURN_IF_FAILED(_az_span_builder_aligned_append(builder, data, out_array_begin));
  *out_array_size = item_count;

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_span_builder_append_json_data(
    az_span_builder * const builder,
    az_json_data const data,
    az_json_data const ** const out) {
  uint8_t * p = { 0 };
  AZ_RETURN_IF_FAILED(_az_span_builder_aligned_append(builder, _AZ_DATA(data), &p));
  *out = (az_json_data const *)p;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_span_builder_write_json_string(
    az_span_builder * const builder,
    az_span const json_string,
    az_span * const out) {
  // TODO: escape symbols.
  *out = (az_span){ .begin = builder->buffer.begin + builder->length, json_string.size };
  return az_span_builder_append(builder, json_string);
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
      az_span s = { 0 };
      AZ_RETURN_IF_FAILED(_az_span_builder_write_json_string(builder, token.data.string, &s));
      *out = az_json_data_string(s);
      break;
    }
    case AZ_JSON_TOKEN_OBJECT: {
      size_t const buffer_size = builder->buffer.size;
      while (true) {
        az_json_token_member token_member;
        az_result const result = az_json_parser_read_object_member(parser, &token_member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        az_json_object_member data_member = { 0 };
        AZ_RETURN_IF_FAILED(
            _az_span_builder_write_json_string(builder, token_member.name, &data_member.name));
        AZ_RETURN_IF_FAILED(_az_span_builder_write_json_value(
            builder, parser, token_member.value, &data_member.value));
        AZ_RETURN_IF_FAILED(_az_span_builder_top_aligned_append(builder, _AZ_DATA(data_member)));
      }
      void * p = { 0 };
      size_t i = 0;
      AZ_RETURN_IF_FAILED(_az_span_builder_top_array_revert(
          builder, _AZ_TYPE(az_json_object_member), buffer_size, &p, &i));
      *out = az_json_data_object((az_json_object){ ._detail = { .begin = p, .size = i } });
      break;
    }
    case AZ_JSON_TOKEN_ARRAY: {
      size_t const buffer_size = builder->buffer.size;
      while (true) {
        az_json_token item_token;
        az_result const result = az_json_parser_read_array_element(parser, &item_token);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        az_json_data value = { 0 };
        AZ_RETURN_IF_FAILED(_az_span_builder_write_json_value(builder, parser, item_token, &value));
        AZ_RETURN_IF_FAILED(_az_span_builder_top_aligned_append(builder, _AZ_DATA(value)));
      }
      void * p = { 0 };
      size_t i = 0;
      AZ_RETURN_IF_FAILED(
          _az_span_builder_top_array_revert(builder, _AZ_TYPE(az_json_data), buffer_size, &p, &i));
      *out = az_json_data_array((az_json_array){ ._detail = { .begin = p, .size = i } });
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

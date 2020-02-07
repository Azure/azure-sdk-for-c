// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_data.h>

#include "az_span_private.h"
#include <az_json_parser.h>

#include <_az_cfg.h>

uint8_t * _az_align_floor(uint8_t * p, size_t align) {
  return (uint8_t *)(((size_t)p) / align * align);
}

uint8_t * _az_align_ceil(uint8_t * p, size_t align) {
  return _az_align_floor(p + (align - 1), align);
}

/**
 * Run-time type properties.
 */
typedef struct {
  int32_t size;
  int32_t align;
} _az_type;

// MSVC
#if defined(_MSC_VER)
#define _az_ALIGNOF(T) __alignof(T)
// GCC
#elif defined(__GNUC__)
#define _az_ALIGNOF(T) __alignof__(T)
// unknown
#else
#define _az_ALIGNOF(T) _Alignof(T)
#endif

/**
 * Creates @_az_type from the given type/variable.
 */
#define _az_TYPE(DATA_TYPE) \
  ((_az_type){ \
      .size = sizeof(DATA_TYPE), \
      .align = _az_ALIGNOF(DATA_TYPE), \
  })

/**
 * Run-time data properties.
 */
typedef struct {
  void * p;
  _az_type type;
} _az_data;

/**
 * Creates @_az_data from the given variable.
 */
#define _az_DATA(DATA) ((_az_data){ .p = &DATA, .type = _az_TYPE(DATA) })

AZ_NODISCARD AZ_INLINE az_span _az_data_get_span(_az_data data) {
  return az_span_init(data.p, data.type.size, data.type.size);
}

AZ_NODISCARD az_result
_az_span_aligned_append(az_span * builder, _az_data const data, void const ** out) {
  // alignment.
  {
    uint8_t * const p = az_span_ptr(*builder) + az_span_length(*builder);
    AZ_RETURN_IF_FAILED(az_span_append_zeros(builder, _az_align_ceil(p, data.type.align) - p));
  }
  *out = az_span_ptr(*builder) + az_span_length(*builder);
  return az_span_append(*builder, _az_data_get_span(data), builder);
}

AZ_NODISCARD az_result _az_span_top_aligned_append(az_span * const builder, _az_data data) {
  int32_t const size = az_span_capacity(*builder);
  if (size < data.type.size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  uint8_t * const begin = az_span_ptr(*builder);
  uint8_t * const end = _az_align_floor(begin + size - data.type.size, data.type.align);
  if (end < begin + az_span_length(*builder)) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  int32_t const new_size = end - begin;
  {
    az_span result = { 0 };
    az_span copy = az_span_drop(*builder, new_size);
    AZ_RETURN_IF_FAILED(az_span_copy(copy, _az_data_get_span(data), &result));
  }
  // TODO: use setter?
  builder->_internal.capacity = new_size;
  return AZ_OK;
}

AZ_NODISCARD az_result _az_span_top_array_revert(
    az_span * builder,
    _az_type item_type,
    int32_t new_size,
    void const ** out_array_begin,
    int32_t * out_array_size) {
  AZ_CONTRACT_ARG_NOT_NULL(builder);
  AZ_CONTRACT_ARG_NOT_NULL(out_array_begin);
  AZ_CONTRACT_ARG_NOT_NULL(out_array_size);

  // 1. restore buffer size
  int32_t const offset = az_span_capacity(*builder);
  // TODO: Remove the capacity mutation
  builder->_internal.capacity = new_size;
  az_span const buffer = *builder;

  // 2. revert the array.
  size_t const item_count = (new_size - offset) / item_type.size;
  for (size_t i = 0; i < item_count / 2; ++i) {
    az_span const a
        = az_span_take(az_span_drop(buffer, offset + i * item_type.size), item_type.size);
    az_span const b = az_span_take(
        az_span_drop(buffer, offset + (item_count - i - 1) * item_type.size), item_type.size);
    az_span_swap(a, b);
  }

  // 3. move it to front.
  az_span const result_array
      = az_span_take(az_span_drop(buffer, offset), item_count * item_type.size);
  _az_data const data = {
    .p = az_span_ptr(result_array),
    .type = { .size = az_span_length(result_array), .align = item_type.align },
  };
  AZ_RETURN_IF_FAILED(_az_span_aligned_append(builder, data, out_array_begin));
  *out_array_size = item_count;

  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result
_az_span_append_json_data(az_span * builder, az_json_data data, az_json_data const ** out) {
  void const * p = { 0 };
  AZ_RETURN_IF_FAILED(_az_span_aligned_append(builder, _az_DATA(data), &p));
  *out = (az_json_data const *)p;
  return AZ_OK;
}

AZ_NODISCARD az_result
_az_span_write_json_string(az_span * builder, az_span json_string, az_span * out) {
  // TODO: escape symbols.
  int32_t const json_string_length = az_span_length(json_string);
  *out = az_span_init(
      az_span_ptr(*builder) + az_span_length(*builder), json_string_length, json_string_length);

  return az_span_append(*builder, json_string, builder);
}

AZ_NODISCARD az_result _az_span_write_json_value(
    az_span * builder,
    az_json_parser * parser,
    az_json_token token,
    az_json_data * out) {
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
      AZ_RETURN_IF_FAILED(_az_span_write_json_string(builder, token.data.string, &s));
      *out = az_json_data_string(s);
      break;
    }
    case AZ_JSON_TOKEN_OBJECT: {
      size_t const buffer_size = az_span_capacity(*builder);
      while (true) {
        az_json_token_member token_member;
        az_result const result = az_json_parser_parse_token_member(parser, &token_member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        az_json_object_member data_member = { 0 };
        AZ_RETURN_IF_FAILED(
            _az_span_write_json_string(builder, token_member.name, &data_member.name));
        AZ_RETURN_IF_FAILED(
            _az_span_write_json_value(builder, parser, token_member.value, &data_member.value));
        AZ_RETURN_IF_FAILED(_az_span_top_aligned_append(builder, _az_DATA(data_member)));
      }
      void const * p = { 0 };
      int32_t i = 0;
      AZ_RETURN_IF_FAILED(
          _az_span_top_array_revert(builder, _az_TYPE(az_json_object_member), buffer_size, &p, &i));
      *out = az_json_data_object((az_json_object){ ._internal = { .begin = p, .size = i } });
      break;
    }
    case AZ_JSON_TOKEN_ARRAY: {
      int32_t const buffer_size = az_span_capacity(*builder);
      while (true) {
        az_json_token item_token;
        az_result const result = az_json_parser_parse_array_item(parser, &item_token);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        az_json_data value = { 0 };
        AZ_RETURN_IF_FAILED(_az_span_write_json_value(builder, parser, item_token, &value));
        AZ_RETURN_IF_FAILED(_az_span_top_aligned_append(builder, _az_DATA(value)));
      }
      void const * p = { 0 };
      int32_t i = 0;
      AZ_RETURN_IF_FAILED(
          _az_span_top_array_revert(builder, _az_TYPE(az_json_data), buffer_size, &p, &i));
      *out = az_json_data_array((az_json_array){ ._internal = { .begin = p, .size = i } });
      break;
    }
    default: {
      return AZ_ERROR_JSON_INVALID_STATE;
    }
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_to_data(az_span json, az_span buffer, az_json_data const ** out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_json_parser parser = az_json_parser_init(json);

  az_json_token token = { 0 };
  AZ_RETURN_IF_FAILED(az_json_parser_read(&parser, &token));
  az_json_data data = { 0 };
  AZ_RETURN_IF_FAILED(_az_span_write_json_value(&buffer, &parser, token, &data));
  return _az_span_append_json_data(&buffer, data, out);
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_JSON_PRIVATE_H
#define _az_JSON_PRIVATE_H

#include <azure/core/az_json.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg_prefix.h>

#define _az_JSON_TOKEN_DEFAULT (az_json_token){ \
  .kind = AZ_JSON_TOKEN_NONE, \
  ._internal = { 0 } \
}

enum
{
  // We are using a uint64_t to represent our nested state, so we can only go 64 levels deep.
  // This is safe to do because sizeof will not dereference the pointer and is used to find the size
  // of the field used as the stack.
  _az_MAX_JSON_STACK_SIZE = sizeof(((_az_json_bit_stack*)0)->_internal.az_json_stack) * 8 // 64
};

enum
{
  // Max size for an already escaped string value (~ half of INT_MAX)
  _az_MAX_ESCAPED_STRING_SIZE = 1000000000,

  // In the worst case, an ASCII character represented as a single UTF-8 byte could expand 6x when
  // escaped.
  // For example: '+' becomes '\u0043'
  // Escaping surrogate pairs (represented by 3 or 4 UTF-8 bytes) would expand to 12 bytes (which is
  // still <= 6x).
  _az_MAX_EXPANSION_FACTOR_WHILE_ESCAPING = 6,

  _az_MAX_UNESCAPED_STRING_SIZE
  = _az_MAX_ESCAPED_STRING_SIZE / _az_MAX_EXPANSION_FACTOR_WHILE_ESCAPING, // 166_666_666 bytes
};

typedef enum
{
  _az_JSON_STACK_OBJECT = 1,
  _az_JSON_STACK_ARRAY = 0,
} _az_json_stack_item;

AZ_INLINE _az_json_stack_item _az_json_stack_pop(_az_json_bit_stack* json_stack)
{
  _az_PRECONDITION(
      json_stack->_internal.current_depth > 0
      && json_stack->_internal.current_depth <= _az_MAX_JSON_STACK_SIZE);

  // Don't do the right bit shift if we are at the last bit in the stack.
  if (json_stack->_internal.current_depth != 0)
  {
    json_stack->_internal.az_json_stack >>= 1;

    // We don't want current_depth to become negative, in case preconditions are off, and if
    // append_container_end is called before append_X_start.
    json_stack->_internal.current_depth--;
  }

  // true (i.e. 1) means _az_JSON_STACK_OBJECT, while false (i.e. 0) means _az_JSON_STACK_ARRAY
  return (json_stack->_internal.az_json_stack & 1) != 0 ? _az_JSON_STACK_OBJECT : _az_JSON_STACK_ARRAY;
}

AZ_INLINE void _az_json_stack_push(_az_json_bit_stack* json_stack, _az_json_stack_item item)
{
  _az_PRECONDITION(
      json_stack->_internal.current_depth >= 0
      && json_stack->_internal.current_depth < _az_MAX_JSON_STACK_SIZE);

  json_stack->_internal.current_depth++;
  json_stack->_internal.az_json_stack <<= 1;
  json_stack->_internal.az_json_stack |= item;
}

AZ_NODISCARD AZ_INLINE _az_json_stack_item _az_json_stack_peek(_az_json_bit_stack const* json_stack)
{
  _az_PRECONDITION(
      json_stack->_internal.current_depth >= 0
      && json_stack->_internal.current_depth <= _az_MAX_JSON_STACK_SIZE);

  // true (i.e. 1) means _az_JSON_STACK_OBJECT, while false (i.e. 0) means _az_JSON_STACK_ARRAY
  return (json_stack->_internal.az_json_stack & 1) != 0 ? _az_JSON_STACK_OBJECT : _az_JSON_STACK_ARRAY;
}

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_SPAN_PRIVATE_H

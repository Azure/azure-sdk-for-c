// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_context.h>

#include <stddef.h>

#include <_az_cfg.h>

// This is a global az_context node representing the entire application. By default, this node
// never expires. Call az_context_cancel passing a pointer to this node to cancel the entire
// application (which cancels all the child nodes).
az_context az_context_app = {
  ._internal
  = { .parent = NULL, .expiration = _az_CONTEXT_MAX_EXPIRATION, .key = NULL, .value = NULL }
};

// Returns the soonest expiration time of this az_context node or any of its parent nodes.
AZ_NODISCARD int64_t az_context_get_expiration(az_context const* context)
{
  int64_t expiration = _az_CONTEXT_MAX_EXPIRATION;
  for (; context != NULL; context = context->_internal.parent)
  {
    if (context->_internal.expiration < expiration)
      expiration = context->_internal.expiration;
  }
  return expiration;
}

// Walks up this az_context node's parent until it find a node whose key matches the specified key
// and return the corresponding value. Returns AZ_ERROR_ITEM_NOT_FOUND is there are no nodes
// matching the specified key.
AZ_NODISCARD az_result az_context_get_value(az_context const* context, void* key, void** out_value)
{
  for (; context != NULL; context = context->_internal.parent)
  {
    if (context->_internal.key == key)
    {
      *out_value = context->_internal.value;
      return AZ_OK;
    }
  }
  *out_value = NULL;
  return AZ_ERROR_ITEM_NOT_FOUND;
}

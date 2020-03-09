// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_context.h
 *
 * @brief Context for cancelling long running operations.
 */

#ifndef _az_CONTEXT_H
#define _az_CONTEXT_H

#include <az_result.h>

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct az_context az_context;

/**
 * @brief An az_context value in a node in a tree that represents expiration times and key/value
 * pairs. The root node in the tree (ultimate parent) is az_context_app which is a context for the
 * entire application. Each new node is a child of some parent.
 */
struct az_context
{
  struct
  {
    az_context const* parent; // Pointer to parent context (or NULL); immutable after creation
    int64_t expiration; // Time when context expires
    void* key;
    void* value; // Pointers to the key & value (usually NULL)
  } _internal;
};

#define _az_CONTEXT_MAX_EXPIRATION 0x7FFFFFFFFFFFFFFF

extern az_context az_context_app;

// Creates a new child az_context node by specifying its parent and the time when the node expires
AZ_NODISCARD AZ_INLINE az_context
az_context_with_expiration(az_context const* parent, int64_t expiration)
{
  return (az_context){ ._internal = { .parent = parent, .expiration = expiration } };
}

// Creates a new child az_context node by specifying its parent and its key/value pairs.
AZ_NODISCARD AZ_INLINE az_context
az_context_with_value(az_context const* parent, void* key, void* value)
{
  return (az_context){
    ._internal
    = { .parent = parent, .expiration = _az_CONTEXT_MAX_EXPIRATION, .key = key, .value = value }
  };
}

// Cancels an az_context node in the tree; this effectively cancels all the child nodes as well.
AZ_INLINE void az_context_cancel(az_context* context)
{
  if (context != NULL)
  {
    context->_internal.expiration = 0; // The beginning of time
  }
}

/**
 * @brief Returns the soonest expiration time of this az_context node or any of its parent nodes.
 *
 * @param context the leaf context
 * @return the soonest expiration time from this context and its parents
 */
AZ_NODISCARD int64_t az_context_get_expiration(az_context const* context);

// Returns true if this az_context node or any of its parent nodes' expiration is before the
// current time.
AZ_NODISCARD AZ_INLINE bool az_context_has_expired(az_context const* context, int64_t current_time)
{
  return az_context_get_expiration(context) < current_time;
}

// Walks up this az_context node's parent until it find a node whose key matches the specified key
// and return the corresponding value. Returns AZ_ERROR_ITEM_NOT_FOUND is there are no nodes
// matching the specified key.
AZ_NODISCARD az_result az_context_get_value(az_context const* context, void* key, void** out_value);

#include <_az_cfg_suffix.h>

#endif // _az_CONTEXT_H

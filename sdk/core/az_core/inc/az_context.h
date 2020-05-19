// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_context.h
 *
 * @brief Context for cancelling long running operations.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_CONTEXT_H
#define _az_CONTEXT_H

#include <az_result.h>

#include <stddef.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct az_context az_context;

/**
 * @brief A context is a node within a tree that represents expiration times and key/value
 * pairs.
 * @details The root node in the tree (ultimate parent) is #az_context_app which is a context for the
 * entire application. Each new node is a child of some parent.
 */
struct az_context
{
  struct
  {
    az_context const* parent; // Pointer to parent context (or NULL); immutable after creation
    int64_t expiration; // Time when context expires
    void* key; // Pointers to the key & value (usually NULL)
    void* value;
  } _internal;
};

#define _az_CONTEXT_MAX_EXPIRATION 0x7FFFFFFFFFFFFFFF

/**
 * @brief az_context_app is the ultimate root of all az_context instances. It allows you to cancel
 * your entire application. The az_context_app never expires but you can explciit cancel it by
 * passing its address to az_context_cancel which effectively cancels all the az_context child
 * nodes.
 */
extern az_context az_context_app;

/**
 * @brief az_context_with_expiration creates a newexpiring az_context node that is a child of the
 * specified parent.
 *
 * @param[in] parent The az_context node that the new node is to be a child of; passing NULL sets
 * the parent to az_context_app.
 * @param[in] expiration The time when this new child node should be canceled
 * @return The new child az_context node
 */
AZ_NODISCARD AZ_INLINE az_context
az_context_with_expiration(az_context const* parent, int64_t expiration)
{
  return (az_context){ ._internal = { .parent = ((parent != NULL) ? parent : &az_context_app),
                                      .expiration = expiration } };
}

/**
 * @brief az_context_with_value creates a new key/value az_context node that is a child of the
 * specified parent.
 *
 * @param[in] parent The az_context node that the new node is to be a child of; passing NULL sets
 * the parent to az_context_app.
 * @param[in] key A pointer to the key of this new az_context node
 * @param[in] value A pointer to the value of this new az_context node
 * @return The new child az_context node
 */
AZ_NODISCARD AZ_INLINE az_context
az_context_with_value(az_context const* parent, void* key, void* value)
{
  return (az_context){ ._internal = { .parent = (parent != NULL) ? parent : &az_context_app,
                                      .expiration = _az_CONTEXT_MAX_EXPIRATION,
                                      .key = key,
                                      .value = value } };
}

/**
 * @brief az_context_cancel cancels the specified az_context node; this cancels all the child nodes
 * as well.
 *
 * @param[in] context A pointer to the az_context node to be canceled; passing NULL cancels the root
 * az_context_app.
 */
AZ_INLINE void az_context_cancel(az_context* context)
{
  context = ((context != NULL) ? context : &az_context_app);
  context->_internal.expiration = 0; // The beginning of time
}

/**
 * @brief az_context_get_expiration returns the soonest expiration time of this az_context node or
 * any of its parent nodes.
 *
 * @param context A pointer to an az_context node
 * @return the soonest expiration time from this context and its parents
 */
AZ_NODISCARD int64_t az_context_get_expiration(az_context const* context);

/**
 * @brief az_context_has_expired returns true if this az_context node or any of its parent nodes'
 * expiration is before the current time.
 *
 * @param[in] context A pointer to the az_context node to check; passing NULL checks the root
 * az_context_app.
 * @param[in] current_time The current time.
 */
AZ_NODISCARD AZ_INLINE bool az_context_has_expired(az_context const* context, int64_t current_time)
{
  return az_context_get_expiration(context) < current_time;
}

/**
 * @brief az_context_get_value walks up this az_context node's parents until it find a node whose
 * key matches the specified key and returns the corresponding value.
 *
 * @param[in] context The az_context node in the tree where checking starts.
 * @param[in] key A pointer to the key to be scanned for
 * @param[in] out_value A pointer to a 'void*' that will receive the key's associated value if the
 * key is found
 * @return  #AZ_OK if the key is found
 *          #AZ_ERROR_ITEM_NOT_FOUND if no nodes are found with the specified key.
 */
AZ_NODISCARD az_result az_context_get_value(az_context const* context, void* key, void** out_value);

#include <_az_cfg_suffix.h>

#endif // _az_CONTEXT_H

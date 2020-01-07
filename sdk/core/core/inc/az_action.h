// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_ACTION_H
#define AZ_ACTION_H

#include <az_result.h>
#include <az_static_assert.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

#define AZ_CONTRACT(condition, error) \
  do { \
    if (!(condition)) { \
      return error; \
    } \
  } while (0)

#define AZ_CONTRACT_ARG_NOT_NULL(arg) AZ_CONTRACT((arg) != NULL, AZ_ERROR_ARG)

#define AZ_CAT(A, B) A##B

#define AZ_ACTION_ARG(NAME) AZ_CAT(NAME, _arg)

/**
 * Defines an action @NAME type which accepts an argument of type @ARG.
 */
#define AZ_ACTION_TYPE(NAME, ARG) \
  typedef ARG AZ_ACTION_ARG(NAME); \
  typedef struct { \
    az_result (*func)(void *, ARG); \
    void * self; \
  } NAME; \
  AZ_NODISCARD AZ_INLINE az_result AZ_CAT(NAME, _do)(NAME const action, ARG const arg) { \
    AZ_CONTRACT_ARG_NOT_NULL(action.func); \
    return action.func(action.self, arg); \
  }

AZ_STATIC_ASSERT(sizeof(size_t) == sizeof(void *))

/**
 * Defines a function @NAME##_action which creates an action of type @ACTION
 * using the given @NAME function.
 *
 * The function is a safe way to create an action with the same type @SELF for
 * the first argument of the function @NAME and @ACTION.self.
 */
#define AZ_ACTION_FUNC(NAME, SELF, ACTION) \
  AZ_NODISCARD az_result NAME(SELF * const, AZ_ACTION_ARG(ACTION) const); \
  AZ_NODISCARD AZ_INLINE ACTION AZ_CAT(NAME, _action)(SELF * const self) { \
    return (ACTION){ \
      .func = (az_result(*)(void *, AZ_ACTION_ARG(ACTION)))NAME, \
      .self = (void *)(size_t)self, \
    }; \
  }

#include <_az_cfg_suffix.h>

#endif

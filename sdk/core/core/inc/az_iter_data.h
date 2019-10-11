// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_ITER_DATA_H
#define AZ_ITER_DATA_H

#include <_az_cfg_prefix.h>

/**
 * Generic data for any polymorphic iterator.
 *
 * The assumption is that one pointer points to an immutable container and another one is used
 * for iteration (mutable field). 
 *
 * Examples:
 * - span iterator:
 *   - @begin points to the first item (this value is mutable)
 *   - @end points to the item after the last one (this value is immutable)
 * - linked list:
 *   - @begin points to the first item of a list.
 *   - @end is NULL
 * - FILE
 *   - @begin is a FILE HANDLE
 *   - @end is a position in the file.
 */
typedef struct {
  void const * begin;
  void const * end;
} az_iter_data;

#include <_az_cfg_suffix.h>

#endif

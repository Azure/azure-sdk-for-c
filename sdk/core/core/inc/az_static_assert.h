// SPDX-License-Identifier: MIT

#ifndef AZ_STATIC_ASSERT_H
#define AZ_STATIC_ASSERT_H

#include <stdbool.h>

#include <_az_cfg_prefix.h>

inline void az_static_assert(int x[1]) { x; }

#define AZ_STATIC_ASSERT(CONDITION) inline void az_static_assert(int x[(CONDITION) ? 1 : -1]);

AZ_STATIC_ASSERT(true)

#include <_az_cfg_suffix.h>

#endif

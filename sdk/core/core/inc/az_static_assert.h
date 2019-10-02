// SPDX-License-Identifier: MIT

#ifndef AZ_STATIC_ASSERT_H
#define AZ_STATIC_ASSERT_H

#include <stdbool.h>

#include <_az_cfg_prefix.h>

#define AZ_STATIC_ASSERT(CONDITION) typedef int az_static_assert[(CONDITION) ? 1 : -1];

AZ_STATIC_ASSERT(true)

#include <_az_cfg_suffix.h>

#endif

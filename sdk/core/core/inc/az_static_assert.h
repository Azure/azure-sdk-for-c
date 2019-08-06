// SPDX-License-Identifier: MIT

#ifndef AZ_STATIC_ASSERT_H
#define AZ_STATIC_ASSERT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AZ_STATIC_ASSERT(C) typedef int _az_static_assert[(C) ? 1 : -1];

AZ_STATIC_ASSERT(true);

#ifdef __cplusplus
}
#endif

#endif

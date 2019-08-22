// SPDX-License-Identifier: MIT

#ifndef AZ_ASSERT_H
#define AZ_ASSERT_H

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AZ_ASSERT(CONDITION) assert(CONDITION)

#ifdef __cplusplus
}
#endif

#endif

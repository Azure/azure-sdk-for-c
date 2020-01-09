// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <_az_cfg_extern_include_prefix.h>

#include <assert.h>

#include <_az_cfg_extern_include_suffix.h>

extern int exit_code;

#define TEST_ASSERT(c) \
  do { \
    if (c) { \
      printf("  - `%s`: succeeded\n", #c); \
    } else { \
      fprintf(stderr, "  - `%s`: failed\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while (false);

#define TEST_EXPECT_SUCCESS(exp) TEST_ASSERT(az_succeeded(exp))

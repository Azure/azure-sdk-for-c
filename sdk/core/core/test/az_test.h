// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef az_TEST_H
#define az_TEST_H

#include <assert.h>
#include <stdio.h>

extern int exit_code;

#define TEST_ASSERT(c) \
  do \
  { \
    if (c) \
    { \
      printf("  - `%s`: succeeded\n", #c); \
    } \
    else \
    { \
      fprintf(stderr, "  - `%s`: failed\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while (false);

#define TEST_EXPECT_SUCCESS(exp) TEST_ASSERT(az_succeeded(exp))

#endif // az_TEST_H
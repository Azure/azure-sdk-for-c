// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <stdio.h>

extern int exit_code;

#define _az_ANSI_COLOR_RED "\x1b[31m"
#define _az_ANSI_COLOR_GREEN "\x1b[32m"
#define _az_ANSI_COLOR_RESET "\x1b[0m"

#define TEST_ASSERT(c) \
  do \
  { \
    if (c) \
    { \
      printf("  - `%s`: " _az_ANSI_COLOR_GREEN "succeeded" _az_ANSI_COLOR_RESET "\n", #c); \
    } \
    else \
    { \
      fprintf(stderr, "  - `%s`: " _az_ANSI_COLOR_RED "failed" _az_ANSI_COLOR_RESET "\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while (false);

#define TEST_EXPECT_SUCCESS(exp) TEST_ASSERT(az_succeeded(exp))

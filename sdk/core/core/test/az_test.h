// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <assert.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define _az_ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern int exit_code;

#define TEST_ASSERT(c) \
  do { \
    if (c) { \
      printf("  - `%s`: " ANSI_COLOR_GREEN "succeeded" ANSI_COLOR_RESET "\n", #c); \
    } else { \
      fprintf(stderr, "  - `%s`: " ANSI_COLOR_RED "failed" ANSI_COLOR_RESET "\n", #c); \
      assert(false); \
      exit_code = 1; \
    } \
  } while (false);

#define TEST_EXPECT_SUCCESS(exp) TEST_ASSERT(az_succeeded(exp))

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <az/exampleshortname/operations.h>
#include <stdint.h>
int32_t az_exampleshortname_operation(int32_t a, int32_t b) {
  if (a == 0) {
    return b;
  } else {
    return a;
  }
}

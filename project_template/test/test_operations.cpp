// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <az/exampleshortname/operations.h>
#include <catch2/catch.hpp>

TEST_CASE("operations test", "[operations]") {
  REQUIRE(az_exampleshortname_add_two_numbers(1, 1) == 2);
}

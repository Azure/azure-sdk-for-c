// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// This file is needed to work around GCC-specific link problem with HTTP transport library.
// It is to create an empty library that depends on the Azure SDK libraries,
// And then for the sample executable to depend on this library.
// An empty function in this file is needed because ISO C forbids empty translation units.

static inline void f() {}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

static void g();
static void f() { g(); }
static void g() { f(); }

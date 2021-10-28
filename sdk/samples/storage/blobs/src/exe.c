// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

void core_span_create();
void storage_upload();

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  core_span_create();
  storage_upload();
  return 0;
}

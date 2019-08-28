// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

int main(void)
{
    size_t failed_test_count = 0;
    RUN_TEST_SUITE(azstorage_blob_ut, failed_test_count);
    return (int)failed_test_count;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_TEST_LOG_H
#define _az_TEST_LOG_H

// This macro helps with the expected values for tests when verifying logging.

#ifdef AZ_NO_LOGGING
#define _az_BUILT_WITH_LOGGING(value_if_yes, value_if_no) (value_if_no)
#else
#define _az_BUILT_WITH_LOGGING(value_if_yes, value_if_no) (value_if_yes)
#endif // AZ_NO_LOGGING

#endif // _az_TEST_LOG_H

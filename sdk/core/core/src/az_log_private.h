// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_PRIVATE_H
#define _az_LOG_PRIVATE_H

#include <_az_cfg_prefix.h>

enum {
  _az_LOG_MSG_BUF_SIZE = 1024, // Size (in bytes) of the buffer to allocate on stack when building a
                               // log message => the maximum size of the log message.
};

#include <_az_cfg_suffix.h>

#endif

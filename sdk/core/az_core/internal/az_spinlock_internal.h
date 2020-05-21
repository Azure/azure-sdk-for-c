// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_SPINLOCK_INTERNAL_H
#define _az_SPINLOCK_INTERNAL_H

#include <_az_spinlock.h>

#include <_az_cfg_prefix.h>

void _az_spinlock_enter_writer(_az_spinlock* lock);
void _az_spinlock_exit_writer(_az_spinlock* lock);

void _az_spinlock_enter_reader(_az_spinlock* lock);
void _az_spinlock_exit_reader(_az_spinlock* lock);

#include <_az_cfg_suffix.h>

#endif // _az_SPINLOCK_INTERNAL_H

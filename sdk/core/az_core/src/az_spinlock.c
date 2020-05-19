// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_platform.h>
#include <az_spinlock_internal.h>

#include <stdbool.h>

#include <_az_cfg.h>

// These _az_MASK symbols assume that a pointer is at least 32-bits wide
// The top 2 bits are reserved for writers; the bottom 30 bits are for readers
enum
{
  _az_SPINLOCK_WRITER_BIT = (int)0x80000000,
  _az_SPINLOCK_WRITER_WAITING_BIT = 0x40000000,
  _az_SPINLOCK_WRITER_BITS = _az_SPINLOCK_WRITER_BIT | _az_SPINLOCK_WRITER_WAITING_BIT,
  _az_SPINLOCK_READER_BITS = ~_az_SPINLOCK_WRITER_BITS,
};

void _az_spinlock_enter_writer(_az_spinlock* lock)
{
  while (true)
  {
    uint32_t const state = (uint32_t)lock->_internal.state;

    // If the lock is free (0) or (no readers and a writer is waiting): try to set the writer bit
    // If successful, return
    if ((state == 0 || state == _az_SPINLOCK_WRITER_WAITING_BIT)
        && az_platform_atomic_compare_exchange(
            &lock->_internal.state, state, (uint32_t)_az_SPINLOCK_WRITER_BIT))
    {
      return;
    }

    // Otherwise, if no writer is waiting, try to set that a writer is waiting
    // If we fail, we'll spin around and try again
    if ((state & _az_SPINLOCK_WRITER_WAITING_BIT) == 0
        && az_platform_atomic_compare_exchange(
            &lock->_internal.state, state, state | _az_SPINLOCK_WRITER_WAITING_BIT))
    {
      return;
    }
  }
}

void _az_spinlock_exit_writer(_az_spinlock* lock)
{
  // Set the state to 0 (free the lock)
  // But, keep the writer waiting bit (if set) to prevent a reader from getting in.
  lock->_internal.state = lock->_internal.state & _az_SPINLOCK_WRITER_WAITING_BIT;
}

void _az_spinlock_enter_reader(_az_spinlock* lock)
{
  while (true)
  {
    uint32_t const state = (uint32_t)lock->_internal.state;

    if ((state & (uint32_t)_az_SPINLOCK_WRITER_BITS) == 0)
    {
      // If no writer, try to add a reader
      uint32_t const incremented_state = state + 1;
      if (az_platform_atomic_compare_exchange(&lock->_internal.state, state, incremented_state))
      {
        return;
      }
    }

    // Else, spin
  }
}

void _az_spinlock_exit_reader(_az_spinlock* lock)
{
  while (true)
  {
    // Validate we hold a read lock.
    uintptr_t const state = (uint32_t)lock->_internal.state;

    // Update the lock by subtracting 1 reader; preserve the writer waiting bit (if any).
    if (az_platform_atomic_compare_exchange(
            &lock->_internal.state,
            state,
            ((state & _az_SPINLOCK_READER_BITS) - 1) | (state & _az_SPINLOCK_WRITER_WAITING_BIT)))
    {
      return;
    }
  }
}

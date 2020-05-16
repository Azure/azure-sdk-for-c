// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_config_internal.h>
#include <az_platform.h>

#include <time.h>

#include <unistd.h>

#include <_az_cfg.h>

AZ_NODISCARD int64_t az_platform_clock_msec()
{
  return (int64_t)((clock() / CLOCKS_PER_SEC) * _az_TIME_MILLISECONDS_PER_SECOND);
}

void az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep((useconds_t)milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
}

AZ_NODISCARD bool az_platform_atomic_compare_exchange(
    void* volatile* obj,
    void* expected,
    void* desired)
{
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1))
  // __GNUC__ being defined does not mean that the compiler being used is necessarily GCC, but it
  // means that the compiler supports GNU extensions to the C language. Clang supports it, Intel
  // likely does. __sync_bool_compare_and_swap is one of these extensions, supported since the
  // version 4.1.
  // https://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/C-Extensions.html
  // https://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html#Atomic-Builtins

  return __sync_bool_compare_and_swap(obj, expected, desired);
#else
  return AZ_ERROR_NOT_IMPLEMENTED;
#endif // __GNUC__ >= 4.1
}

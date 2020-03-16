// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file _az_cfg_suffix.h
 *
 * @brief Close "extern C" and restore warnings state.
 */

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) // !_MSC_VER
#pragma GCC diagnostic pop
#elif defined(__clang__) // !_MSC_VER !__GNUC__
#pragma clang diagnostic pop
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_GCOV_H__
#define __UK_GCOV_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Dump the coverage information using the selected method
 * @return 0 on success, error code otherwise
 */
int ukgcov_dump_info(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UK_GCOV_H__ */

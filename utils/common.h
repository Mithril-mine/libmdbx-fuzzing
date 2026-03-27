/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#ifndef _UTILS_COMMON_H
# define _UTILS_COMMON_H

#include <stddef.h>
#include <stdint.h>

int mkdir_p(const char *path);
int remove_if_exists(const char *path);
int write_file(const char *path, const void *buf, size_t len);

#endif /* _UTILS_COMMON_H */

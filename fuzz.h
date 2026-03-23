/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#ifndef _FUZZ_H
# define _FUZZ_H

#include <mdbx.h>

typedef struct      dbi_mode_desc
{
    const char      *seed_name;
    const char      *dbi_name;
    MDBX_db_flags_t flags;
} dbi_mode_desc_t;

extern const dbi_mode_desc_t g_modes[];

void logger(MDBX_log_level_t level, const char *function, int line,
        const char *fmt, va_list args);

#endif

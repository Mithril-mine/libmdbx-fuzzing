/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#include <fuzz.h>

const struct dbi_mode_desc g_modes[] = {
    { "mode0_default",  NULL,       MDBX_DB_DEFAULTS },
    { "mode1_named",    "named",    MDBX_DB_DEFAULTS },
    { "mode2_rev",      "rev",      MDBX_REVERSEKEY },
    { "mode3_dup",      "dup",      MDBX_DUPSORT },
    { "mode4_dupfixed", "dupfixed", MDBX_DUPSORT | MDBX_DUPFIXED },
    { "mode5_intkey",   "intkey",   MDBX_INTEGERKEY },
    { "mode6_intdup",   "intdup",   MDBX_DUPSORT | MDBX_DUPFIXED | MDBX_INTEGERDUP },
    { "mode7_revdup",   "revdup",   MDBX_DUPSORT | MDBX_REVERSEDUP },
};

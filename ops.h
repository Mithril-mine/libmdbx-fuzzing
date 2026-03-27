/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#ifndef _OPS_H
# define _OPS_H

#include <fuzz_api.pb.h>
#include <fuzz.h>

/*
 * Op.op_case() returns the proto field number of the active oneof variant
 * (1-13), matching OP_* + 1.  g_ops[0] is NULL (Op::OP_NOT_SET).
 */
typedef void (*op_fn_t)(fuzz_ctx_t *, const fuzz_api::Op &);

extern const op_fn_t    g_ops[];
extern const size_t     g_ops_count;

#endif

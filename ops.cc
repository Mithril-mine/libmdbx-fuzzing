/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#include <ops.h>

/* ------------------------------------------------------- transaction helper */

static void         ctx_cycle_txn(fuzz_ctx_t *ctx, int commit)
{
    if (ctx->cursor != NULL)
    {
        mdbx_cursor_close(ctx->cursor);
        ctx->cursor = NULL;
    }
    if (ctx->txn != NULL)
    {
        if (commit)
            (void)mdbx_txn_commit(ctx->txn);
        else
            mdbx_txn_abort(ctx->txn);
        ctx->txn = NULL;
    }
    if (ctx->env != NULL
        && mdbx_txn_begin(ctx->env, NULL, MDBX_TXN_READWRITE, &ctx->txn)
           == MDBX_SUCCESS)
        (void)mdbx_cursor_open(ctx->txn, ctx->dbi, &ctx->cursor);
}

/* ----------------------------------------------------------- op handlers  */

static void op_put(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    const auto  &m   = op.put();
    MDBX_val    key  = {(void *)m.key().data(), m.key().size()};
    MDBX_val    val  = {(void *)m.val().data(), m.val().size()};

    if (ctx->txn)
        (void)mdbx_put(ctx->txn, ctx->dbi, &key, &val, MDBX_UPSERT);
}

static void op_get(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    const auto  &m   = op.get();
    MDBX_val    key  = {(void *)m.key().data(), m.key().size()};
    MDBX_val    data = {};

    if (ctx->txn)
        (void)mdbx_get(ctx->txn, ctx->dbi, &key, &data);
}

static void op_del(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    const auto  &m   = op.del();
    MDBX_val    key  = {(void *)m.key().data(), m.key().size()};

    if (ctx->txn)
        (void)mdbx_del(ctx->txn, ctx->dbi, &key, NULL);
}

static void op_cursor_first(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    MDBX_val key = {}, data = {};

    (void)op;
    if (ctx->cursor)
        (void)mdbx_cursor_get(ctx->cursor, &key, &data, MDBX_FIRST);
}

static void op_cursor_next(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    MDBX_val key = {}, data = {};

    (void)op;
    if (ctx->cursor)
        (void)mdbx_cursor_get(ctx->cursor, &key, &data, MDBX_NEXT);
}

static void op_cursor_prev(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    MDBX_val key = {}, data = {};

    (void)op;
    if (ctx->cursor)
        (void)mdbx_cursor_get(ctx->cursor, &key, &data, MDBX_PREV);
}

static void op_cursor_last(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    MDBX_val key = {}, data = {};

    (void)op;
    if (ctx->cursor)
        (void)mdbx_cursor_get(ctx->cursor, &key, &data, MDBX_LAST);
}

static void op_cursor_set(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    const auto  &m   = op.cursor_set();
    MDBX_val    key  = {(void *)m.key().data(), m.key().size()};
    MDBX_val    data = {};

    if (ctx->cursor)
        (void)mdbx_cursor_get(ctx->cursor, &key, &data, MDBX_SET);
}

static void op_txn_commit(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    (void)op;
    ctx_cycle_txn(ctx, 1);
}

static void op_txn_abort(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    (void)op;
    ctx_cycle_txn(ctx, 0);
}

static void op_cursor_put(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    const auto  &m   = op.cursor_put();
    MDBX_val    key  = {(void *)m.key().data(), m.key().size()};
    MDBX_val    val  = {(void *)m.val().data(), m.val().size()};

    if (ctx->cursor)
        (void)mdbx_cursor_put(ctx->cursor, &key, &val, MDBX_UPSERT);
}

static void op_cursor_del(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    (void)op;
    if (ctx->cursor)
        (void)mdbx_cursor_del(ctx->cursor, MDBX_CURRENT);
}

static void op_cursor_del_alldups(fuzz_ctx_t *ctx, const fuzz_api::Op &op)
{
    (void)op;
    if (ctx->cursor)
        (void)mdbx_cursor_del(ctx->cursor, MDBX_ALLDUPS);
}

/* ------------------------------------------------------------- dispatch table
 * Indexed by Op::op_case() (proto field number, 1-13).
 * g_ops[0] is NULL — Op::OP_NOT_SET. */

const op_fn_t   g_ops[] = {
    /* 0  OP_NOT_SET           */ NULL,
    /* 1  put                  */ op_put,
    /* 2  get                  */ op_get,
    /* 3  del                  */ op_del,
    /* 4  cursor_first         */ op_cursor_first,
    /* 5  cursor_next          */ op_cursor_next,
    /* 6  cursor_prev          */ op_cursor_prev,
    /* 7  cursor_last          */ op_cursor_last,
    /* 8  cursor_set           */ op_cursor_set,
    /* 9  txn_commit           */ op_txn_commit,
    /* 10 txn_abort            */ op_txn_abort,
    /* 11 cursor_put           */ op_cursor_put,
    /* 12 cursor_del           */ op_cursor_del,
    /* 13 cursor_del_alldups   */ op_cursor_del_alldups,
};

const size_t    g_ops_count = sizeof(g_ops) / sizeof(g_ops[0]);

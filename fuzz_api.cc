/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#include <ops.h>

static void         fuzz_logger(MDBX_log_level_t level, const char *function,
                                int line, const char *fmt,
                                va_list args) noexcept
{
    logger(level, function, line, fmt, args);
}

static void         cleanup_dir(const char *dir, const char *db_path,
                                const char *lock_path)
{
    if (db_path)
        unlink(db_path);
    if (lock_path)
        unlink(lock_path);
    if (dir)
        rmdir(dir);
}

static int          ctx_open(fuzz_ctx_t *ctx, const char *workdir, uint8_t mode)
{
    MDBX_db_flags_t dbi_flags;
    const char      *dbi_name;
    int             err = -1;

    memset(ctx, 0, sizeof(*ctx));
    if (mdbx_env_create(&ctx->env) == MDBX_SUCCESS)
    {
        (void)mdbx_env_set_maxdbs(ctx->env, 4);
        /* Cap map size: prevents OOM from large geometry writes.
         * 64 MB is consistent with fuzz_raw_db_format. */
        (void)mdbx_env_set_geometry(ctx->env, 0, 0,
                                    64 * 1024 * 1024, -1, -1, -1);
        if (mdbx_env_open(ctx->env, workdir, MDBX_ENV_DEFAULTS, 0664)
            == MDBX_SUCCESS)
        {
            if (mdbx_txn_begin(ctx->env, NULL, MDBX_TXN_READWRITE, &ctx->txn)
                == MDBX_SUCCESS)
            {
                dbi_flags = g_modes[mode % 8].flags | MDBX_CREATE;
                dbi_name  = g_modes[mode % 8].dbi_name;
                if (mdbx_dbi_open(ctx->txn, dbi_name, dbi_flags, &ctx->dbi)
                    == MDBX_SUCCESS)
                {
                    /* cursor failure is non-fatal; op handlers check NULL */
                    (void)mdbx_cursor_open(ctx->txn, ctx->dbi, &ctx->cursor);
                    err = 0;
                }
            }
        }
    }

    return (err);
}

static void         ctx_close(fuzz_ctx_t *ctx)
{
    if (ctx->cursor != NULL)
        mdbx_cursor_close(ctx->cursor);
    if (ctx->txn != NULL)
        mdbx_txn_abort(ctx->txn);
    if (ctx->env != NULL)
        mdbx_env_close(ctx->env);
}

DEFINE_PROTO_FUZZER(const fuzz_api::FuzzSession &session)
{
    char        workdir[]  = "/tmp/libmdbx-api-XXXXXX";
    char        db_path[PATH_MAX]   = {};
    char        lock_path[PATH_MAX] = {};
    fuzz_ctx_t  ctx;

#ifdef MDBX_FUZZ_DEBUG
    mdbx_setup_debug(
        MDBX_LOG_VERBOSE, MDBX_DBG_DUMP | MDBX_DBG_ASSERT | MDBX_DBG_AUDIT
      | MDBX_DBG_LEGACY_OVERLAP | MDBX_DBG_DONT_UPGRADE, fuzz_logger);
#endif

    if (mkdtemp(workdir))
    {
        if (snprintf(db_path, sizeof(db_path), "%s/mdbx.dat", workdir)
            < (int)sizeof(db_path))
        {
            if (snprintf(lock_path, sizeof(lock_path), "%s/mdbx.lck", workdir)
                < (int)sizeof(lock_path))
            {
                ctx_open(&ctx, workdir, (uint8_t)session.mode());
                for (const auto &op : session.ops())
                {
                    int c = op.op_case();

                    if (c > 0 && c < (int)g_ops_count && g_ops[c] != NULL)
                        g_ops[c](&ctx, op);
                }
                ctx_close(&ctx);
            }
        }
        cleanup_dir(workdir, db_path, lock_path);
    }
}

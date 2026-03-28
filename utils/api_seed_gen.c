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

#include <fuzz.h>
#include <utils/common.h>

static void         pb_bytes(FILE *f, const void *data, size_t len)
{
    const uint8_t   *p = (const uint8_t *)data;

    for (size_t i = 0; i < len; ++i)
    {
        if (p[i] >= 0x20 && p[i] < 0x7f && p[i] != '"' && p[i] != '\\')
            fputc((int)p[i], f);
        else
            fprintf(f, "\\x%02x", p[i]);
    }
}

static void     op_bare(FILE *f, const char *name)
{
    fprintf(f, "ops { %s {} }\n", name);
}

static void     op_put_raw(FILE *f,
                            const void *k, size_t kl,
                            const void *v, size_t vl)
{
    fprintf(f, "ops { put { key: \"");
    pb_bytes(f, k, kl);
    fprintf(f, "\" val: \"");
    pb_bytes(f, v, vl);
    fprintf(f, "\" } }\n");
}

static void     op_put(FILE *f, const char *k, const char *v)
{
    op_put_raw(f, k, strlen(k), v, strlen(v));
}

static void     op_get(FILE *f, const char *k)
{
    fprintf(f, "ops { get { key: \"");
    pb_bytes(f, k, strlen(k));
    fprintf(f, "\" } }\n");
}

static void     op_del(FILE *f, const char *k)
{
    fprintf(f, "ops { del { key: \"");
    pb_bytes(f, k, strlen(k));
    fprintf(f, "\" } }\n");
}

static void     op_cursor_set(FILE *f, const void *k, size_t kl)
{
    fprintf(f, "ops { cursor_set { key: \"");
    pb_bytes(f, k, kl);
    fprintf(f, "\" } }\n");
}

static void     op_cursor_put(FILE *f,
                               const void *k, size_t kl,
                               const void *v, size_t vl)
{
    fprintf(f, "ops { cursor_put { key: \"");
    pb_bytes(f, k, kl);
    fprintf(f, "\" val: \"");
    pb_bytes(f, v, vl);
    fprintf(f, "\" } }\n");
}

static FILE     *open_seed(const char *path, unsigned mode)
{
    FILE        *f = fopen(path, "w");

    if (f)
        fprintf(f, "mode: %u\n", mode);
    else
        fprintf(stderr, "open_seed(fopen): opening %s: %m\n", path);
    return (f);
}

static int      close_seed(FILE *f)
{
    if (fclose(f))
    {
        fprintf(stderr, "close_seed(fopen): %m\n");
        return (-1);
    }

    return (0);
}

static void     tail_commit_scan_abort(FILE *f)
{
    op_bare(f, "txn_commit");
    op_bare(f, "cursor_first");
    op_bare(f, "cursor_next");
    op_bare(f, "cursor_prev");
    op_bare(f, "cursor_last");
    op_bare(f, "txn_abort");
    op_bare(f, "cursor_first");
}

static int      seed_mode0(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[0].seed_name);
    if ((f = open_seed(path, 0)) != NULL)
    {
        for (int i = 0; i < 8; ++i)
        {
            char k[16], v[16];
            snprintf(k, sizeof(k), "k%04d", i);
            snprintf(v, sizeof(v), "v%04d", i);
            op_put(f, k, v);
        }
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_last");
        op_bare(f, "txn_commit");
        op_put(f, "k0100", "v0100");
        op_put(f, "k0101", "v0101");
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_del");
        op_bare(f, "cursor_last");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_mode0_get_roundtrip", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_put(f, "hello", "world");
        op_get(f, "hello");
        op_get(f, "missing");
        op_del(f, "hello");
        op_get(f, "hello");
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_mode0_cursor_put", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_cursor_put(f, "cpk1", 4, "cpv1", 4);
        op_cursor_put(f, "cpk2", 4, "cpv2", 4);
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_prev");
        op_put(f, "cpk1", "cpv1-updated");
        op_get(f, "cpk1");
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    return (err);
}

static int      seed_mode1(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[1].seed_name);
    if ((f = open_seed(path, 1)) != NULL)
    {
        for (int i = 0; i < 8; ++i)
        {
            char k[24], v[24];
            snprintf(k, sizeof(k), "named-key-%02d", i);
            snprintf(v, sizeof(v), "named-val-%02d", i);
            op_put(f, k, v);
        }
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_last");
        op_bare(f, "txn_commit");
        op_put(f, "named-key-10", "named-val-10");
        op_put(f, "named-key-11", "named-val-11");
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_del");
        op_bare(f, "cursor_last");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    return (err);
}

static int      seed_mode2(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[2].seed_name);
    if ((f = open_seed(path, 2)) != NULL)
    {
        op_put(f, "abc",  "rev-val-0");
        op_put(f, "abcd", "rev-val-1");
        op_put(f, "za",   "rev-val-2");
        op_put(f, "zz",   "rev-val-3");
        op_put(f, "zzz",  "rev-val-4");
        op_put(f, "yx",   "rev-val-5");
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_del");
        op_bare(f, "cursor_last");
        op_bare(f, "cursor_prev");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_rev_prefix", outdir);
    if ((f = open_seed(path, 2)) != NULL)
    {
        op_put(f, "fooA", "1");
        op_put(f, "fooB", "2");
        op_put(f, "fooC", "3");
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_last");
        if (close_seed(f) != 0)
            err = 1;
    } else err = 1;

    return (err);
}

static int      seed_mode3(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[3].seed_name);
    if ((f = open_seed(path, 3)) != NULL)
    {
        op_put(f, "dupkey", "one");
        op_put(f, "dupkey", "two");
        op_put(f, "dupkey", "three");
        op_put(f, "other",  "x");
        op_cursor_set(f, "dupkey", 6);
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_del");
        op_cursor_set(f, "dupkey", 6);
        op_bare(f, "cursor_del_alldups");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_dup_traverse", outdir);
    if ((f = open_seed(path, 3)) != NULL)
    {
        op_put(f, "k", "alpha");
        op_put(f, "k", "beta");
        op_put(f, "k", "gamma");
        op_cursor_set(f, "k", 1);
        op_bare(f, "cursor_last");
        op_bare(f, "cursor_prev");
        op_bare(f, "cursor_prev");
        op_bare(f, "cursor_next");
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    return (err);
}

static int      seed_mode4(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    int         err = 0;
    const char  *dupkey = "dupkey";

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[4].seed_name);
    if ((f = open_seed(path, 4)) != NULL)
    {
        op_put(f, dupkey, "AAAA");
        op_put(f, dupkey, "BBBB");
        op_put(f, dupkey, "CCCC");
        op_put(f, "other",  "DDDD");
        op_cursor_set(f, dupkey, strlen(dupkey));
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_del");
        op_cursor_set(f, dupkey, strlen(dupkey));
        op_bare(f, "cursor_del_alldups");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_dupfixed_many", outdir);
    if ((f = open_seed(path, 4)) != NULL)
    {
        const char *dups[] = { "0000", "1111", "2222", "3333",
                               "4444", "5555", "6666", "7777" };
        for (int i = 0; i < 8; ++i)
            op_put(f, "key", dups[i]);
        op_cursor_set(f, "key", 3);
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_last");
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    return (err);
}

static int      seed_mode5(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    uint32_t    k;
    char        v[32];
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[5].seed_name);
    if ((f = open_seed(path, 5)) != NULL)
    {
        for (uint32_t i = 1; i <= 8; ++i)
        {
            int vl = snprintf(v, sizeof(v), "ival-%u", i);
            op_put_raw(f, &i, sizeof(i), v, (size_t)vl);
        }
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_last");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0)
            err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_intkey_edge", outdir);
    if ((f = open_seed(path, 5)) != NULL)
    {
        k = 0;
        op_put_raw(f, &k, 4, "zero", 4);
        k = 1;
        op_put_raw(f, &k, 4, "one",  3);
        k = UINT32_MAX;
        op_put_raw(f, &k, 4, "max",  3);
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_last");
        op_bare(f, "cursor_prev");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    return (err);
}

static int      seed_mode6(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    uint32_t    vals[] = { 10, 20, 30, 40 };
    const char  *k = "dupint";
    uint32_t    ev;
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[6].seed_name);
    if ((f = open_seed(path, 6)) != NULL)
    {
        for (int i = 0; i < 4; ++i)
            op_put_raw(f, k, strlen(k), &vals[i], sizeof(vals[i]));
        op_cursor_set(f, k, strlen(k));
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_del");
        op_cursor_set(f, k, strlen(k));
        op_bare(f, "cursor_del_alldups");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0)
            err = 1;
    } else err = 1;

    snprintf(path, sizeof(path), "%s/api_intdup_edge", outdir);
    if ((f = open_seed(path, 6)) != NULL)
    {
        ev = 0;
        op_put_raw(f, k, strlen(k), &ev, 4);
        ev = 1;
        op_put_raw(f, k, strlen(k), &ev, 4);
        ev = UINT32_MAX;
        op_put_raw(f, k, strlen(k), &ev, 4);
        op_cursor_set(f, k, strlen(k));
        op_bare(f, "cursor_last");
        op_bare(f, "cursor_prev");
        op_bare(f, "cursor_first");
        if (close_seed(f) != 0) err = 1;
    } else err = 1;

    return (err);
}

static int      seed_mode7(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    const char  *k = "duprev";
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_%s", outdir, g_modes[7].seed_name);
    if ((f = open_seed(path, 7)) != NULL)
    {
        op_put(f, k, "abc");
        op_put(f, k, "bcd");
        op_put(f, k, "zzz");
        op_cursor_set(f, k, strlen(k));
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_del");
        op_cursor_set(f, k, strlen(k));
        op_bare(f, "cursor_del_alldups");
        tail_commit_scan_abort(f);
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_revdup_prefix", outdir);
    if ((f = open_seed(path, 7)) != NULL)
    {
        op_put(f, k, "fooA");
        op_put(f, k, "fooB");
        op_put(f, k, "fooC");
        op_cursor_set(f, k, strlen(k));
        op_bare(f, "cursor_last");
        op_bare(f, "cursor_prev");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    return (err);
}

static int      seed_edge_cases(const char *outdir)
{
    char        path[PATH_MAX];
    FILE        *f;
    int         err = 0;

    snprintf(path, sizeof(path), "%s/api_empty_session", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_empty_db_cursors", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_last");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_prev");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_txn_cycles", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_bare(f, "txn_commit");
        op_bare(f, "txn_abort");
        op_bare(f, "txn_commit");
        op_bare(f, "txn_abort");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_zero_kv", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_put_raw(f, "", 0, "", 0);
        op_bare(f, "cursor_first");
        op_get(f, "");
        op_del(f, "");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_single_byte", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        const uint8_t b0 = 0x00, b1 = 0x01, bff = 0xff;
        op_put_raw(f, &b0,  1, &bff, 1);
        op_put_raw(f, &b1,  1, &b1,  1);
        op_put_raw(f, &bff, 1, &b0,  1);
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_next");
        op_bare(f, "cursor_last");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_commit_visibility", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_put(f, "before", "commit");
        op_bare(f, "txn_commit");
        op_get(f, "before");
        op_put(f, "after", "abort");
        op_bare(f, "txn_abort");
        op_get(f, "after");
        op_get(f, "before");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_large_value", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        char bigval[1024];
        memset(bigval, 'A', sizeof(bigval));
        op_put_raw(f, "bigkey", 6, bigval, sizeof(bigval));
        op_get(f, "bigkey");
        op_del(f, "bigkey");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_cursor_put_upsert", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_put(f, "x", "old");
        op_cursor_set(f, "x", 1);
        op_cursor_put(f, "x", 1, "new", 3);
        op_get(f, "x");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_del_missing", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_del(f, "ghost");
        op_put(f, "real", "v");
        op_del(f, "ghost");
        op_del(f, "real");
        op_del(f, "real");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    snprintf(path, sizeof(path), "%s/api_cursor_del_no_pos", outdir);
    if ((f = open_seed(path, 0)) != NULL)
    {
        op_bare(f, "cursor_del");
        op_put(f, "k", "v");
        op_bare(f, "cursor_first");
        op_bare(f, "cursor_del");
        op_bare(f, "cursor_del");
        if (close_seed(f) != 0)
            err = 1;
    } else
        err = 1;

    return (err);
}

int             main(int argc, char **argv)
{
    const char  *outdir = (argc >= 2) ? argv[1] : "corpus_api";
    int         err = 0;

    if (mkdir_p(outdir) != 0)
        return (1);

    if (seed_mode0(outdir) != 0) err = 1;
    if (seed_mode1(outdir) != 0) err = 1;
    if (seed_mode2(outdir) != 0) err = 1;
    if (seed_mode3(outdir) != 0) err = 1;
    if (seed_mode4(outdir) != 0) err = 1;
    if (seed_mode5(outdir) != 0) err = 1;
    if (seed_mode6(outdir) != 0) err = 1;
    if (seed_mode7(outdir) != 0) err = 1;
    if (seed_edge_cases(outdir) != 0) err = 1;

    if (err == 0)
        printf("API seed corpus written to %s\n", outdir);

    return (err);
}

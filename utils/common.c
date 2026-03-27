/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Chloe Cano
 *
 * OSS-Fuzz harness for libmdbx <https://libmdbx.dqdkfa.ru>
 * libmdbx is Copyright (c) its respective authors, Apache 2.0.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

int           mkdir_p(const char *path)
{
  int         err = 0;
  struct stat st;

  if (stat(path, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      fprintf(stderr, "mkdir_p: %s exists but is not a directory\n", path);
      err = -1;
    }
  } else {
    if (mkdir(path, 0755) != 0) {
      fprintf(stderr, "mkdir_p(mkdir): %s: %m\n", path);
      err = -1;
    }
  }

  return (err);
}

int           remove_if_exists(const char *path)
{
  int         err = 0;

  if (unlink(path) != 0) {
    if (errno != ENOENT) {
      fprintf(stderr, "unlink: %s: %m\n", path);
      err = -1;
    }
  }

  return (err);
}

int             write_file(const char *path, const void *buf, size_t len)
{
  int           fd = -1;
  int           err = -1;
  ssize_t       n = 0;
  size_t        off = 0;
  const uint8_t *p = (const uint8_t *)buf;

  if ((fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644)) >= 0) {
    while (off < len) {
      if ((n = write(fd, p + off, len - off)) < 0) {
        fprintf(stderr, "write_file(write): %s: %m\n", path);
      } else {
        off += (size_t)n;
      }
      if (n < 0)
        break;
    }
    if (off == len)
      err = 0;
    if (close(fd) != 0)
      fprintf(stderr, "write_file(close): %s: %m\n", path);
  } else {
    fprintf(stderr, "write_file(open): %s: %m\n", path);
  }

  return (err);
}

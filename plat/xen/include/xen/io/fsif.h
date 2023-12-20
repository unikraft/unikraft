/******************************************************************************
 * fsif.h
 * 
 * Interface to FS level split device drivers.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2007, Grzegorz Milos, <gm281@cam.ac.uk>.
 */

#ifndef __XEN_PUBLIC_IO_FSIF_H__
#define __XEN_PUBLIC_IO_FSIF_H__

#include "ring.h"
#include "../grant_table.h"

#define REQ_FILE_OPEN        1
#define REQ_FILE_CLOSE       2
#define REQ_FILE_READ        3
#define REQ_FILE_WRITE       4
#define REQ_STAT             5
#define REQ_FILE_TRUNCATE    6
#define REQ_REMOVE           7
#define REQ_RENAME           8
#define REQ_CREATE           9
#define REQ_DIR_LIST        10
#define REQ_CHMOD           11
#define REQ_FS_SPACE        12
#define REQ_FILE_SYNC       13

struct fsif_open_request {
    grant_ref_t gref;
};

struct fsif_close_request {
    __u32 fd;
};

struct fsif_read_request {
    __u32 fd;
    __s32 pad;
    __u64 len;
    __u64 offset;
    grant_ref_t grefs[1];  /* Variable length */
};

struct fsif_write_request {
    __u32 fd;
    __s32 pad;
    __u64 len;
    __u64 offset;
    grant_ref_t grefs[1];  /* Variable length */
};

struct fsif_stat_request {
    __u32 fd;
};

/* This structure is a copy of some fields from stat structure, returned
 * via the ring. */
struct fsif_stat_response {
    __s32  stat_mode;
    __u32 stat_uid;
    __u32 stat_gid;
    __s32  stat_ret;
    __s64  stat_size;
    __s64  stat_atime;
    __s64  stat_mtime;
    __s64  stat_ctime;
};

struct fsif_truncate_request {
    __u32 fd;
    __s32 pad;
    __s64 length;
};

struct fsif_remove_request {
    grant_ref_t gref;
};

struct fsif_rename_request {
    __u16 old_name_offset;
    __u16 new_name_offset;
    grant_ref_t gref;
};

struct fsif_create_request {
    __s8 directory;
    __s8 pad;
    __s16 pad2;
    __s32 mode;
    grant_ref_t gref;
};

struct fsif_list_request {
    __u32 offset;
    grant_ref_t gref;
};

#define NR_FILES_SHIFT  0
#define NR_FILES_SIZE   16   /* 16 bits for the number of files mask */
#define NR_FILES_MASK   (((1ULL << NR_FILES_SIZE) - 1) << NR_FILES_SHIFT)
#define ERROR_SIZE      32   /* 32 bits for the error mask */
#define ERROR_SHIFT     (NR_FILES_SIZE + NR_FILES_SHIFT)
#define ERROR_MASK      (((1ULL << ERROR_SIZE) - 1) << ERROR_SHIFT)
#define HAS_MORE_SHIFT  (ERROR_SHIFT + ERROR_SIZE)    
#define HAS_MORE_FLAG   (1ULL << HAS_MORE_SHIFT)

struct fsif_chmod_request {
    __u32 fd;
    __s32 mode;
};

struct fsif_space_request {
    grant_ref_t gref;
};

struct fsif_sync_request {
    __u32 fd;
};


/* FS operation request */
struct fsif_request {
    __u8 type;                 /* Type of the request                  */
    __u8 pad;
    __u16 id;                  /* Request ID, copied to the response   */
    __u32 pad2;
    union {
        struct fsif_open_request     fopen;
        struct fsif_close_request    fclose;
        struct fsif_read_request     fread;
        struct fsif_write_request    fwrite;
        struct fsif_stat_request     fstat;
        struct fsif_truncate_request ftruncate;
        struct fsif_remove_request   fremove;
        struct fsif_rename_request   frename;
        struct fsif_create_request   fcreate;
        struct fsif_list_request     flist;
        struct fsif_chmod_request    fchmod;
        struct fsif_space_request    fspace;
        struct fsif_sync_request     fsync;
    } u;
};
typedef struct fsif_request fsif_request_t;

/* FS operation response */
struct fsif_response {
    __u16 id;
    __u16 pad1;
    __u32 pad2;
    union {
        __u64 ret_val;
        struct fsif_stat_response fstat;
    } u;
};

typedef struct fsif_response fsif_response_t;

#define FSIF_RING_ENTRY_SIZE   64

#define FSIF_NR_READ_GNTS  ((FSIF_RING_ENTRY_SIZE - sizeof(struct fsif_read_request)) /  \
                                sizeof(grant_ref_t) + 1)
#define FSIF_NR_WRITE_GNTS ((FSIF_RING_ENTRY_SIZE - sizeof(struct fsif_write_request)) / \
                                sizeof(grant_ref_t) + 1)

DEFINE_RING_TYPES(fsif, struct fsif_request, struct fsif_response);

#define STATE_INITIALISED     "init"
#define STATE_READY           "ready"
#define STATE_CLOSING         "closing"
#define STATE_CLOSED          "closed"


#endif

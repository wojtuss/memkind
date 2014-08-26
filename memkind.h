/*
 * Copyright (C) 2014 Intel Corperation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef memkind_include_h
#define memkind_include_h
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/*!
 *  \mainpage
 *
 *  The memkind library extends libnuma with the ability to
 *  categorize groups of numa nodes into different "kinds" of
 *  memory. It provides a low level interface for generating inputs to
 *  mbind() and mmap(), and a high level interface for heap
 *  management.  The heap management is implemented with an extension
 *  to the jemalloc library which dedicates "arenas" to each CPU and
 *  kind of memory.  To use memkind, jemalloc must be compiled with
 *  the --enable-memkind option.
 */

enum memkind_const {
    MEMKIND_MAX_KIND = 512,
    MEMKIND_NUM_BASE_KIND = 6,
    MEMKIND_ERROR_MESSAGE_SIZE = 128,
    MEMKIND_NAME_LENGTH = 64
};

enum memkind_error {
    MEMKIND_ERROR_UNAVAILABLE = -1,
    MEMKIND_ERROR_MBIND = -2,
    MEMKIND_ERROR_MEMALIGN = -3,
    MEMKIND_ERROR_MALLCTL = -4,
    MEMKIND_ERROR_MALLOC = -5,
    MEMKIND_ERROR_GETCPU = -6,
    MEMKIND_ERROR_PMTT = -7,
    MEMKIND_ERROR_TIEDISTANCE = -8,
    MEMKIND_ERROR_ALIGNMENT = -9,
    MEMKIND_ERROR_MALLOCX = -10,
    MEMKIND_ERROR_ENVIRON = -11,
    MEMKIND_ERROR_INVALID = -12,
    MEMKIND_ERROR_REPNAME = -13,
    MEMKIND_ERROR_TOOMANY = -14,
    MEMKIND_ERROR_PTHREAD = -15,
    MEMKIND_ERROR_RUNTIME = -255
};

enum memkind_base_partition {
    MEMKIND_PARTITION_DEFAULT = 0,
    MEMKIND_PARTITION_HBW = 1,
    MEMKIND_PARTITION_HBW_HUGETLB = 2,
    MEMKIND_PARTITION_HBW_PREFERRED = 3,
    MEMKIND_PARTITION_HBW_PREFERRED_HUGETLB = 4,
    MEMKIND_PARTITION_HUGETLB = 5
};

struct memkind_ops;

struct memkind {
    const struct memkind_ops *ops;
    int partition;
    char name[MEMKIND_NAME_LENGTH];
    pthread_once_t init_once;
    int arena_map_len;
    unsigned int *arena_map;
};

struct memkind_ops {
    int (* create)(struct memkind *kind, const struct memkind_ops *ops, const char *name);
    int (* destroy)(struct memkind *kind);
    void *(* malloc)(struct memkind *kind, size_t size);
    void *(* calloc)(struct memkind *kind, size_t num, size_t size);
    int (* posix_memalign)(struct memkind *kind, void **memptr, size_t alignment, size_t size);
    void *(* realloc)(struct memkind *kind, void *ptr, size_t size);
    void (* free)(struct memkind *kind, void *ptr);
    int (* is_available)(struct memkind *kind);
    int (* mbind)(struct memkind *kind, void *ptr, size_t len);
    int (* get_mmap_flags)(struct memkind *kind, int *flags);
    int (* get_mbind_mode)(struct memkind *kind, int *mode);
    int (* get_mbind_nodemask)(struct memkind *kind, unsigned long *nodemask, unsigned long maxnode);
    int (* get_arena) (struct memkind *kind, unsigned int *arena);
    int (* get_size) (struct memkind *kind, size_t *total, size_t *free);
    void (*init_once)(void);
};

typedef struct memkind * memkind_t;

extern memkind_t MEMKIND_DEFAULT;
extern memkind_t MEMKIND_HUGETLB;
extern memkind_t MEMKIND_HBW;
extern memkind_t MEMKIND_HBW_PREFERRED;
extern memkind_t MEMKIND_HBW_HUGETLB;
extern memkind_t MEMKIND_HBW_PREFERRED_HUGETLB;

/* Convert error number into an error message */
void memkind_error_message(int err, char *msg, size_t size);

/* Free all resources allocated by the library (must be last call to library by the process) */
int memkind_finalize(void);

/* KIND MANAGEMENT INTERFACE */

/* Create a new kind */
int memkind_create(const struct memkind_ops *ops, const char *name);

/* Query the number of kinds instantiated */
int memkind_get_num_kind(int *num_kind);

/* Get kind associated with a partition (index from 0 to num_kind - 1) */
int memkind_get_kind_by_partition(int partition, memkind_t *kind);

/* Get kind given the name of the kind */
int memkind_get_kind_by_name(const char *name, memkind_t *kind);

/* Get the amount in bytes of total and free memory of the NUMA nodes assciated with the kind */
int memkind_get_size(memkind_t kind, size_t *total, size_t *free);

/* returns 1 if memory kind is availble else 0 */
int memkind_is_available(memkind_t kind);

/* HEAP MANAGEMENT INTERFACE */

/* malloc from the numa nodes of the specified kind */
void *memkind_malloc(memkind_t kind, size_t size);

/* calloc from the numa nodes of the specified kind */
void *memkind_calloc(memkind_t kind, size_t num, size_t size);

/* posix_memalign from the numa nodes of the specified kind */
int memkind_posix_memalign(memkind_t kind, void **memptr, size_t alignment,
                            size_t size);

/* realloc from the numa nodes of the specified kind */
void *memkind_realloc(memkind_t kind, void *ptr, size_t size);

/* Free memory allocated with the memkind API */
void memkind_free(memkind_t kind, void *ptr);

/* ALLOCATOR CALLBACK FUNCTIONS */

/* returns 1 if memory kind associated with the partition is availble else 0 */
int memkind_partition_is_available(int partition);

/* get flags for call to mmap for the memory kind associated with the partition */
int memkind_partition_get_mmap_flags(int partition, int *flags);

/* mbind to the nearest numa node of the memory kind associated with the partition */
int memkind_partition_mbind(int partition, void *addr, size_t len);

#ifdef __cplusplus
}
#endif
#endif

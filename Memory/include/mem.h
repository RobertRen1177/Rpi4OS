#ifndef MEM_H
#define MEM_H
#include <stddef.h>
#include <stdint.h>
#include "include/page.h"

#define ALIGNMENT 16
#define RESEVERED HEAP 1024 * 1024 * 1
#define CHUNK_MAGIC 0xCA40CA40CA40CA40LL
#define ALIGN_UP(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1)) // formula I got of the web

/* --- Flag bits, packed into the low bits of size ---
 * Chunk sizes are always multiples of 16, so the
 * bottom 4 bits of size are always 0 in a clean size value (351!!!! Woahhhh so cool)
 * We steal the bottom 2 of those bits for flags.
*/
#define FLAG_ALLOCATED   (1UL << 0)
#define FLAG_PREV_INUSE  (1UL << 1)
#define SIZE_MASK        (~(uint64_t)(ALIGNMENT - 1))

// start of the free list
static header_t* free_list_head = NULL; 
static header_t* free_list_tail = NULL; 

/* Header
 * Present on EVERY chunk, allocated or free. It is exactly 16 bytes so that the
 * payload (which starts immediately after it) is 16-byte aligned. In this simple
 * design the second word is pure padding -- we spend 8 bytes per chunk to keep the
 * layout dead simple, instead of the glibc trick of sharing it with the neighbour.
*/
typedef struct block_header {
    uint64_t size;   /* chunk size (16-aligned) | FLAG_ALLOCATED | FLAG_PREV_INUSE */
    uint64_t security_key; // when freeing memory, if 8 byte pad does not equal our security key, it is an invalid free
} header_t;

/* Footer: only valid on FREE chunks. Stores this chunk's size so the chunk to the
 * RIGHT can walk back to this header during backward coalescing.
*/
typedef struct block_footer {
    uint64_t size;
} footer_t;

/* --- Derived size constants --- */
#define HEADER_SIZE     sizeof(header_t)                 /* 16 bytes: size + pad     */
#define FOOTER_SIZE     sizeof(footer_t)                 /* 8 bytes                  */
/* The free-list links (next/prev) live in the payload, valid only while free.
 * A free chunk must therefore be big enough for: header + next + prev + footer. */
#define FREE_LINKS_SIZE (2 * sizeof(void *))             /* next + prev = 16 bytes   */
#define OVERHEAD_FREE   (HEADER_SIZE + FREE_LINKS_SIZE + FOOTER_SIZE) /* 40 bytes     */

/*
 * Smallest possible chunk: has to be big enough to hold a full
 * free-block header + links + footer later, even if it's allocated right now
 * with a tiny payload. This is what stops free() from writing past
 * the end of a too-small chunk.
*/
#define MIN_CHUNK_SIZE ALIGN_UP(OVERHEAD_FREE)           /* 48 bytes */


// the entire chunk size including the header
static inline size_t chunk_size(header_t *h) {
    return h->size & SIZE_MASK;
}

// is the current chunk being used
static inline int is_allocated(header_t *h) {
    return (h->size & FLAG_ALLOCATED) != 0;
}

// is the previous chunk being used
static inline int prev_inuse(header_t *h) {
    return (h->size & FLAG_PREV_INUSE) != 0;
}

// set new size which should be including the header
static inline void set_size(header_t *h, size_t sz, int alloc_flag, int prev_flag) {
    h->size = (sz & SIZE_MASK) | (alloc_flag ? FLAG_ALLOCATED : 0)
                                | (prev_flag ? FLAG_PREV_INUSE : 0);
}

// returns a pointer to the payload
static inline void *chunk_to_mem(header_t *h) {
    return (void *)((char *)h + HEADER_SIZE);
}

// given the payload, returns the pointer to the header
static inline header_t *mem_to_chunk(void *mem) {
    return (header_t *)((char *)mem - HEADER_SIZE);
}

/* --- physical neighbour navigation --- */
static inline header_t *next_chunk(header_t *h) {
    return (header_t *)((char *)h + chunk_size(h));
}
static inline footer_t *footer_of(header_t *h) {
    return (footer_t *)((char *)h + chunk_size(h) - FOOTER_SIZE);
}

/* --- free-list links: stored in the payload, only valid while the chunk is FREE --- */
static inline header_t *get_next(header_t *h) {
    return ((header_t **)chunk_to_mem(h))[0];
}
static inline header_t *get_prev(header_t *h) {
    return ((header_t **)chunk_to_mem(h))[1];
}
static inline void set_next(header_t *h, header_t *n) {
    ((header_t **)chunk_to_mem(h))[0] = n;
}
static inline void set_prev(header_t *h, header_t *p) {
    ((header_t **)chunk_to_mem(h))[1] = p;
}
int remove_from_free_list(header_t* h);
header_t* chunk_out_required_size(header_t* chunk, uint64_t size);
int request_page();
void* kmalloc(int size);
int kfree(void* mem);
#endif

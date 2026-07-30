#ifndef MEMORY_PAGE_H
#define MEMORY_PAGE_H

#include <stdint.h>
#include <stddef.h>

// constants initalization
#define PAGE_SIZE 4096
#define MEMORY_SIZE 128 * 1024 * 1024   // total RAM in bytes; must be a multiple of PAGE_SIZE

// create the meta data struct for pages
struct page {
    uint8_t  is_allocated;
    uint8_t  is_kernel_page;
    uint32_t identifier;     // page number == index into the page array
    struct page* next;       // free-list link
};

// set up the page metadata array and the free list
void page_init(void);

// hand out one free 4 KB page; returns its physical address (zeroed), or NULL if none
void* allocate_page(void);

// return a page (by the address allocate_page gave you) to the free list
void free_page(void* ptr);

#endif // MEMORY_PAGE_H

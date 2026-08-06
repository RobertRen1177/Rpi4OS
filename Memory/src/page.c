#include "Memory/include/page.h"

// _end is planted by the linker just after the kernel image (.text/.rodata/.data/.bss).
// We only ever use its address, so the declared type doesn't matter.
extern uint8_t _end;

// base of the page metadata array (lives right after the kernel image)
static struct page* all_pages_array;

// head of the singly-linked list of free pages
static struct page* free_list_head;

// physical start of the contiguous region reserved for the kernel heap
static uintptr_t heap_start_addr;

void page_init(void) {
    // total memory divided by each page's size is how many pages we have
    uint32_t numpages = MEMORY_SIZE / PAGE_SIZE;

    // The metadata array starts right at _end (the linker script keeps _end 16-byte
    // aligned, so this address is safe to access with the MMU off).
    all_pages_array = (struct page*)&_end;

    // Pages consumed by firmware + kernel image + this metadata array. Round UP so the
    // page that holds the array's tail is reserved, not handed out.
    //   0x0     -> 0x80000 : firmware / boot region (and the stack grows down into here)
    //   0x80000 -> _end     : the kernel image
    //   _end    -> array_end: this metadata array itself
    uintptr_t array_end   = (uintptr_t)&_end + (uintptr_t)numpages * sizeof(struct page);
    uint32_t  array_pages = (uint32_t)((array_end + PAGE_SIZE - 1) / PAGE_SIZE);

    // The heap is a fixed contiguous region immediately after the array.
    heap_start_addr        = (uintptr_t)array_pages * PAGE_SIZE;
    uint32_t heap_pages    = HEAP_SIZE / PAGE_SIZE;

    // Reserve firmware + kernel + array + heap; none of these ever go on the free list.
    uint32_t reserved_pages = array_pages + heap_pages;
    if (reserved_pages > numpages) {
        reserved_pages = numpages;   // safety clamp: RAM too small (never happens in practice)
    }

    free_list_head = NULL;

    for (uint32_t i = 0; i < numpages; i++) {
        all_pages_array[i].identifier = i;
        all_pages_array[i].next = NULL;

        if (i < reserved_pages) {
            // kernel / firmware / metadata: reserved, never on the free list
            all_pages_array[i].is_allocated = 1;
            all_pages_array[i].is_kernel_page = 1;
        } else {
            // free page: push onto the head of the free list (O(1))
            all_pages_array[i].is_allocated = 0;
            all_pages_array[i].is_kernel_page = 0;
            all_pages_array[i].next = free_list_head;
            free_list_head = &all_pages_array[i];
        }
    }
}

void* allocate_page(void) {
    if (free_list_head == NULL) {
        return NULL;   // out of memory
    }

    // pop the head of the free list
    struct page* page = free_list_head;
    free_list_head = page->next;

    page->is_allocated = 1;
    page->next = NULL;

    void* page_mem = (void*)((uintptr_t)page->identifier * PAGE_SIZE);
    uint8_t* bytes = (uint8_t*)page_mem;
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        bytes[i] = 0;
    }

    return page_mem;
}

void free_page(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    // recover the metadata from the address: index == address / PAGE_SIZE
    uint32_t index = (uint32_t)((uintptr_t)ptr / PAGE_SIZE);
    struct page* page = &all_pages_array[index];

    if (!page->is_allocated) {
        return;   // guard against double-free corrupting the list
    }

    page->is_allocated = 0;
    page->next = free_list_head;
    free_list_head = page;
}

void* heap_region_start(void) {
    return (void*)heap_start_addr;
}

size_t heap_region_size(void) {
    return HEAP_SIZE;
}

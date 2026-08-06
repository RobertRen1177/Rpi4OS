#include "include/mem.h"

int init_heap() {
    // getter function getting us the start of the heap 
    header_t* start_of_heap = (header_t*) heap_region_start();
    
    //create header and footer of this guy
    set_size(start_of_heap, HEAP_SIZE - ALIGNMENT, 0, 0); // you will need ALIGNMENT sized header so the effective size is HEAP_SIZE- ALIGNMENT
    footer_of(start_of_heap)->size = chunk_size(start_of_heap);

    // a protective fence at the end of the heap so we cannot reclaim memory further than we are allowed
    header_t* fence = next_chunk(start_of_heap);
    set_size(fence, 0, 1, 0);


    // initalizing the free list
    set_prev(start_of_heap, NULL);
    set_next(start_of_heap, NULL);
    free_list_head = start_of_heap; 
    free_list_tail = start_of_heap; 
    return 1; 
}

void* kmalloc(int size) {
    if (size <= 0) {
        return NULL;
    }

    // Round the payload up to 16, and make sure the resulting chunk is at least
    // MIN_CHUNK_SIZE so it can still hold header + links + footer once it's freed.
    // (payload floor of MIN_CHUNK_SIZE - HEADER_SIZE guarantees HEADER_SIZE + payload
    //  >= MIN_CHUNK_SIZE). `need` is the total chunk size, header included.
    uint64_t payload = ALIGN_UP((uint64_t)size);
    if (payload < MIN_CHUNK_SIZE - HEADER_SIZE) {
        payload = MIN_CHUNK_SIZE - HEADER_SIZE;
    }
    uint64_t need = HEADER_SIZE + payload;

    // first-fit search through the whole free list (including the last node,
    // and safe when the list is empty)
    header_t* curr = free_list_head;
    while (curr != NULL) {
        if (chunk_size(curr) >= need) {
            break;
        }
        curr = get_next(curr);
    }
    if (curr == NULL) {
        return NULL;   // no chunk big enough
    }

    // take curr off the free list, carve out what we need (the remainder, if any,
    // goes back on the list), stamp the magic, and hand back the payload pointer.
    remove_from_free_list(curr);
    curr = chunk_out_required_size(curr, payload);
    curr->security_key = CHUNK_MAGIC;
    return chunk_to_mem(curr);
}

int kfree(void* mem) {
    if (mem == NULL) {
        return 0;
    }
    header_t* chunk = mem_to_chunk(mem);

    // validation: the magic must match, and the chunk must actually be allocated.
    // (both together reject double-frees and bogus pointers)
    if (chunk->security_key != CHUNK_MAGIC) {
        return 0;
    }
    if (is_allocated(chunk) != 1) {
        return 0;
    }

    // mark this chunk free (keep our own prev_inuse) and burn the magic so a
    // second kfree on the same pointer fails the check above.
    set_size(chunk, chunk_size(chunk), 0, prev_inuse(chunk));
    chunk->security_key = 0;

    // coalsecing the next chunk
    header_t* next = next_chunk(chunk);
    if (is_allocated(next) == 0) {
        remove_from_free_list(next); 
        set_size(chunk, chunk_size(chunk) + chunk_size(next), 0, prev_inuse(chunk));
    }

    // backwards coalesce
    if (prev_inuse(chunk) == 0) {
        footer_t* prev_foot = (footer_t*)((char*)chunk - FOOTER_SIZE);
        header_t* prev = (header_t*)((char*)chunk - prev_foot->size);
        remove_from_free_list(prev);
        set_size(prev, chunk_size(prev) + chunk_size(chunk), 0, prev_inuse(prev));
        chunk = prev;
    }

    // now that the final size is settled, stamp the footer and tell our right
    // neighbour that the chunk in front of it is free.
    footer_of(chunk)->size = chunk_size(chunk);
    header_t* after = next_chunk(chunk);
    set_size(after, chunk_size(after), is_allocated(after), 0);

    // add the merged chunk back onto the free list (append at the tail)
    set_next(chunk, NULL);
    if (free_list_tail == NULL) {
        set_prev(chunk, NULL);
        free_list_head = chunk;
    } else {
        set_prev(chunk, free_list_tail);
        set_next(free_list_tail, chunk);
    }
    free_list_tail = chunk;

    return 1;
}

int remove_from_free_list(header_t* h) {
    if (h == NULL) {
        return 0; 
    }
    header_t* next = get_next(h);
    header_t* prev = get_prev(h);

    if (next == NULL && prev == NULL) { //this is the first and only block in the list
        free_list_head = NULL;
        free_list_tail = NULL; 
    }
    else if (next == NULL) { // this is the last item in the free list
        set_next(prev, NULL); 
        free_list_tail = prev; 
    }
    else if (prev == NULL) { // this is the first item in the free list aka this is the head of the free list
        set_prev(next, NULL);
        free_list_head = next;
    }
    else { // the normal condition
        set_next(prev, next);
        set_prev(next, prev);
    }
    return 1; 
}


header_t* chunk_out_required_size(header_t* chunk, uint64_t size) {
    uint64_t total = chunk_size(chunk);
    uint64_t alloc_total = HEADER_SIZE + ALIGN_UP(size);  // header + padded payload
    uint64_t remainder = total - alloc_total;           // what would be left after carving

    // Not enough left over to be a standalone chunk so wehand out the whole thing.
    if (remainder < MIN_CHUNK_SIZE) {
        set_size(chunk, total, 1, prev_inuse(chunk));
        header_t* after = next_chunk(chunk);
        set_size(after, chunk_size(after), is_allocated(after), 1);
        return chunk;
    }

    set_size(chunk, alloc_total, 1, prev_inuse(chunk));
    header_t* split = next_chunk(chunk);

    // free flag, its previous chunk (`chunk`) is now in use so  prev_inuse 1
    set_size(split, remainder, 0, 1);
    footer_of(split)->size = chunk_size(split);

    // Append split to the tail of the free list, handling the empty-list case.
    set_next(split, NULL);
    if (free_list_tail == NULL) { // list is empty
        set_prev(split, NULL);
        free_list_head = split;
    } else {
        set_prev(split, free_list_tail);
        set_next(free_list_tail, split);
    }
    free_list_tail = split;
    return chunk;
}
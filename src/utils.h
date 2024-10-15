/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  6/28/2024 3:32:38 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(UTILS_H)

typedef struct mem_arena mem_arena;
struct mem_arena {
    usize used;
    usize size; /* NOTE(abid): Size of the memory we've committed */
    usize max_size; /* NOTE(abid): Size of the memory we've reserved. */
    usize alloc_stride;
    void *ptr;
    
    u32 temp_count;
};

typedef struct {
    mem_arena *arena;
    usize used;
} temp_memory;

#define UTILS_H
#endif

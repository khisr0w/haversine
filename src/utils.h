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
    usize Used;
    usize Size;
    void *Ptr;
    
    u32 TempCount;
    
    mem_arena *Next;
    mem_arena *Prev;
};

#define UTILS_H
#endif

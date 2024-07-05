/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  6/28/2024 3:32:38 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(UTILS_H)

typedef struct {
    usize Used;
    usize MaxSize;
    void *Ptr;
    
    u32 TempCount;
} mem_arena;

#define UTILS_H
#endif

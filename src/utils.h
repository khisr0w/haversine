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
    usize size;
    void *ptr;
    
    u32 tempcount;
    
    mem_arena *next;
    mem_arena *prev;
};

typedef struct token token;
typedef struct {
    mem_arena *arena;

    token *token_list;
    token *current_token;
} memory;

#define UTILS_H
#endif

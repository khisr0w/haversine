/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  6/28/2024 3:31:58 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include "utils.h"

#ifdef PLT_WIN
#include <windows.h>
#endif

#define assert(Expr, ErrorStr, ...) \
    if((Expr)) { } \
    else { \
        fprintf(stderr, "ASSERTION ERROR (%s:%d): " ErrorStr "\n", \
                __FILE__, __LINE__, ##__VA_ARGS__); \
        *(i32 *)0 = 0; \
    }

inline internal f64
Square(f64 A) {
    f64 Result = (A*A);
    return Result;
}

inline internal f64 
RadiansFromDegrees(f64 Degrees) {
    f64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}

inline internal void *
platform_allocate(usize Size) {
    void *Result = NULL;

#ifdef PLT_WIN
    Result = VirtualAlloc(NULL, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(Result == NULL) {
        GetLastError();
        exit(EXIT_FAILURE);
    }
#endif 

#ifdef PLT_LINUX
    Result = mmap(NULL, Size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(Result == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
#endif 

    return Result;
}

internal mem_arena *
allocate_arena(usize bytes_to_allocate) {
    mem_arena *arena = platform_allocate(sizeof(mem_arena) + bytes_to_allocate);
    arena->ptr = (void *)(arena + 1);
    arena->size = bytes_to_allocate;
    arena->used = 0;
    arena->next = NULL;
    arena->prev = NULL;

    return arena;
}

#define push_struct(type, arena) (type *)push_size(arena, sizeof(type))
#define push_array(Arena, Type, Count) (Type *)push_size(Arena, (Count)*sizeof(Type))
internal void *
push_size(memory *current_memory, usize size) {
    /* NOTE(Abid): If out of memory swap the old arena with a newly allocated one. */
    if(current_memory->arena->used + size >= current_memory->arena->size) {
        printf("Expand memory(Prev = %zi > %zi)\n", current_memory->arena->used + size, current_memory->arena->size);
        mem_arena *prev_arena = current_memory->arena;
        mem_arena *new_arena = allocate_arena(prev_arena->size);
        new_arena->prev = prev_arena;
        prev_arena->next = new_arena;
        current_memory->arena = new_arena;
    }

    void *result = (u8 *)current_memory->arena->ptr + current_memory->arena->used;
    current_memory->arena->used += size;

    return result;
}
internal void *
push_size_arena(mem_arena *Arena, usize Size){
    Assert(Arena->used + Size < Arena->size, "not enough arena memory");
    void *Result = (u8 *)Arena->ptr + Arena->used;
    Arena->used += Size;

    return Result;
}

#define ArenaCurrent(Arena) (void *)((u8 *)(Arena)->ptr + (Arena)->used)
#define ArenaAdvance(Arena, Number, Type) (Arena)->used += sizeof(Type)*(Number)

inline internal mem_arena
sub_arena_create(usize Size, mem_arena *Arena) {
    mem_arena SubArena = {0};
    SubArena.size = Size;
    SubArena.ptr = push_size_arena(Arena, SubArena.size);
    SubArena.used = 0;

    return SubArena;
}

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

#define __assert_glue(a, b) a ## b

#define assert_static(expr)                      \
    enum {                                       \
        __assert_glue(assert_fail_at_, __LINE__) \
            = 1 (int)(!!(expr))                  \
    }

#define assert(Expr, ErrorStr, ...) \
    if((Expr)) { } \
    else { \
        fprintf(stderr, "ASSERTION ERROR (%s:%d): " ErrorStr "\n", \
                __FILE__, __LINE__, ##__VA_ARGS__); \
        *(i32 *)0 = 0; \
        exit(EXIT_FAILURE); \
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

/* NOTE(abid): Get the physical memory (RAM) size. */
inline internal usize
platform_ram_size_get() {
#ifdef PLT_WIN
    MEMORYSTATUSEX mem_stat = { .dwLength = sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&mem_stat);
    return mem_stat.ullTotalPhys;
#elif PLT_LINUX
    /* TODO(abid): Implement `platform_get_ram_size()` for linux. */
    assert_static(0);
#endif
}

inline internal void *
platform_reserve(usize reserve_size) {
#ifdef PLT_WIN
    void *result = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_NOACCESS);
    if(result == NULL) {
        GetLastError();
        exit(EXIT_FAILURE);
    }
#elif PLT_LINUX
    void *result = mmap(NULL, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if(result == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
#endif

    return result;
}

/* NOTE(abid): Commit memory for the already reserved virtual memory space. */
inline internal void *
platform_commit(void *base_addr, usize commit_size) {
#ifdef PLT_WIN
    void *result = VirtualAlloc(base_addr, commit_size, MEM_COMMIT, PAGE_READWRITE);
    if(result == NULL) {
        GetLastError();
        exit(EXIT_FAILURE);
    }
#elif PLT_LINUX
    void *result = mmap(base_addr, Size, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if(result == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
#endif 

    return result;
}

inline internal temp_memory
mem_temp_begin(mem_arena *arena) {
    temp_memory result = {0};

    result.arena = arena;
    result.used = arena->used;

    ++arena->temp_count;

    return result;
}

inline internal void
mem_temp_end(temp_memory temp_mem) {
    mem_arena *arena = temp_mem.arena;
    Assert(arena->used >= temp_mem.used, "something was freed when it shouldn't have been.");
    Assert(arena->temp_count > 0, "no temp memory registered for it to end.");
    arena->used = temp_mem.used;

    --arena->temp_count;
}

/* NOTE(abid): At the moment, we reserve a virtual memory space equivalent to the
 * system's entire physical memory. This means that we will only run out of memory
 * if we've exhaused all physical memory space or we will start paging to disk :( - 28.Sep.2024
 * TODO: We are not keeping track of the committed pages yet, which means we have no way of
 * shrinking the memory. Figure out if the added computation is worth it. - 28.Sep.2024 */
internal mem_arena *
arena_create(usize bytes_to_allocate) {
    usize physical_mem_max_size = platform_ram_size_get();
    void *base_addr = platform_reserve(physical_mem_max_size);
    mem_arena *arena = platform_commit(base_addr, sizeof(mem_arena) + bytes_to_allocate);
    /* TODO(abid): Probably need to remove this, most OS already zero out the memory. Investigate. */
    memset(arena, 0, bytes_to_allocate);
    arena->ptr = (void *)(arena + 1);
    arena->size = bytes_to_allocate;
    arena->used = 0;
    arena->max_size = physical_mem_max_size;
    arena->alloc_stride = bytes_to_allocate; /* NOTE(abid): How much to commit when memory is full. Check `push_size`. */

    return arena;
}

#define push_struct(type, arena) (type *)push_size(sizeof(type), arena)
#define push_array(type, count, arena) (type *)push_size((count)*sizeof(type), arena)
internal void *
push_size(usize size, mem_arena *arena) {
    /* NOTE(abid): If out of memory commit more memory. */
    if(arena->used + size >= arena->size) {
        assert(arena->used + size < physical_mem_max_size, "out of memory. Sorry.");

        u64 size_to_allocate = (arena->alloc_stride > size) ? arena->alloc_stride : size;
        platform_commit(arena->ptr + size, size_to_allocate);
    }

    void *result = (u8 *)arena->ptr + arena->used;
    arena->used += size;

    return result;
}

#if 0
internal void *
push_size_arena(mem_arena *Arena, usize Size){
    Assert(Arena->used + Size < Arena->size, "not enough arena memory");
    void *Result = (u8 *)Arena->ptr + Arena->used;
    Arena->used += Size;

    return Result;
}
#endif

#define ArenaCurrent(Arena) (void *)((u8 *)(Arena)->ptr + (Arena)->used)
#define ArenaAdvance(Arena, Number, Type) (Arena)->used += sizeof(Type)*(Number)

inline internal mem_arena
sub_arena_create(usize size, mem_arena *arena) {
    mem_arena SubArena = {0};
    sub_arena.size = size;
    sub_arena.ptr = push_size_arena(arena, sub_arena.size);
    sub_arena.used = 0;

    return sub_arena;
}

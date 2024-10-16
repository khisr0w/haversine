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
#elif PLT_LINUX
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#endif

/* NOTE(abid): Byte Macros */
#define kilobyte(value) ((value)*1024LL)
#define megabyte(value) (kilobyte(value)*1024LL)
#define gigabyte(value) (megabyte(value)*1024LL)
#define terabyte(value) (gigabyte(value)*1024LL)

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
square(f64 a) {
    f64 result = (a*a);
    return result;
}

inline internal f64 
radians_from_degrees(f64 degrees) {
    f64 result = 0.01745329251994329577f * degrees;
    return result;
}

/* NOTE(abid): Get the physical memory (RAM) size. */
inline internal usize
platform_ram_size_get() {
#ifdef PLT_WIN
    MEMORYSTATUSEX mem_stat = { .dwLength = sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&mem_stat);
    return mem_stat.ullTotalPhys;
#elif PLT_LINUX
    return getpagesize();
#endif
}

inline internal void *
platform_allocate(usize alloc_size) {
#ifdef PLT_WIN
    void *result = VirtualAlloc(NULL, alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(result == NULL) {
            GetLastError();
            exit(EXIT_FAILURE);
    }
#elif PLT_LINUX
    void *result = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if(result == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
#endif

    return result;
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
    void *result = mmap(NULL, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, -1, 0);
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
    void *result = mmap(base_addr, commit_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if(result == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
#endif 

    return result;
}

inline internal bool
platform_free(void *ptr, usize size) {
#ifdef PLT_WIN
    /* NOTE(abid): Win32 requires the size to be zero when MEM_RELEASE. */
    size = 0;
    bool result = (VirtualFree(ptr, size, MEM_RELEASE) != 0);
#elif PLT_LINUX
    bool result = (munmap(ptr, size) == 0);
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

/* TODO: We are not keeping track of the committed pages yet, which means we have no way of
 * shrinking the memory. Figure out if the added computation is worth it. - 28.Sep.2024 */
internal mem_arena *
arena_create(usize bytes_to_allocate, usize bytes_to_reserve) {
    usize physical_mem_max_size = bytes_to_reserve;
    /* TODO(abid): Probably need to get rid of the ram branch here, user can decide. - 16.Oct.2024 */
    if(bytes_to_reserve == 0) physical_mem_max_size = platform_ram_size_get();
    
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

inline internal bool
arena_free(mem_arena *arena) { return platform_free(arena->ptr, arena->size); }

#define push_struct(type, arena) (type *)push_size(sizeof(type), arena)
#define push_array(type, count, arena) (type *)push_size((count)*sizeof(type), arena)
internal void *
push_size(usize size, mem_arena *arena) {
    /* NOTE(abid): If out of memory commit more memory. */
    if(arena->used + size >= arena->size) {
        assert(arena->used + size < arena->max_size, "out of memory. Sorry.");

        u64 size_to_allocate = (arena->alloc_stride > size) ? arena->alloc_stride : size;
        platform_commit((u8 *)arena->ptr + size, size_to_allocate);
        arena->size += size_to_allocate;
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

#define arena_current(Arena) (void *)((u8 *)(Arena)->ptr + (Arena)->used)
#define arena_advance(Arena, Number, Type) (Arena)->used += sizeof(Type)*(Number)

#if 0
inline internal mem_arena
sub_arena_create(usize size, mem_arena *arena) {
    mem_arena sub_arena = {0};
    sub_arena.size = size;
    sub_arena.ptr = push_size_arena(arena, sub_arena.size);
    sub_arena.used = 0;

    return sub_arena;
}
#endif

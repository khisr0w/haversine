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
PlatoformAllocate(usize Size) {
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
AllocateArena(usize BytesToAllocate) {
    mem_arena *Arena = PlatoformAllocate(sizeof(mem_arena) + BytesToAllocate);
    Arena->Ptr = (void *)(Arena + 1);
    Arena->Size = BytesToAllocate;
    Arena->Used = 0;

    return Arena;
}

#define PushStruct(Arena, Type) (Type *)PushSize(Arena, sizeof(Type))
#define PushArray(Arena, Type, Count) (Type *)PushSize(Arena, (Count)*sizeof(Type))
internal void *
PushSize(mem_arena *Arena, usize Size) {
    /* NOTE(Abid): If out of memory swap the old arena with a newly allocated one. */
    if(Arena->Used + Size <= Arena->Size) {
        Arena = (mem_arena *)Arena->Ptr - 1;
        mem_arena *NewArena = AllocateArena(Arena->Size);
        NewArena->Prev = Arena;
        Arena->Next = NewArena;
        Arena = NewArena;
    }
    void *Result = (u8 *)Arena->Ptr + Arena->Used;
    Arena->Used += Size;

    return Result;
}

#define ArenaCurrent(Arena) (void *)((u8 *)(Arena)->Ptr + (Arena)->Used)
#define ArenaAdvance(Arena, Number, Type) (Arena)->Used += sizeof(Type)*(Number)

inline internal mem_arena
SubArenaCreate(usize Size, mem_arena *Arena) {
    mem_arena SubArena = {0};
    SubArena.Size = Size;
    SubArena.Ptr = PushSize(Arena, SubArena.Size);
    SubArena.Used = 0;

    return SubArena;
}


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

internal f64
Square(f64 A) {
    f64 Result = (A*A);
    return Result;
}

internal f64 
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

internal mem_arena
AllocateArena(usize BytesToAllocate) {
    mem_arena Arena = {0};
    Arena.MaxSize = BytesToAllocate;
    Arena.Ptr = PlatoformAllocate(Arena.MaxSize);
    Arena.Used = 0;

    return Arena;
}

#define PushStruct(Arena, Type) (Type *)PushSize(Arena, sizeof(Type))
#define PushArray(Arena, Type, Count) (Type *)PushSize(Arena, (Count)*sizeof(Type))
internal void *
PushSize(mem_arena *Arena, usize Size) {
    Assert(Arena->Used + Size <= Arena->MaxSize, "not enough arena memory");
    void *Result = (u8 *)Arena->Ptr + Arena->Used;
    Arena->Used += Size;

    return Result;
}

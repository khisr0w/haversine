/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Do 14 Nov 2024 03:14:18 CET                                   |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(TYPES_H)

typedef size_t usize;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i32;
typedef int64_t i64;
typedef uintptr_t uintptr;
typedef float f32;
typedef double f64;
typedef int8_t bool;
typedef uint8_t u8;
typedef int8_t i8;
typedef int16_t i16;
#define internal static
#define local_persist static
#define global_var static
#define true 1
#define false 0
#define array_size(Arr) sizeof((Arr)) / sizeof((Arr)[0])

#define TYPES_H
#endif

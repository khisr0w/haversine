/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Do 14 Nov 2024 03:16:42 CET                                   |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(RANDOM_H)

typedef struct {
    u64 V;

    i32 NumU8Reserves;
    i32 NumU16Reserves;
    u64 U8Reserves;
    u64 U16Reserves;
} rand_state;

#define RANDOM_H
#endif

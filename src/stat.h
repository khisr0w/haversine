/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Do 14 Nov 2024 03:15:49 CET                                   |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(STAT_H)

typedef struct {
    f64 Latest;
    f64 Sum;
    f64 SumSquared;
    u64 Count;
    f64 Max;
    f64 Min;
} stat_f64;

#define STAT_H
#endif

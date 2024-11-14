/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Do 14 Nov 2024 03:15:12 CET                                   |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include "stat.h"

internal inline void
stat_f64_accumulate(f64 Value, stat_f64 *Stat) {
    if(Stat->Count == 0) {
        Stat->Max = Value;
        Stat->Min = Value;
    }

    Stat->Latest = Value;
    Stat->SumSquared += Value*Value;
    Stat->Sum += Value;
    ++Stat->Count;
}
internal inline f64
stat_f64_mean(stat_f64 *Stat) {
    Assert(Stat->Count > 0, "cannot calculate mean for count < 1");
    return Stat->Sum / Stat->Count;
}

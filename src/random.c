/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Do 14 Nov 2024 03:16:16 CET                                   |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include "random.h"

global_var rand_state __GLOBALRandState = {
        .V = 4101842887655102017LL,
        .NumU8Reserves = 0,
        .NumU16Reserves = 0,
        .U8Reserves = 0,
        .U16Reserves = 0,
};

inline internal void
rand_seed(u64 Seed) {
    /* NOTE(abid): Reserve bits for U8 and U16 routines. */

    __GLOBALRandState.V ^= Seed;
    /* NOTE(abid): RandU64() routine here. */
    __GLOBALRandState.V ^= __GLOBALRandState.V >> 21;
    __GLOBALRandState.V ^= __GLOBALRandState.V << 35;
    __GLOBALRandState.V ^= __GLOBALRandState.V >> 4;
    __GLOBALRandState.V *= 2685821657736338717LL;
}

inline internal u64 
RandU64() {
    __GLOBALRandState.V ^= __GLOBALRandState.V >> 21;
    __GLOBALRandState.V ^= __GLOBALRandState.V << 35;
    __GLOBALRandState.V ^= __GLOBALRandState.V >> 4;
    return __GLOBALRandState.V * 2685821657736338717LL;
}

/* NOTE(abid): Range in [Min, Max) */
/* TODO(abid): Get rid of modulus in here (faster). */
inline internal u64
rand_range_u64(u64 Min, u64 Max) { return Min + RandU64() % (Max-Min); }

inline internal u32 RandU32() { return (u32)RandU64(); }

inline internal u16 RandU16() {
    if(__GLOBALRandState.NumU16Reserves--)
        return (u16)(__GLOBALRandState.U16Reserves >>= 16);
    __GLOBALRandState.U16Reserves = RandU64();
    __GLOBALRandState.NumU16Reserves = 3;

    return (u16)__GLOBALRandState.U16Reserves;
}

inline internal u8 RandU8() {
    if(__GLOBALRandState.NumU8Reserves--)
        return (u8)(__GLOBALRandState.U8Reserves >>= 8);
    __GLOBALRandState.U8Reserves = RandU64();
    __GLOBALRandState.NumU8Reserves = 7;

    return (u8)__GLOBALRandState.U8Reserves;
}
inline internal f64
RandF64() { return 5.42101086242752217e-20 * (f64)RandU64(); }

/* NOTE(abid): Range [Min, Max] */
inline internal f64
rand_range_f64(f64 Min, f64 Max) { return Min + RandF64() * (Max - Min); }

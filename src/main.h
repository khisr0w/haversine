/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  6/28/2024 1:25:24 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(MAIN_H)

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
#define ArraySize(Arr) sizeof((Arr)) / sizeof((Arr)[0])
#define Assert(Expr, ErrorStr) if(!(Expr)) { fprintf(stderr, "ASSERTION ERROR (%s:%d): " ErrorStr "\nExiting...\n", __FILE__, __LINE__); *(i32 *)0 = 0; }

/* NOTE(Abid): Byte Macros */
#define Kilobyte(Value) ((Value)*1024LL)
#define Megabyte(Value) (Kilobyte(Value)*1024LL)
#define Gigabyte(Value) (Megabyte(Value)*1024LL)
#define Terabyte(Value) (Gigabyte(Value)*1024LL)

typedef struct {
    f64 Latest;
    f64 Sum;
    f64 SumSquared;
    u64 Count;
    f64 Max;
    f64 Min;
} stat_f64;

internal inline void
StatF64Accumulate(f64 Value, stat_f64 *Stat) {
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
StatF64Mean(stat_f64 *Stat) {
    Assert(Stat->Count > 0, "cannot calculate mean for count < 1");
    return Stat->Sum / Stat->Count;
}

typedef struct {
    u64 V;

    i32 NumU8Reserves;
    i32 NumU16Reserves;
    u64 U8Reserves;
    u64 U16Reserves;
} rand_state;

global_var rand_state __GLOBALRandState = {
        .V = 4101842887655102017LL,
        .NumU8Reserves = 0,
        .NumU16Reserves = 0,
        .U8Reserves = 0,
        .U16Reserves = 0,
};

inline internal void
RandSeed(u64 Seed) {
    /* NOTE(Abid): Reserve bits for U8 and U16 routines. */

    __GLOBALRandState.V ^= Seed;
    /* NOTE(Abid): RandU64() routine here. */
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

/* NOTE(Abid): Range in [Min, Max) */
/* TODO(Abid): Get rid of modulus in here (faster). */
inline internal u64
RandRangeU64(u64 Min, u64 Max) { return Min + RandU64() % (Max-Min); }
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

/* NOTE(Abid): Range [Min, Max] */
inline internal f64
RandRangeF64(f64 Min, f64 Max) { return Min + RandF64() * (Max - Min); }

#define MAIN_H
#endif

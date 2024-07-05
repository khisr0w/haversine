/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  6/28/2024 1:23:31 AM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "main.h"
#include "utils.c"

/* NOTE(Abid): EarthRadius is generally expected to be 6372.8 */
internal f64
ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius) {
    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    
    f64 dLat = RadiansFromDegrees(lat2 - lat1);
    f64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);
    
    f64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 Result = EarthRadius * c;
    
    return Result;
}


internal u64
StrLen(char *Str) {
    Assert(Str, "cannot find len for an empty string.");

    u64 Len = 0;
    while(Str[Len]) ++Len;

    return Len;
}

internal void
OffloadToBuffer(char *Buffer, u64 *BufferPointer, u64 BufferSize, f64 Y0, f64 Y1, f64 X0, f64 X1,
                f64 *ResultBuffer, u64 *ResultPointer, u64 ResultSize, bool IsLast,
                char *JSONFilename, char *F64Filename) {
    char *Suffix = "\n]}";
    u64 SuffixLen = 3;

    /* NOTE(Abid): We will be overestimating our memory usage since we assume each number's
     *             length to be 3 + FloatPrecision, which will not always be true. */
    u32 Precision = 20;
    i32 NumCharsPerPair = 1 + 2 + 4*4 + 4 + 3 + 1 + 1 + 4*(4 + 1 + Precision);
    sprintf_s(Buffer + *BufferPointer, BufferSize-*BufferPointer,
              "\t{\"x0\":%.*f, \"y0\":%.*f, \"x1\":%.*f, \"y1\":%.*f}",
              Precision, X0, Precision, Y0, Precision, X1, Precision, Y1);
    *BufferPointer += StrLen(Buffer + *BufferPointer);
    if(IsLast) {
        sprintf_s(Buffer + *BufferPointer, SuffixLen+1, Suffix);
        *BufferPointer += SuffixLen;
    } else {
        sprintf_s(Buffer + *BufferPointer, 3, ",\n");
        *BufferPointer += 2;
    }

    /* NOTE(Abid): Save the result to buffer and .f64 file. */
    f64 Result = ReferenceHaversine(X0, Y0, X1, Y1, 6372.8);
    ResultBuffer[*ResultPointer] = Result;
    (*ResultPointer)++;
    if(*ResultPointer >= ResultSize/sizeof(f64) || IsLast) {
        FILE *FileHandle = NULL;
        fopen_s(&FileHandle, F64Filename, "ab");
        Assert(FileHandle, "cannot save to .f64 file.");
        fwrite(ResultBuffer, sizeof(f64), *ResultPointer, FileHandle);
        fclose(FileHandle);
        *ResultPointer = 0;
    }

    if((*BufferPointer + NumCharsPerPair + SuffixLen + 1 >= BufferSize) || IsLast) {
        FILE *FileHandle = NULL;
        fopen_s(&FileHandle, JSONFilename, "ab");
        Assert(FileHandle, "cannot save to .json file.");
        fputs(Buffer, FileHandle);
        fclose(FileHandle);
        *BufferPointer = 0;
    }
}

internal stat_f64
GenerateHaversineJSON(u64 NumberPairs, u64 NumClusters, char *Filename, mem_arena *Arena) {
    u64 FilenameLen = StrLen(Filename);
    char *JSONExtension = ".json";
    char *F64Extension = ".f64";
    u64 JSONExtensionLen = StrLen(JSONExtension);
    u64 F64ExtensionLen = StrLen(F64Extension);
    char *JSONFilename = PushSize(Arena, FilenameLen + JSONExtensionLen + 1);
    char *F64Filename = PushSize(Arena, FilenameLen + F64ExtensionLen + 1);

    for(u64 Idx = 0; Idx < FilenameLen; ++Idx) {
        JSONFilename[Idx] = Filename[Idx];
        F64Filename[Idx] = Filename[Idx];
    }
    for(u64 Idx = 0; Idx < JSONExtensionLen; ++Idx) JSONFilename[FilenameLen+Idx] = JSONExtension[Idx];
    JSONFilename[JSONExtensionLen+FilenameLen] = '\0';
    for(u64 Idx = 0; Idx < F64ExtensionLen; ++Idx) F64Filename[FilenameLen+Idx] = F64Extension[Idx];
    F64Filename[F64ExtensionLen+FilenameLen] = '\0';

    stat_f64 HaversineState = {0};

    u64 BufferSize = (u64)((Arena->MaxSize - Arena->Used) / 2);
    char *Buffer = PushSize(Arena, BufferSize);
    usize BufferPointer = 0;

    u64 ResultSize = Arena->MaxSize - Arena->Used;
    /// u64 ResultSize = 4*sizeof(f64);
    f64 *ResultBuffer = PushSize(Arena, ResultSize);
    usize ResultPointer = 0;

    /* NOTE(Abid): Alignment to the 8 bytes of f64. */
    usize Alignment = sizeof(f64);
    Assert(ResultSize >= 2*sizeof(f64)*Alignment, "minimum size requirement for result buffer not met.");
    usize AlignmentMask = Alignment - 1;
    if((usize)ResultBuffer & AlignmentMask) {
        usize AligmentOffset = Alignment - ((usize)ResultBuffer & AlignmentMask);
        ResultBuffer = (void *)((usize)ResultBuffer + AligmentOffset);
        ResultSize -= sizeof(f64);
    }

    char *Prefix = "{\"pairs\":[\n";
    u64 PrefixLen = 11;
    for(; BufferPointer < PrefixLen; ++BufferPointer) Buffer[BufferPointer] = Prefix[BufferPointer];

    if(NumClusters) {
        // NumClusters = (NumClusters) ? NumClusters : RandRangeU64(20, 300);
        u64 NumPairPerCluster = (u64)(NumberPairs / NumClusters);
        u64 PairRemainder = NumberPairs % NumClusters;
        if(PairRemainder) ++NumClusters;
        for(u64 ClusterIdx = 0; ClusterIdx < NumClusters; ClusterIdx++) {
            f64 Lat1Size = RandRangeF64(10., 100.);
            f64 Lon1Size = RandRangeF64(20., 200.);
            f64 Lat1Start = RandRangeF64(-90., 90. - Lat1Size);
            f64 Lon1Start = RandRangeF64(-180., 180. - Lon1Size);

            f64 Lat2Size = RandRangeF64(10., 100.);
            f64 Lon2Size = RandRangeF64(20., 200.);
            f64 Lat2Start = RandRangeF64(-90., 90. - Lat2Size);
            f64 Lon2Start = RandRangeF64(-180., 180. - Lon2Size);

            /* NOTE(Abid): In case we have remainders left. */
            u64 NumToGenerate = ((ClusterIdx == NumClusters-1) && PairRemainder) ? PairRemainder
                                                                                 : NumPairPerCluster;
            for(u64 PairIdx = 0; PairIdx < NumToGenerate; PairIdx++) {
                f64 Lat1 = RandRangeF64(Lat1Start, Lat1Start+Lat1Size);
                f64 Lat2 = RandRangeF64(Lat2Start, Lat2Start+Lat2Size);
                f64 Lon1 = RandRangeF64(Lon1Start, Lon1Start+Lon1Size);
                f64 Lon2 = RandRangeF64(Lon2Start, Lon2Start+Lon2Size);
                StatF64Accumulate(Lat1, &HaversineState);
                StatF64Accumulate(Lat2, &HaversineState);
                StatF64Accumulate(Lon1, &HaversineState);
                StatF64Accumulate(Lon2, &HaversineState);
                OffloadToBuffer(Buffer, &BufferPointer, BufferSize, Lat1, Lat2, Lon1, Lon2,
                                ResultBuffer, &ResultPointer, ResultSize,
                                ClusterIdx*NumPairPerCluster + PairIdx + 1 == NumberPairs,
                                JSONFilename, F64Filename);
            }
        }
    } else {
        for(u64 Idx = 0; Idx < NumberPairs; Idx++) {
            f64 Lat1 = RandRangeF64(-90., 90.);
            f64 Lat2 = RandRangeF64(-90., 90.);
            f64 Lon1 = RandRangeF64(-180., 180.);
            f64 Lon2 = RandRangeF64(-180., 180.);
            StatF64Accumulate(Lat1, &HaversineState);
            StatF64Accumulate(Lat2, &HaversineState);
            StatF64Accumulate(Lon1, &HaversineState);
            StatF64Accumulate(Lon2, &HaversineState);

            OffloadToBuffer(Buffer, &BufferPointer, BufferSize, Lat1, Lat2, Lon1, Lon2,
                            ResultBuffer, &ResultPointer, ResultSize,
                            Idx+1 == NumberPairs, JSONFilename, F64Filename);
        }
    }

    return HaversineState;
}

i32 main(i32 argc, char* argv[]) {
    Assert(argc == 5, "[seed] [number of pairs] [number of clusters] [file name]");
    u64 Seed = atoll(argv[1]);
    u64 NumPairs = atoll(argv[2]);
    u64 NumClusters = atoll(argv[3]);
    char* Filename = argv[4];

    RandSeed(Seed);
    mem_arena Arena = AllocateArena(Megabyte(1));
    stat_f64 GenerationStat = GenerateHaversineJSON(NumPairs, NumClusters, Filename, &Arena);
    printf("Seed: %lld\nPair Count: %lld\nExpected Sum: %f\n\n",
            Seed, NumPairs, GenerationStat.Sum);

    return 0;
}

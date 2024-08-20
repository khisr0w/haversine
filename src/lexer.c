/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/12/2024 5:11:40 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */
#include "lexer.h"


inline internal char 
IsIgnoreChar(char Character) {
    /* NOTE(Abid): Add ignore characters here... */
    return (
        (Character ==  ' ') ||
        (Character == '\n') ||
        (Character == '\t') ||
        (Character == '\r')
    );
}

inline internal char
IsEOF(char Character) { return Character == '\0'; }

inline internal char
CurrentChar(buffer *Buffer) { Buffer->Base[Buffer->Pointer]; }

inline internal char
ConsumeUntilChar(buffer *Buffer, char CharToStop) {
    /* NOTE(Abid): Consume until next `CharToStop` appears. \0 is eof. */
    while(CurrentChar(Buffer) != CharToStop) { ++Buffer->Pointer; }

    /* NOTE(Abid): Start here... */
    (CurrentChar(Buffer))) {
    }
}

internal char
ConsumeChar(buffer *Buffer) { 
    /* NOTE(Abid): Consume until next `significant` character appears. \0 is eof. */
    while(IsIgnoreChar(CurrentChar(Buffer))) { ++Buffer->Pointer; }

    /* NOTE(Abid): In case, we've not reached eof yet. */
    if(!IsEOF(PeekCharOffset(Buffer, 0))) return Buffer[Buffer->Pointer++];
    else return Buffer[Buffer->Pointer];
}

inline internal char
PeekCharOffset(buffer *Buffer, usize PosOffset) { return Buffer->Base[Buffer->Pointer + PosOffset]; }

internal void
Lexer(buffer *Buffer) {
    while(Buffer->Length > Buffer->Pointer) {
        char CurrentPos = Consume(Buffer);
        switch(CurrentPos) {
            case "\"": {
                /* NOTE(Abid): We are either at the start of a key or a string value. */

            } break;
        }
    }
}

internal buffer 
ReadTextFile(char *Filename) {
    buffer Result = {0};
    FILE *FileHandle = NULL;
    fopen_s(&FileHandle, Filename, "rb");

    /* NOTE(Abid): Get the size of the File */
    fseek(FileHandle, 0, SEEK_END);
    usize FileSize = ftell(FileHandle);
    rewind(FileHandle);

    /* NOTE(Abid): Read into the start of the main memory */
    char* Content = PlatoformAllocate(FileSize);
    if (Content == NULL) {
        fclose(FileHandle);
        Assert(false, "could not allocate memory");
    }

    size_t BytesRead = fread(Content, 1, FileSize, FileHandle);
    if (BytesRead != FileSize) {
        fclose(FileHandle);
        free(Content);
        Assert(false, "could not load file into memory");
    }
    fclose(FileHandle);

    Result.Base = Content;
    Result.Length = BytesRead - 1; // -1 for the \0 at the end of the buffer

    return Result;
}

internal void
ParseJSON(char *Filename) {
    buffer Buffer = ReadTextFile(Filename);
    Lexer(Buffer);
}

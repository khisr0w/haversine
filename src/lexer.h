/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/19/2024 6:36:46 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(LEXER_H)

typedef enum {
    tt_key,
    tt_value_float,
    tt_value_int,
    tt_value_str,
} token_type;

typedef struct {
    token_type Type;
    void *Body;
} token;

typedef struct {
    char *Base;
    usize Pointer;
    usize Length;
} buffer;

typedef struct {
    char *Ptr;
    usize Length;
} string;

#define LEXER_H
#endif

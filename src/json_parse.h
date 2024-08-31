/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/19/2024 6:36:46 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#if !defined(JSON_PARSE_H)

typedef struct {
    char *data;
    usize length;
} string;

typedef struct {
    string str;
    usize current_idx;
} buffer;

typedef enum {
    tt_key,
    tt_value_float,
    tt_value_int,
    tt_value_str,
    tt_list_begin,
    tt_list_end,
    tt_dict_begin,
    tt_dict_end,
    tt_comma,
} token_type;

typedef struct token token;
struct token {
    token_type type;
    void *body;

    token *next;
};

typedef enum {
    jv_str,
    jv_float,
    jv_int,
    jv_dict,
    jv_list,
} json_value_type;

typedef struct {
    string key;
    void *value;
} json;

typedef struct {
    i32 placeholder;
} json_dict;

typedef struct {
    i32 placeholder;
} json_list;

#define JSON_PARSE_H
#endif

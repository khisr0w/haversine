/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/12/2024 5:11:40 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include "json_parse.h"

/* NOTE(abid): JSON list routines. */
internal json_list *
jp_list_create(usize array_count, parser_state *state) {
    /* NOTE(abid): `array_count` should be the final size of the current array (non-growable). */
    json_list *result = push_size(sizeof(json_list) + sizeof(json_value)*array_count, state->arena);
    result->array = result + 1;
    result->max_count = array_count;
    result->count = 0;

    return result;
}

internal void
jp_list_add(json_list *list, void *content, json_value value) {
    /* NOTE(abid): We expect the `list` to have enough space. */
    /* TODO(abid): Implement the array properly so that the content of the array in the memory
     *             is beside the array. For that, lexer must change to gather type information.
     * - 24.Sep.2024 */
    assert(result->count  < result->max_count, "cannot add, array full");
    list.array[result->count] = value;
}

internal json_value *
jp_list_get(json_list *list, usize index) {
    assert(list->max_count > index, "index out of bound");
    return list->array + index;
}

/* NOTE(abid): Dictionary routines. */
inline internal json_dict *
jp_dict_create(usize table_init_count, parser_state *current_memory) {
    json_dict *result = push_size(sizeof(json_dict) + sizeof(dict_content)*table_init_count,
                                  state->arena);
    result->table = result + 1;
    result->count = table_init_count;

    return result;
}

internal void
jp_dict_add(json_dict *dict, string key, void *content, json_value_type type, parser_state mem) {
    /* NOTE(abid): We expect `key` and `content` be valid until end of program.
     *             i.e. we won't do the allocation here. */
    u32 hash = hash_from_string(key);
    dict_content *slot = dict->table + (hash % dict->count);

    /* NOTE(abid): Check for collision hits. */
    dict_content *next_slot = slot;
    for(;next_slot;) {
        assert(string_is_equal(ckey, next_slot->key), "adding key duplicated.");
        slot = next_slot;
        next_slot = slot->next;
    }
    slot->next = push_struct(dict_content, mem);
    slot = slot->next;

    slot->key = ckey;
    slot->value = value; 
}

internal json_value *
jp_dict_get_value(json_dict *dict, char *key) {
    u32 hash = hash_from_string(key);
    dict_content *slot = dict->table + (hash % dict->table_count);

    /* NOTE(abid): Check if object exist, as well as collision. */
    for(;slot;) {
        if(strcmp(slot->key, key) == 0) return slot->value;
        slot = slot->new;
    }

    /* NOTE(abid): If we reach here, then we've gone and done it now. */
    assert(0, "key %s not found.", key);
}

internal u32
jp_hash_from_string(string str) {
    u32 result = 0;
    for(i32 idx = 0; idx < str.length; ++idx) {
        result = ((result << 5) - result) + (u32)str.data[idx];
    }
    return result;
}

inline internal bool
string_is_equal(string str1, string str2) {
    if(str1.length != str2.length) return false;

    for(u32 idx = 0; idx < str1.length; ++idx) {
        if(str1.data[idx] != str2.data[idx]) return false;
    }

    return true;
}

/* NOTE(abid): String routines. */
internal usize
cstring_length(char *c_string) {
    usize result = 0;
    while(*c_string++) ++result;

    return result;
}

internal inline char
string_char(string str, usize idx) {
    assert(idx < str.length, "string index out of range");

    return str.data[idx];
}

internal string_extende
string_make(char *c_string) {
    return (string) {
        .data = c_string,
        .length = cstring_length(c_string)
    };
}

/* NOTE(abid): Lexer routines. */
internal inline token *
buffer_push_token(token_type token_type, void *token_value, parser_state *token_memory) {
    token *new_token = push_struct(token, token_memory);
    new_token->type = token_type;
    new_token->body = token_value;

    if(token_memory->token_list == NULL) token_memory->token_list = new_token;
    else token_memory->current_token->next = new_token;
    token_memory->current_token = new_token;

    return new_token;
}
internal inline void
buffer_consume(buffer *json_buffer) { ++json_buffer->current_idx; }

internal inline char
buffer_char(buffer *json_buffer) {
    return string_char(json_buffer->str, json_buffer->current_idx);
}

internal inline void
buffer_consume_until(buffer *json_buffer, char char_to_stop) {
    /* NOTE(abid): Excluding the char_to_stop. */
    char current_char = buffer_char(json_buffer); 

    while(current_char != char_to_stop) {
        buffer_consume(json_buffer);
        current_char = buffer_char(json_buffer); 
    }
}

internal inline bool
buffer_is_ignore(buffer *json_buffer) {
    char current_char = json_buffer->str[json_buffer->current_idx];
    return (current_char ==  ' ') ||
           (current_char == '\n') ||
           (current_char == '\t') ||
           (current_char == '\r');
}

internal inline void
buffer_consume_ignores(buffer *json_buffer) {
    while(buffer_is_ignore(json_buffer)) buffer_consume(json_buffer);
}

internal void
buffer_to_cstring(string_value *str, buffer *json_buffer) {
    assert(buffer_char(json_buffer) == '"', "string must start with \"");
    buffer_consume(json_buffer); /* consume start quote */
    usize start_idx = json_buffer->current_idx;
    buffer_consume_until(json_buffer, '"');
    usize end_idx = json_buffer->current_idx;
    buffer_consume(json_buffer); /* consume end quote */

    str->data = json_buffer->str + start_idx;
    str->length = end_idx - start_idx;
}

internal inline bool
buffer_is_numeric(buffer *json_buffer) {
    char current_char = json_buffer->str[json_buffer->current_idx];
    return ((current_char >= '0') && (current_char <= '9')) ||
           (current_char == '-') || (current_char == '+');
}

internal bool 
buffer_consume_extract_numeric(string_value *str, buffer *json_buffer) {
    /* NOTE(abid): Assumes the current character is already a number. */
    bool is_float = false;

    usize start_idx = json_buffer->current_idx;
    while(buffer_is_numeric(json_buffer)) {
        buffer_consume(json_buffer);
        if(!is_float && json_buffer->str[json_buffer->current_idx] == '.') {
            is_float = true;
            buffer_consume(json_buffer);
        }
    }
    usize end_idx = json_buffer->current_idx;
    str->data = json_buffer->str.data + start_idx;
    str->length = end_idx - start_idx;

    return is_float;
}

/* NOTE(abid): Okay, so we've decided that it would be better to go back to the old
 * c-style string instead of this class, mainly because of how it should be used after
 * the parsing is done. One would expect all strings to be null terminated, and c-library
 * compliant. So making our own string library is @not a good idea here. 24.Sep.2024 */
internal void
jp_lexer(buffer *json_buffer, parser_state *state) {
    usize *content_bytes_size = 0;
    while(json_buffer->str[json_buffer->current_idx]) {
        buffer_consume_ignores(json_buffer);
        /* TODO(abid): Guard against using comma at the end of a scope. */
        bool expect_element = false;

        switch(json_buffer->str[json_buffer->current_idx]) {
            case '"': {
                /* TODO(abid): No support for Unicode - 24.Sep.2024 */
                // string_value *str = push_struct(string_value, memory);
                string_value *str_value = push_struct(string_value, state->arena)
                buffer_to_cstring(str_value, json_buffer);
                buffer_consume_ignores(json_buffer);
                if(json_buffer->str[json_buffer->current_idx] == ':') {
                    /* NOTE(abid): We have a key. */
                    buffer_push_token(tt_key, (void *)str_value, state);
                    buffer_consume(json_buffer);
                } else buffer_push_token(tt_value_str, (void *)str_value, state);

                content_bytes_size += str_value->length+1;
            } break;
            case '{': { 
                buffer_push_token(tt_dict_begin, content_bytes_size, state);
                buffer_consume(json_buffer); 

                content_bytes_size += sizeof(json_value) + sizeof(json_dict);
            } break;
            case '[': { 
                buffer_push_token(tt_list_begin, num_content, state);
                buffer_consume(json_buffer);

                content_bytes_size += sizeof(json_value) + sizeof(json_list);
            } break;
            case '}': { buffer_push_token(tt_dict_end,   0, state); buffer_consume(json_buffer); } break;
            case ']': { buffer_push_token(tt_list_end,   0, _state); buffer_consume(json_buffer); } break;
            case '\0': { buffer_push_token(tt_eot, 0, state); buffer_consume(json_buffer); } break;
            case '\t':
            case '\n':
            case '\r':
            case ',':
            case ' ': { buffer_consume_ignores(json_buffer); } break;
            default: {
                if(buffer_is_numeric(json_buffer)) {
                    string_value *str = push_struct(string, state);
                    bool is_float = buffer_consume_extract_numeric(str, json_buffer);
                    if(is_float) {
                        /* NOTE(abid): Float is 64-bit. */
                        buffer_push_token(tt_value_float, str, state);

                        content_bytes_size += sizeof(f64);
                    } else {
                        /* NOTE(abid): Integer is 64-bit */
                        buffer_push_token(tt_value_int, str, state);
                        content_bytes_size += sizeof(i64);
                    }
                } else assert(0, "invalid path");
            }
        }
    }

    state->current_token = state->token_list;
    state->content_bytes_size = content_bytes_size;
}

/* NOTE(abid): Parser routines. */
#define parse_err(str, ...) fprintf(stderr, "parse error: " str "\n", __VA_ARGS__)
#define parse_assert(expr, str, ...) \
    if((expr)) { } \
    else { \
        parse_err(str, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    }

inline internal void
token_expect(token *tok, token_type tok_type) {
    parse_assert(tok->type == tok_type, "expected token [%s], got [%s]\n",
                  token_type_str[tok_type], token_type_str[tok->type]);
}

inline internal void
token_advance(token **tok) { *tok = (*tok)->next; }

internal token *
token_peek(token *tok, usize forward_count) {
    token *result = tok;
    for(;forward_count != 0; --forward_count) {
        result = result->next;
    }

    return result;
}

/* TODO(abid): We already know the size of the strings, with include keys as well,
 * which means that we will be able to create @two arrays for dictionaries, one for keys
 * and another for the values to be more efficient in our memory pattern. - 24.Sep.2024 */
internal json_dict
jp_parser(parser_state *state) {
    token *current_token = state->token_list;
    /* NOTE(abid): The initial tokens must dict_begin followed by a key. */
    token_expect(current_token, tt_dict_begin);
    token_expect(token_peek(current_token, 1), tt_key);

    /* NOTE(abid): Push the entire required memory for json object at once. */
    void *json_memory = push_size(state->content_bytes_size, state->arena);
    json_dict *json_root = json_memory;

    /* NOTE(abid): Every allocation after this will be discard at its end. */
    temp_memory temp_mem = mem_temp_begin(state->arena);

    /* NOTE(abid): Current scope of the container we are in [json_list | json_dict]. */
    linked_list *current_scope = NULL;
    while(current_token->type != tt_eot) {
        switch(current_token->type) {
            case tt_dict_begin: {
                json_value *value;
                /* NOTE(abid): Check if we are at the root dictionary or not. */
                if(current_scope == NULL) value = (json_value *)json_memory;
                else value = (json_value *)current_scope->content;

                json_memory += sizeof(json_value) + sizeof(json_dict);
                dict->type = jvt_dict;
                linked_list *ll = push_struct(linked_list, state->arena);
                ll->content = value;
                ll->parent = current_scope;
                current_scope = ll;

                json_dict *dict = value + 1;
                dict->table = json_memory;
                dict->count = 0;
                dict->content_size = 0;
            } break;
            case tt_list_begin: {
                parse_assert(current_scope == NULL, "a list cannot be the first scope in JSON");
                json_value *value = (json_value *)current_scope->content;
                json_memory += sizeof(json_value) + sizeof(json_list);

                linked_list *ll = push_struct(linked_list, state->arena);
                ll->content = value;
                ll->parent = current_scope;
                current_scope = ll;

                json_list *lst = value + 1;
                dict->array = json_memory;
                dict->count = 0;
                dict->content_size = 0;
            } break;
            case tt_list_end: {
            case tt_dict_end: {
                parse_assert(current_scope != NULL, "unexpected closing of scope, did you enter an extra }/]?");
                current_scope = current_scope->parent;
            } break;
            case tt_key: {
                is_dict = ((json_value *)current_scope->content)->type == jvt_dict
                parse_assert(is_dict, "unexpected key inside a non-dictionary scope");

                string_value *string = (string_value *)current_token->body;
                memcpy(string->data, json_memory, string->length);
                string *str = (string *)current_token->body;
                token_advance(current_token);
            } break;
            case tt_value_float: {
            } break;
            case tt_value_int: {
            } break;
            case tt_value_str: {
            } break;

            default: assert(0, "invalid code path in parser");
        }
        token_advance(&current_token);
    }
    mem_temp_end(temp_mem);

    parse_assert(dict_scope_value == 0, "mismatch in json object scope");
    parse_assert(list_scope_value == 0, "mismatch in list scope");

    return parsed_json;
}

internal buffer 
read_text_file(char *Filename) {
    buffer Result = {0};
    FILE *FileHandle = NULL;
    fopen_s(&FileHandle, Filename, "rb");

    /* NOTE(abid): Get the size of the File */
    fseek(FileHandle, 0, SEEK_END);
    usize FileSize = ftell(FileHandle);
    rewind(FileHandle);

    /* NOTE(abid): Read into the start of the main memory. */
    char* Content = platform_allocate(FileSize);
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

    Result.str.data = Content;
    Result.str.length = BytesRead - 1; // -1 for the \0 at the end of the buffer
    Result.current_idx = 0;

    return Result;
}

internal void
jp_load(char *Filename) {
    buffer Buffer = read_text_file(Filename);
    parser_state state = {
        .arena = allocate_arena(Megabyte(10)),
        .token_list = NULL,
        .current_token = NULL
    };
    lexer(&Buffer, &state);
    parser_parse(&state);
}

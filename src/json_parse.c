/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  7/12/2024 5:11:40 PM                                          |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */
#include "json_parse.h"

internal usize
c_string_length(char *c_string) {
    usize result = 0;
    while(*c_string++) ++result;

    return result;
}

internal inline char
string_char(string str, usize idx) {
    assert(idx < str.length, "string index out of range");

    return str.data[idx];
}

internal string
to_string(char *c_string) {
    return (string) {
        .data = c_string,
        .length = c_string_length(c_string)
    };
}

internal inline token *
buffer_push_token(token_type token_type, void *token_value, memory *token_memory) {
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
    /* NOTE(Abid): Excluding the char_to_stop. */
    char current_char = buffer_char(json_buffer); 

    while(current_char != char_to_stop) {
        buffer_consume(json_buffer);
        current_char = buffer_char(json_buffer); 
    }
}

internal inline bool
buffer_is_ignore(buffer *json_buffer) {
    char current_char = string_char(json_buffer->str, json_buffer->current_idx);
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
buffer_consume_extract_string(string *str, buffer *json_buffer) {
    assert(buffer_char(json_buffer) == '"', "string must start with \"");
    buffer_consume(json_buffer); /* consume start quote */
    usize start_idx = json_buffer->current_idx;
    buffer_consume_until(json_buffer, '"');
    usize end_idx = json_buffer->current_idx;
    buffer_consume(json_buffer); /* consume end quote */

    str->data = json_buffer->str.data + start_idx;
    str->length = end_idx - start_idx;
}

internal inline bool
buffer_is_numeric(buffer *json_buffer) {
    char current_char = buffer_char(json_buffer);
    return ((current_char >= '0') && (current_char <= '9')) ||
           (current_char == '-') || (current_char == '+');
}

internal bool 
buffer_consume_extract_numeric(string *str, buffer *json_buffer) {
    /* NOTE(Abid): Assumes the current character is already a number. */
    bool is_float = false;

    usize start_idx = json_buffer->current_idx;
    while(buffer_is_numeric(json_buffer)) {
        buffer_consume(json_buffer);
        if(!is_float && buffer_char(json_buffer) == '.') {
            is_float = true;
            buffer_consume(json_buffer);
        }
    }
    usize end_idx = json_buffer->current_idx;
    str->data = json_buffer->str.data + start_idx;
    str->length = end_idx - start_idx;

    return is_float;
}

internal void
lexer(buffer *json_buffer, memory *parser_memory) {
    while(json_buffer->current_idx < json_buffer->str.length) {
        buffer_consume_ignores(json_buffer);
        switch(buffer_char(json_buffer)) {
            case '"': {
                string *str = push_struct(string, parser_memory);
                buffer_consume_extract_string(str, json_buffer);
                buffer_consume_ignores(json_buffer);
                if(buffer_char(json_buffer) == ':') {
                    /* NOTE(Abid): We have a key. */
                    buffer_push_token(tt_key, (void *)str, parser_memory);
                    buffer_consume(json_buffer);
                } else {
                    buffer_push_token(tt_value_str, (void *)str, parser_memory);
                }
            } break;
            case '{': { buffer_push_token(tt_dict_begin, 0, parser_memory); buffer_consume(json_buffer); } break;
            case '}': { buffer_push_token(tt_dict_end,   0, parser_memory); buffer_consume(json_buffer); } break;
            case '[': { buffer_push_token(tt_list_begin, 0, parser_memory); buffer_consume(json_buffer); } break;
            case ']': { buffer_push_token(tt_list_end,   0, parser_memory); buffer_consume(json_buffer); } break;
            case ',': { buffer_push_token(tt_comma,      0, parser_memory); buffer_consume(json_buffer); } break;
            case '\t':
            case '\n':
            case '\r':
            case ' ': { buffer_consume_ignores(json_buffer); } break;
            default: {
                if(buffer_is_numeric(json_buffer)) {
                    string *str = push_struct(string, parser_memory);
                    bool is_float = buffer_consume_extract_numeric(str, json_buffer);
                    if(is_float) buffer_push_token(tt_value_float, str, parser_memory);
                    else buffer_push_token(tt_value_int, str, parser_memory);
                } else assert(0, "invalid path");
            }
        }
    }

    parser_memory->current_token = parser_memory->token_list;
}

internal buffer 
read_text_file(char *Filename) {
    buffer Result = {0};
    FILE *FileHandle = NULL;
    fopen_s(&FileHandle, Filename, "rb");

    /* NOTE(Abid): Get the size of the File */
    fseek(FileHandle, 0, SEEK_END);
    usize FileSize = ftell(FileHandle);
    rewind(FileHandle);

    /* NOTE(Abid): Read into the start of the main memory */
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
parser(memory *parse_memory) {
    *parse_memory;
}

internal void
parse_json(char *Filename) {
    buffer Buffer = read_text_file(Filename);
    memory parse_memory = {
        .arena = allocate_arena(Megabyte(10)),
        .token_list = NULL,
        .current_token = NULL
    };
    lexer(&Buffer, &parse_memory);
    parser(&parse_memory);
}

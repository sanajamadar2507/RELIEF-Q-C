#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char* buffer;
    size_t capacity;
    size_t length;
    bool overflow;
} JsonBuffer;

void json_buf_init(JsonBuffer* jb, char* buffer, size_t capacity);
void json_buf_append(JsonBuffer* jb, const char* str);
void json_buf_append_escaped(JsonBuffer* jb, const char* str);
void json_buf_append_int(JsonBuffer* jb, int val);
void json_buf_append_bool(JsonBuffer* jb, bool val);

#endif /* JSON_UTILS_H */

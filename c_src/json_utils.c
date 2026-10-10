#include "json_utils.h"
#include <stdio.h>
#include <string.h>

void json_buf_init(JsonBuffer* jb, char* buffer, size_t capacity) {
    jb->buffer = buffer;
    jb->capacity = capacity;
    jb->length = 0;
    jb->overflow = false;
    if (capacity > 0 && buffer != NULL) {
        buffer[0] = '\0';
    }
}

void json_buf_append(JsonBuffer* jb, const char* str) {
    if (!jb || !jb->buffer || jb->overflow || !str) return;
    size_t len = strlen(str);
    if (jb->length + len >= jb->capacity) {
        jb->overflow = true;
        return;
    }
    memcpy(jb->buffer + jb->length, str, len);
    jb->length += len;
    jb->buffer[jb->length] = '\0';
}

void json_buf_append_escaped(JsonBuffer* jb, const char* str) {
    if (!jb || !jb->buffer || jb->overflow) return;
    if (!str) {
        json_buf_append(jb, "");
        return;
    }
    while (*str) {
        unsigned char c = (unsigned char)*str;
        if (c == '"') {
            json_buf_append(jb, "\\\"");
        } else if (c == '\\') {
            json_buf_append(jb, "\\\\");
        } else if (c == '\b') {
            json_buf_append(jb, "\\b");
        } else if (c == '\f') {
            json_buf_append(jb, "\\f");
        } else if (c == '\n') {
            json_buf_append(jb, "\\n");
        } else if (c == '\r') {
            json_buf_append(jb, "\\r");
        } else if (c == '\t') {
            json_buf_append(jb, "\\t");
        } else if (c < 0x20) {
            char esc[8];
            snprintf(esc, sizeof(esc), "\\u%04x", c);
            json_buf_append(jb, esc);
        } else {
            char ch[2] = { (char)c, '\0' };
            json_buf_append(jb, ch);
        }
        if (jb->overflow) return;
        str++;
    }
}

void json_buf_append_int(JsonBuffer* jb, int val) {
    char num[32];
    snprintf(num, sizeof(num), "%d", val);
    json_buf_append(jb, num);
}

void json_buf_append_bool(JsonBuffer* jb, bool val) {
    json_buf_append(jb, val ? "true" : "false");
}

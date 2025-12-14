/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON */
/* JSON parser in C - Minimal implementation for nex */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "cJSON.h"

/* Windows compatibility: strcasecmp doesn't exist on Windows */
#ifdef _WIN32
#define strcasecmp _stricmp
#endif

/* Define our own boolean type */
#ifndef true
#define true ((cJSON_bool)1)
#define false ((cJSON_bool)0)
#endif

typedef struct {
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth;
} parse_buffer;

/* Reusable error pointer */
static const char *global_error = NULL;

CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void) {
    return global_error;
}

/* Internal malloc/free */
static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free_fn)(void *ptr) = free;

static cJSON *cJSON_New_Item(void) {
    cJSON *node = (cJSON *)cJSON_malloc(sizeof(cJSON));
    if (node) {
        memset(node, 0, sizeof(cJSON));
    }
    return node;
}

CJSON_PUBLIC(void) cJSON_Delete(cJSON *item) {
    cJSON *next = NULL;
    while (item != NULL) {
        next = item->next;
        if (!(item->type & cJSON_IsReference) && (item->child != NULL)) {
            cJSON_Delete(item->child);
        }
        if (!(item->type & cJSON_IsReference) && (item->valuestring != NULL)) {
            cJSON_free_fn(item->valuestring);
        }
        if (!(item->type & cJSON_StringIsConst) && (item->string != NULL)) {
            cJSON_free_fn(item->string);
        }
        cJSON_free_fn(item);
        item = next;
    }
}

CJSON_PUBLIC(void) cJSON_free(void *object) {
    cJSON_free_fn(object);
}

/* Type checking */
CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON *item) { return (item == NULL) || ((item->type & 0xFF) == cJSON_Invalid); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_False); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_True); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON *item) { return (item != NULL) && (((item->type & 0xFF) == cJSON_True) || ((item->type & 0xFF) == cJSON_False)); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_NULL); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_Number); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_String); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_Array); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_Object); }
CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON *item) { return (item != NULL) && ((item->type & 0xFF) == cJSON_Raw); }

/* Skip whitespace */
static parse_buffer *skip_whitespace(parse_buffer *buffer) {
    if (buffer == NULL || buffer->content == NULL) return NULL;
    while (buffer->offset < buffer->length && buffer->content[buffer->offset] <= 32) {
        buffer->offset++;
    }
    return buffer;
}

/* Parse the input text to generate a number */
static cJSON_bool parse_number(cJSON *item, parse_buffer *buffer) {
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char temp[64];
    size_t i = 0;

    if (buffer == NULL || buffer->content == NULL) return false;

    for (i = 0; i < 63 && (buffer->offset + i) < buffer->length; i++) {
        temp[i] = buffer->content[buffer->offset + i];
        if (temp[i] == '\0' || temp[i] == ',' || temp[i] == ']' || temp[i] == '}' || temp[i] <= 32) break;
    }
    temp[i] = '\0';

    number = strtod((const char *)temp, (char **)&after_end);
    if (temp == after_end) return false;

    item->valuedouble = number;
    item->valueint = (int)number;
    item->type = cJSON_Number;
    buffer->offset += (size_t)(after_end - temp);
    return true;
}

/* Parse string */
static cJSON_bool parse_string(cJSON *item, parse_buffer *buffer) {
    const unsigned char *ptr = buffer->content + buffer->offset + 1;
    const unsigned char *end = ptr;
    size_t len = 0;
    char *out;

    if (buffer->content[buffer->offset] != '\"') return false;

    while (*end != '\"' && *end != '\0') {
        if (*end == '\\') end++;
        end++;
    }
    len = (size_t)(end - ptr);

    out = (char *)cJSON_malloc(len + 1);
    if (!out) return false;

    /* Copy with escape handling */
    size_t i = 0, j = 0;
    while (i < len) {
        if (ptr[i] == '\\' && i + 1 < len) {
            i++;
            switch (ptr[i]) {
                case 'n': out[j++] = '\n'; break;
                case 't': out[j++] = '\t'; break;
                case 'r': out[j++] = '\r'; break;
                case '\\': out[j++] = '\\'; break;
                case '\"': out[j++] = '\"'; break;
                default: out[j++] = ptr[i]; break;
            }
        } else {
            out[j++] = ptr[i];
        }
        i++;
    }
    out[j] = '\0';

    item->type = cJSON_String;
    item->valuestring = out;
    buffer->offset += len + 2;
    return true;
}

/* Forward declaration */
static cJSON_bool parse_value(cJSON *item, parse_buffer *buffer);

/* Parse array */
static cJSON_bool parse_array(cJSON *item, parse_buffer *buffer) {
    cJSON *child = NULL;
    cJSON *tail = NULL;

    if (buffer->content[buffer->offset] != '[') return false;
    buffer->offset++;
    buffer->depth++;

    skip_whitespace(buffer);
    if (buffer->offset < buffer->length && buffer->content[buffer->offset] == ']') {
        buffer->depth--;
        buffer->offset++;
        item->type = cJSON_Array;
        return true;
    }

    do {
        child = cJSON_New_Item();
        if (!child) return false;

        skip_whitespace(buffer);
        if (!parse_value(child, buffer)) {
            cJSON_Delete(child);
            return false;
        }

        if (tail) {
            tail->next = child;
            child->prev = tail;
        } else {
            item->child = child;
        }
        tail = child;

        skip_whitespace(buffer);
    } while (buffer->offset < buffer->length && buffer->content[buffer->offset] == ',' && buffer->offset++);

    if (buffer->offset >= buffer->length || buffer->content[buffer->offset] != ']') return false;
    buffer->depth--;
    buffer->offset++;
    item->type = cJSON_Array;
    return true;
}

/* Parse object */
static cJSON_bool parse_object(cJSON *item, parse_buffer *buffer) {
    cJSON *child = NULL;
    cJSON *tail = NULL;

    if (buffer->content[buffer->offset] != '{') return false;
    buffer->offset++;
    buffer->depth++;

    skip_whitespace(buffer);
    if (buffer->offset < buffer->length && buffer->content[buffer->offset] == '}') {
        buffer->depth--;
        buffer->offset++;
        item->type = cJSON_Object;
        return true;
    }

    do {
        child = cJSON_New_Item();
        if (!child) return false;

        skip_whitespace(buffer);
        if (!parse_string(child, buffer)) {
            cJSON_Delete(child);
            return false;
        }
        child->string = child->valuestring;
        child->valuestring = NULL;
        child->type = cJSON_Invalid;

        skip_whitespace(buffer);
        if (buffer->offset >= buffer->length || buffer->content[buffer->offset] != ':') {
            cJSON_Delete(child);
            return false;
        }
        buffer->offset++;

        skip_whitespace(buffer);
        if (!parse_value(child, buffer)) {
            cJSON_Delete(child);
            return false;
        }

        if (tail) {
            tail->next = child;
            child->prev = tail;
        } else {
            item->child = child;
        }
        tail = child;

        skip_whitespace(buffer);
    } while (buffer->offset < buffer->length && buffer->content[buffer->offset] == ',' && buffer->offset++);

    if (buffer->offset >= buffer->length || buffer->content[buffer->offset] != '}') return false;
    buffer->depth--;
    buffer->offset++;
    item->type = cJSON_Object;
    return true;
}

/* Parse value */
static cJSON_bool parse_value(cJSON *item, parse_buffer *buffer) {
    if (buffer == NULL || buffer->content == NULL || buffer->offset >= buffer->length) return false;

    skip_whitespace(buffer);

    if (buffer->offset + 4 <= buffer->length && strncmp((const char *)(buffer->content + buffer->offset), "null", 4) == 0) {
        item->type = cJSON_NULL;
        buffer->offset += 4;
        return true;
    }
    if (buffer->offset + 4 <= buffer->length && strncmp((const char *)(buffer->content + buffer->offset), "true", 4) == 0) {
        item->type = cJSON_True;
        item->valueint = 1;
        buffer->offset += 4;
        return true;
    }
    if (buffer->offset + 5 <= buffer->length && strncmp((const char *)(buffer->content + buffer->offset), "false", 5) == 0) {
        item->type = cJSON_False;
        item->valueint = 0;
        buffer->offset += 5;
        return true;
    }
    if (buffer->content[buffer->offset] == '\"') {
        return parse_string(item, buffer);
    }
    if (buffer->content[buffer->offset] == '-' || (buffer->content[buffer->offset] >= '0' && buffer->content[buffer->offset] <= '9')) {
        return parse_number(item, buffer);
    }
    if (buffer->content[buffer->offset] == '[') {
        return parse_array(item, buffer);
    }
    if (buffer->content[buffer->offset] == '{') {
        return parse_object(item, buffer);
    }

    return false;
}

CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value) {
    parse_buffer buffer = { 0, 0, 0, 0 };
    cJSON *item = NULL;

    if (value == NULL) return NULL;

    buffer.content = (const unsigned char *)value;
    buffer.length = strlen(value);
    buffer.offset = 0;
    buffer.depth = 0;

    item = cJSON_New_Item();
    if (item == NULL) return NULL;

    if (!parse_value(item, &buffer)) {
        cJSON_Delete(item);
        return NULL;
    }

    return item;
}

CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array) {
    cJSON *child = NULL;
    int size = 0;
    if (array == NULL) return 0;
    child = array->child;
    while (child != NULL) {
        size++;
        child = child->next;
    }
    return size;
}

CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index) {
    cJSON *current = NULL;
    if (array == NULL || index < 0) return NULL;
    current = array->child;
    while (current != NULL && index > 0) {
        index--;
        current = current->next;
    }
    return current;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON *object, const char *string) {
    cJSON *current = NULL;
    if (object == NULL || string == NULL) return NULL;
    current = object->child;
    while (current != NULL) {
        if (current->string != NULL && strcasecmp(current->string, string) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string) {
    cJSON *current = NULL;
    if (object == NULL || string == NULL) return NULL;
    current = object->child;
    while (current != NULL) {
        if (current->string != NULL && strcmp(current->string, string) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Create types */
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void) { cJSON *item = cJSON_New_Item(); if (item) item->type = cJSON_NULL; return item; }
CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void) { cJSON *item = cJSON_New_Item(); if (item) item->type = cJSON_True; return item; }
CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void) { cJSON *item = cJSON_New_Item(); if (item) item->type = cJSON_False; return item; }
CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool b) { return b ? cJSON_CreateTrue() : cJSON_CreateFalse(); }
CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item();
    if (item) { item->type = cJSON_Number; item->valuedouble = num; item->valueint = (int)num; }
    return item;
}
CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_String;
        item->valuestring = (char *)cJSON_malloc(strlen(string) + 1);
        if (item->valuestring) strcpy(item->valuestring, string);
    }
    return item;
}
CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void) { cJSON *item = cJSON_New_Item(); if (item) item->type = cJSON_Array; return item; }
CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void) { cJSON *item = cJSON_New_Item(); if (item) item->type = cJSON_Object; return item; }

/* Add items */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    cJSON *child = NULL;
    if (array == NULL || item == NULL) return false;
    child = array->child;
    if (child == NULL) {
        array->child = item;
    } else {
        while (child->next) child = child->next;
        child->next = item;
        item->prev = child;
    }
    return true;
}

CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (object == NULL || string == NULL || item == NULL) return false;
    item->string = (char *)cJSON_malloc(strlen(string) + 1);
    if (!item->string) return false;
    strcpy(item->string, string);
    return cJSON_AddItemToArray(object, item);
}

CJSON_PUBLIC(cJSON*) cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string) {
    cJSON *item = cJSON_CreateString(string);
    if (!cJSON_AddItemToObject(object, name, item)) { cJSON_Delete(item); return NULL; }
    return item;
}

CJSON_PUBLIC(cJSON*) cJSON_AddNumberToObject(cJSON * const object, const char * const name, const double number) {
    cJSON *item = cJSON_CreateNumber(number);
    if (!cJSON_AddItemToObject(object, name, item)) { cJSON_Delete(item); return NULL; }
    return item;
}

CJSON_PUBLIC(cJSON*) cJSON_AddBoolToObject(cJSON * const object, const char * const name, const cJSON_bool boolean) {
    cJSON *item = cJSON_CreateBool(boolean);
    if (!cJSON_AddItemToObject(object, name, item)) { cJSON_Delete(item); return NULL; }
    return item;
}

/* Remove items */
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which) {
    cJSON *c = array->child;
    while (c && which > 0) { c = c->next; which--; }
    if (!c) return NULL;
    if (c->prev) c->prev->next = c->next;
    if (c->next) c->next->prev = c->prev;
    if (c == array->child) array->child = c->next;
    c->prev = c->next = NULL;
    return c;
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which) {
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string) {
    cJSON *c = NULL;
    if (object == NULL || string == NULL) return NULL;
    c = object->child;
    while (c) {
        if (c->string && strcasecmp(c->string, string) == 0) {
            if (c->prev) c->prev->next = c->next;
            if (c->next) c->next->prev = c->prev;
            if (c == object->child) object->child = c->next;
            c->prev = c->next = NULL;
            return c;
        }
        c = c->next;
    }
    return NULL;
}

CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string) {
    cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}

CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string) {
    return cJSON_GetObjectItem(object, string) != NULL ? 1 : 0;
}

/* Print - simple implementation */
static char *print_string(const char *str) {
    size_t len = strlen(str);
    char *out = (char *)cJSON_malloc(len * 2 + 3);
    if (!out) return NULL;
    
    char *ptr = out;
    *ptr++ = '\"';
    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
            case '\\': *ptr++ = '\\'; *ptr++ = '\\'; break;
            case '\"': *ptr++ = '\\'; *ptr++ = '\"'; break;
            case '\n': *ptr++ = '\\'; *ptr++ = 'n'; break;
            case '\t': *ptr++ = '\\'; *ptr++ = 't'; break;
            case '\r': *ptr++ = '\\'; *ptr++ = 'r'; break;
            default: *ptr++ = str[i]; break;
        }
    }
    *ptr++ = '\"';
    *ptr = '\0';
    return out;
}

static char *print_value(const cJSON *item, int depth, int fmt);

static char *print_array(const cJSON *item, int depth, int fmt) {
    char **entries = NULL;
    char *out = NULL;
    size_t len = 5;
    int numentries = 0;
    cJSON *child = item->child;
    
    while (child) { numentries++; child = child->next; }
    if (!numentries) {
        out = (char *)cJSON_malloc(3);
        if (out) strcpy(out, "[]");
        return out;
    }
    
    entries = (char **)cJSON_malloc(numentries * sizeof(char *));
    if (!entries) return NULL;
    memset(entries, 0, numentries * sizeof(char *));
    
    child = item->child;
    for (int i = 0; i < numentries; i++) {
        entries[i] = print_value(child, depth + 1, fmt);
        if (entries[i]) len += strlen(entries[i]) + 2 + (fmt ? depth + 1 : 0);
        child = child->next;
    }
    
    out = (char *)cJSON_malloc(len + (fmt ? depth + 1 : 0));
    if (!out) { for (int i = 0; i < numentries; i++) if (entries[i]) cJSON_free_fn(entries[i]); cJSON_free_fn(entries); return NULL; }
    
    *out = '[';
    char *ptr = out + 1;
    if (fmt) *ptr++ = '\n';
    
    for (int i = 0; i < numentries; i++) {
        if (fmt) for (int j = 0; j <= depth; j++) *ptr++ = '\t';
        if (entries[i]) { strcpy(ptr, entries[i]); ptr += strlen(entries[i]); cJSON_free_fn(entries[i]); }
        if (i < numentries - 1) *ptr++ = ',';
        if (fmt) *ptr++ = '\n';
    }
    if (fmt) for (int j = 0; j < depth; j++) *ptr++ = '\t';
    *ptr++ = ']';
    *ptr = '\0';
    
    cJSON_free_fn(entries);
    return out;
}

static char *print_object(const cJSON *item, int depth, int fmt) {
    char **entries = NULL;
    char **names = NULL;
    char *out = NULL;
    size_t len = 7;
    int numentries = 0;
    cJSON *child = item->child;
    
    while (child) { numentries++; child = child->next; }
    if (!numentries) {
        out = (char *)cJSON_malloc(3);
        if (out) strcpy(out, "{}");
        return out;
    }
    
    entries = (char **)cJSON_malloc(numentries * sizeof(char *));
    names = (char **)cJSON_malloc(numentries * sizeof(char *));
    if (!entries || !names) { cJSON_free_fn(entries); cJSON_free_fn(names); return NULL; }
    memset(entries, 0, numentries * sizeof(char *));
    memset(names, 0, numentries * sizeof(char *));
    
    child = item->child;
    for (int i = 0; i < numentries; i++) {
        names[i] = print_string(child->string);
        entries[i] = print_value(child, depth + 1, fmt);
        if (names[i]) len += strlen(names[i]);
        if (entries[i]) len += strlen(entries[i]);
        len += 4 + (fmt ? (depth + 1) * 2 : 0);
        child = child->next;
    }
    
    out = (char *)cJSON_malloc(len);
    if (!out) {
        for (int i = 0; i < numentries; i++) { if (names[i]) cJSON_free_fn(names[i]); if (entries[i]) cJSON_free_fn(entries[i]); }
        cJSON_free_fn(names); cJSON_free_fn(entries);
        return NULL;
    }
    
    *out = '{';
    char *ptr = out + 1;
    if (fmt) *ptr++ = '\n';
    
    for (int i = 0; i < numentries; i++) {
        if (fmt) for (int j = 0; j <= depth; j++) *ptr++ = '\t';
        if (names[i]) { strcpy(ptr, names[i]); ptr += strlen(names[i]); cJSON_free_fn(names[i]); }
        *ptr++ = ':';
        if (fmt) *ptr++ = ' ';
        if (entries[i]) { strcpy(ptr, entries[i]); ptr += strlen(entries[i]); cJSON_free_fn(entries[i]); }
        if (i < numentries - 1) *ptr++ = ',';
        if (fmt) *ptr++ = '\n';
    }
    if (fmt) for (int j = 0; j < depth; j++) *ptr++ = '\t';
    *ptr++ = '}';
    *ptr = '\0';
    
    cJSON_free_fn(names);
    cJSON_free_fn(entries);
    return out;
}

static char *print_value(const cJSON *item, int depth, int fmt) {
    char *out = NULL;
    if (!item) return NULL;
    
    switch ((item->type) & 0xFF) {
        case cJSON_NULL: out = (char *)cJSON_malloc(5); if (out) strcpy(out, "null"); break;
        case cJSON_False: out = (char *)cJSON_malloc(6); if (out) strcpy(out, "false"); break;
        case cJSON_True: out = (char *)cJSON_malloc(5); if (out) strcpy(out, "true"); break;
        case cJSON_Number:
            out = (char *)cJSON_malloc(64);
            if (out) {
                if (floor(item->valuedouble) == item->valuedouble && fabs(item->valuedouble) < 1e15)
                    sprintf(out, "%.0f", item->valuedouble);
                else
                    sprintf(out, "%g", item->valuedouble);
            }
            break;
        case cJSON_String: out = print_string(item->valuestring); break;
        case cJSON_Array: out = print_array(item, depth, fmt); break;
        case cJSON_Object: out = print_object(item, depth, fmt); break;
    }
    return out;
}

CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item) {
    return print_value(item, 0, 1);
}

CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item) {
    return print_value(item, 0, 0);
}

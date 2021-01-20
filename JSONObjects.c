#include "JSONObjects.h"

struct JSONValue* parseValue(char* str) {
    size_t len = strlen(str);
    union JSONValuePtr val;
    enum JSONTypes type;
    if (*str == '"') {
        // Value is a string.
        ++str;
        char v[len - 1];
        strncpy(v, str, len - 2);
        v[len - 2] = '\0';
        val.str = &v;
        type = TYPE_string;
    } else if (strcmp("true", str) == 0) {
        // Boolean true
        bool b = true;
        val.boolean = &b;
        type = TYPE_boolean;
    } else if (strcmp("false", str) == 0) {
        // Boolean false
        bool b = false;
        val.boolean = &b;
        type = TYPE_boolean;
    } else if (strcmp("null", str) == 0) {
        // null value
        val.nilptr = NULL;
        type = TYPE_nil;
    } else if (isNumeric(str)) {
        long double num = strtold(str, NULL);
        val.number = &num;
        type = TYPE_number;
    } else if (*str == '[') {
        val.child = parseArray(str);
        type = TYPE_array;
    } else {
        return NULL;
    }
    return createJSONValue(type, val);
}

struct JSONValue* createJSONValue(enum JSONTypes type, union JSONValuePtr val) {
    struct JSONValue* value = malloc(sizeof(struct JSONValue));
    value->type = type;
    value->value = val;
    return value;
}

struct JSONValue* parseArray(char* str) {
    struct JSONValue* first;
    struct JSONValue* prev;
    // skip leading '['
    ++str;
    while (*str && *str != ']') {
        skipWhitespace(&str);
        int i = getValueLength(str);
        char sub[i + 1];
        strncpy(sub, str, i);
        sub[i] = '\0';
        struct JSONValue* val = parseValue(sub);
        printf("len: %d, str: %s\n", i, sub);
        printf("type: %s\n", JSONTypeNames[val->type]);
        if (prev) {
            prev->next = val;
            val->prev = prev;
        }
        if (!first) {
            first = val;
        }
        prev = val;
        str += i;
        ++str;
    }
    return first;
}
struct JSONValue* parseObject(char* str) {
    struct JSONValue* first;
    struct JSONValue* prev;
    // skip leading '{'
    ++str;
    while (*str && *str != '}') {
        skipWhitespace(&str);
        int i = getValueLength(str);
        char sub[i + 1];
        strncpy(sub, str, i);
        sub[i] = '\0';
        struct JSONValue* val = parseValue(sub);
        printf("len: %d, str: %s\n", i, sub);
        printf("type: %s\n", JSONTypeNames[val->type]);
        if (prev) {
            prev->next = val;
            val->prev = prev;
        }
        if (!first) {
            first = val;
        }
        prev = val;
        str += i;
        ++str;
    }
    return first;
}
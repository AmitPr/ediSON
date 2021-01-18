#include "JSONObjects.h"

struct JSONValue* parseValue(char* str) {
    return NULL;
}

struct JSONValue* createJSONValue(enum JSONTypes type,
                                   union JSONValuePtr val) {
    struct JSONValue* value = malloc(sizeof(struct JSONValue));
    value->type = type;
    value->value = val;
    return value;
}

struct JSONArray* createJSONArray(int initialSize) {
    struct JSONArray* arr = malloc(sizeof(struct JSONArray));
    arr->values = malloc(initialSize * sizeof(JSONValue));
    return arr;
}
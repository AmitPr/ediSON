#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
enum JSONTypes { V_object, V_array, V_string, V_number, V_boolean, V_nil };

union JSONValuePtr {
    struct JSONArray* array;
    struct JSONObject* obj;
    char* str;
    long double* number;
    bool* boolean;
    int* nilptr;
};
/**
 * JSONValue is a struct that contains a value that can be stored in JSONObjects
 * or JSONArrays The size of a JSONValue in memory is constant no matter what is
 * stored as the value. This is because we're only storing a pointer to the
 * actual value object, in addition to the type enum. Pointers are all the same
 * size, therefore we don't need to malloc different amounts of memory.
 */
typedef struct JSONValue {
    enum JSONTypes type;
    union JSONValuePtr value;
} JSONValue;
typedef struct JSONArray {
    struct JSONValue** values;
} JSONArray;

struct JSONValue* createJSONValue(enum JSONTypes type, union JSONValuePtr val);
struct JSONArray* createJSONArray(int initialSize);
struct JSONValue* parseValue(char** buf);
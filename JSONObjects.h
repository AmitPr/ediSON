#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "string.h"
enum JSONTypes { TYPE_object, TYPE_array, TYPE_string, TYPE_number, TYPE_boolean, TYPE_nil };
static const char *JSONTypeNames[] = {
    "object", "array", "string", "number","boolean","null",
};

union JSONValuePtr {
    struct JSONValue* child;
    char** str;
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
 * We also have a pointer to the previous and following JSONValue (for arrays), along with an 
 * optional key, which is used for JSON Objects (Dictionaries)
 */
typedef struct JSONValue {
    enum JSONTypes type;
    union JSONValuePtr value;
    struct JSONValue* next;
    struct JSONValue* prev;
    char** key;
} JSONValue;

struct JSONValue* createJSONValue(enum JSONTypes type, union JSONValuePtr val);
struct JSONValue* parseValue(char* buf);
struct JSONValue* parseArray(char* str);
struct JSONValue* parseObject(char* str);
#include <stdbool.h>
enum JSONTypes { object, array, string, number, boolean, nil };
/**
 * JSONValue is a struct that contains a value that can be stored in JSONObjects
 * or JSONArrays The size of a JSONValue in memory is constant no matter what is
 * stored as the value. This is because we're only storing a pointer to the
 * actual value object, in addition to the type enum. Pointers are all the same
 * size, therefore we don't need to malloc different amounts of memory.
 */
typedef struct JSONValue {
    enum JSONTypes type;
    union value {
        JSONObject* obj;
        char** str;
        long double* number;
        bool* boolean;
        int* nilptr;
    };
};
typedef struct JSONObject {
    char* keys[];
    JSONValue* values[];
};
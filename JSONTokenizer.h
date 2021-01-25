#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
/**
 * Used to define types of values that can be stored in JSONTokens
 */
enum JSONTypes {
    TYPE_object,
    TYPE_array,
    TYPE_string,
    TYPE_number,
    TYPE_boolean,
    TYPE_nil
};
/**
 * JSONToken used to tokenize an input JSON string. Basically used to reference
 * where in the string the Token is.
 */
typedef struct JSONToken {
    enum JSONTypes type;
    int keyStart;
    int keyEnd;
    int start;
    int end;
    struct JSONToken* next;
    struct JSONToken* child;
} JSONToken;
/**
 * Return a JSONToken that represents the root node of a JSON objects contained
 * in char** str. The Token must be freed using freeJSON(token);
 */
struct JSONToken* parseJSON(char** str);
/**
 * Used by parseJSON to recursively tokenize JSON objects.
 */
int tokenize(char** str, int* position, struct JSONToken* token);
/**
 * Used to tokenize one value (e.g, string, number, boolean, etc), or if it is
 * another array/object, call tokenize on it again.
 */
int tokenizeValue(struct JSONToken* token, char** str, int* position);
/**
 * Creates an empty JSONToken
 */
struct JSONToken* createEmptyToken();
/**
 * Print a JSON to the file descriptor in fp. This can be either stdout, or also
 * can be used by something created by open_memstream, etc.
 */
void printJSON(struct JSONToken* object, char** str, FILE* fp);
/**
 * Recursive helper for printJSON.
 */
void printHelper(struct JSONToken* object, char** str, int indent, FILE* fp);
/**
 * Helper to print indentation for printJSON
 */
void printIndent(int indent, FILE* fp);
/**
 * Set a Formatted JSON string in char** str
 */
char* getFormattedString(struct JSONToken* object, char** str);
/**
 * Free a JSONToken recursively
 */
void freeJSON(struct JSONToken* object);
/**
 * Sets the error position in a global external variable called errorLoc. Used
 * to identify error positions.
 */
void setErrorLoc(int loc);
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

struct JSONToken* parseJSON(char** str);
int tokenize(char** str, int* position, struct JSONToken* token);
int tokenizeValue(struct JSONToken* token, char** str, int* position);
struct JSONToken* createEmptyToken();
void printJSON(struct JSONToken* object, char** str, FILE* fp);
void printHelper(struct JSONToken* object, char** str, int indent, FILE* fp);
void printIndent(int indent, FILE* fp);
char* getFormattedString(struct JSONToken* object, char** str);
void freeJSON(struct JSONToken* object);
void setErrorLoc(int loc);
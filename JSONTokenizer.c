#include "JSONTokenizer.h"

struct JSONToken* parseJSON(char** str) {
    uint position = 0;
    struct JSONToken* root = createEmptyToken();
    if (!tokenize(str, &position, root)) {
        return 0;
    }
    return root;
}

int tokenize(char** str, uint* position, struct JSONToken* token) {
    skipWhitespace(str, position);
    char first = **str;
    token->start = *position;
    ++(*str);
    ++(*position);
    skipWhitespace(str, position);
    if (first == '{') {
        token->type = TYPE_object;
        struct JSONToken* last = NULL;
        while (**str != '}') {
            if (**str != '"') return 0;
            struct JSONToken* curPair = createEmptyToken();
            ++(*str);
            ++(*position);
            curPair->keyStart = *position;
            uint ret = skipKey(str, position);
            if (!ret) {
                // malformed key
                return 0;
            }
            curPair->keyEnd = ret;
            tokenizeValue(curPair, str, position);
            skipWhitespace(str, position);
            char cur = **str;
            ++(*str);
            ++(*position);
            if (cur != ',') {
                if (cur == '}') {
                    if (last) {
                        last->next = curPair;
                    } else {
                        token->child = curPair;
                    }
                    break;
                }
                return 0;
            }
            skipWhitespace(str, position);
            if (last) {
                last->next = curPair;
            } else {
                token->child = curPair;
            }
            last = curPair;
        }
    } else if (first == '[') {
        token->type = TYPE_array;
        struct JSONToken* last = NULL;
        while (**str != ']') {
            struct JSONToken* curVal = createEmptyToken();
            if (!tokenizeValue(curVal, str, position)) return 0;
            skipWhitespace(str, position);
            char cur = **str;
            ++(*str);
            ++(*position);
            if (cur != ',') {
                if (cur == ']') {
                    if (last) {
                        last->next = curVal;
                    } else {
                        token->child = curVal;
                    }
                    break;
                }
                return 0;
            }
            skipWhitespace(str, position);
            if (last) {
                last->next = curVal;
            } else {
                token->child = curVal;
            }
            last = curVal;
        }
    } else {
        return 0;
    }
    return 1;
}

struct JSONToken* createEmptyToken() {
    struct JSONToken* token = calloc(1, sizeof(struct JSONToken));
    if (token) {
        token->keyEnd = token->keyStart = -1;
        return token;
    } else {
        // ERROR ALLOCATING MEMORY
        return 0;
    }
}

int tokenizeValue(struct JSONToken* token, char** str, uint* position) {
    switch (**str) {
        // String
        case '"':
            token->type = TYPE_string;
            token->start = *position;
            ++(*str);
            ++(*position);
            bool escaped = false;
            while (true) {
                if (**str == '"')
                    if (escaped)
                        escaped = false;
                    else
                        break;
                else
                    escaped = (**str == '\\');
                ++(*str);
                ++(*position);
                if (!**str) return 0;
            }
            ++(*str);
            ++(*position);
            token->end = *position;
            break;
        // True/False
        case 't':
            token->type = TYPE_boolean;
            token->start = *position;
            if (strncmp(*str, "true", 4) == 0) {
                (*str) += 4;
                (*position) += 4;
                token->end = *position;
            } else
                return 0;
            break;
        case 'f':
            token->type = TYPE_boolean;
            token->start = *position;
            if (strncmp(*str, "false", 5) == 0) {
                (*str) += 5;
                (*position) += 5;
                token->end = *position;
            } else
                return 0;
            break;
        // Null
        case 'n':
            token->type = TYPE_nil;
            token->start = *position;
            if (strncmp(*str, "null", 4) == 0) {
                (*str) += 4;
                (*position) += 4;
                token->end = *position;
            } else
                return 0;
            break;
        case '[':
            token->type = TYPE_array;
        case '{':
            token->type = TYPE_object;
            token->start = *position;
            if (!tokenize(str, position, token)) return 0;
            token->end = *position;
            break;
        // Number, or malformed.
        default:
            break;
    }
    return 1;
}

void printJSON(struct JSONToken* object, char** str) {
    printHelper(object, str, 0);
}
void printHelper(struct JSONToken* object, char** str, int indent) {
    switch (object->type) {
        case TYPE_object:
            if (object->child) {
                printIndent(indent);
                printf("{\n");
                printHelper(object->child, str, indent + 1);
                printIndent(indent);
                printf("}\n");
            } else {
                printIndent(indent);
                printf("{}\n");
            }
            break;
        case TYPE_array:
            if (object->child) {
                printIndent(indent);
                printf("[\n");
                printHelper(object->child, str, indent + 1);
                printIndent(indent);
                printf("]\n");
            } else {
                printIndent(indent);
                printf("[]\n");
            }
            break;
        default:
            printIndent(indent);
            if (object->keyEnd != -1) {
                printf("\"%.*s\": ", object->keyEnd - object->keyStart,
                       (*str) + object->keyStart);
            }
            printf("%.*s", object->end - object->start, (*str) + object->start);
            if (object->next) {
                printf(",\n");
                printHelper(object->next, str, indent);
            } else {
                printf("\n");
            }
            break;
    }
}

void printIndent(int indent) {
    for (int i = indent; i > 0; --i) printf("\t");
}
#include "utils.h"

#define WHITESPACE " \r\n\t"
#define VAL_END_CHARS ",]}"

void skipWhitespace(char** buf) {
    while (strchr(WHITESPACE, **buf)) {
        ++(*buf);
    }
}

int getValueLength(char* buf) {
    int len = 0;
    char* chr=strchr(VAL_END_CHARS, *buf);
    while (!chr && *buf) {
        ++len;
        ++buf;
        chr=strchr(VAL_END_CHARS, *buf);
    }
    return len;
}

int isNumeric(char* str){
    if()
}
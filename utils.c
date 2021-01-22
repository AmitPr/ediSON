#include "utils.h"

#define WHITESPACE " \r\n\t"

void skipWhitespace(char** buf, uint* position) {
    while (**buf && strchr(WHITESPACE, **buf)) {
        ++(*buf);
        ++(*position);
    }
}

uint skipKey(char** buf, uint* position) {
    uint ret = NULL;
    bool escaped = false;
    while (true) {
        if (**buf == '"' && !escaped) {
            ret=*position;
            ++(*position);
            ++(*buf);
            break;
        }
        escaped = false;
        if (**buf == '\\') {
            escaped = true;
        }
        ++(*position);
        ++(*buf);
        if (!(**buf)) {
            break;
        }
    }
    skipWhitespace(buf, position);
    if (**buf != ':') {
        return NULL;
    }
    ++(*buf);
    ++(*position);
    skipWhitespace(buf, position);
    return ret;
}
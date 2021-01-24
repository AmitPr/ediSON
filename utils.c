#include "utils.h"

#define WHITESPACE " \r\n\t"

void skipWhitespace(char** buf, int* position) {
    while (**buf && strchr(WHITESPACE, **buf)) {
        ++(*buf);
        ++(*position);
    }
}

int skipKey(char** buf, int* position) {
    int ret = 0;
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
        return 0;
    }
    ++(*buf);
    ++(*position);
    skipWhitespace(buf, position);
    return ret;
}
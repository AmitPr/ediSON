#include "utils.h"

#define WHITESPACE " \r\n\t"
#define NUMERIC "-+0123456789."
#define VAL_END_CHARS ",\n\r\n "
#define OBJECT_END_CHARS "]}"

void skipWhitespace(char** buf, uint* position) {
    while (**buf && strchr(WHITESPACE, **buf)) {
        ++(*buf);
        ++(*position);
    }
}

int getValueLength(char* buf) {
    int len = 0;
    char* chr;
    bool inString = false;
    bool escaped = false;
    int childDepth = 1;
    while (true) {
        chr = strchr(VAL_END_CHARS, *buf);
        if (strchr("}]", *buf) && !inString) {
            --childDepth;
        } else if (strchr("[{", *buf) && !inString) {
            ++childDepth;
        }
        if (!childDepth || (chr && !inString && childDepth == 1)) {
            break;
        }
        // Only check for strings with " that aren't escaped, and that are in
        // this node, not child nodes.
        if (*buf == '"' && !escaped && childDepth == 1) {
            inString = !inString;
        }
        escaped = false;
        if (*buf == '\\') {
            escaped = true;
        }
        ++len;
        ++buf;
        if (!(*buf)) {
            break;
        }
    }
    return len;
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

bool isNumeric(char* str) {
    while (*str) {
        if (!strchr(NUMERIC, *str)) return false;
        ++str;
    }
    return true;
}
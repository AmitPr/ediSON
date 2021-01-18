#include <stdio.h>
#include <stdlib.h>

#include "JSONObjects.h"
#include "string.h"
#include "utils.h"

int main() {
    union JSONValuePtr data;
    data.str = "Hello World";
    struct JSONValue* test = createJSONValue(V_string, data);
    printf("%s\n", test->value);

    FILE* fp;
    char* buf = NULL;
    fp = fopen("./exampleArray.json", "rb");
    long length;
    if (fp) {
        // Go to the end of the file to get the length of the file
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = malloc(length);
        if (buf) {
            fread(buf, 1, length, fp);
        }
        fclose(fp);
    }
    if (buf) {
        skipWhitespace(&buf);
        if (*buf == '{') {
            ++buf;
            skipWhitespace(&buf);
            printf("%s:\n", buf);
        } else if (*buf == '[') {
            ++buf;
            while (*buf && *buf != ']') {
                skipWhitespace(&buf);
                int i = getValueLength(buf);
                char sub[i + 1];
                strncpy(sub, buf, i);
                sub[i] = '\0';
                printf("len: %d, str: %s\n", i, sub);
                buf += i;
                ++buf;
            }
        } else {
            printf("malformed JSON\n");
        }
    }
    return 0;
}
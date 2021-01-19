#include <stdio.h>
#include <stdlib.h>

#include "JSONObjects.h"
#include "string.h"
#include "utils.h"

int main() {
    FILE* fp;
    char* buf = NULL;
    fp = fopen("./exampleArray.json", "rb");
    long length;
    if (fp) {
        // Go to the end of the file to get the length of the file
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = (char*) malloc(length);
        if (buf) {
            fread(buf, 1, length, fp);
        }
        fclose(fp);
    }
    if (buf) {
        skipWhitespace(&buf);
        // TODO: JSON Parsing causes memory leaks because the structs are never
        // freed.
        struct JSONValue* val = parseValue(buf);
    }
    return 0;
}
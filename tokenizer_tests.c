#include <stdio.h>
#include <stdlib.h>

#include "JSONTokenizer.h"
#include "string.h"
#include "utils.h"

int main() {
    FILE* fp;
    char* buf = NULL;
    fp = fopen("./example.json", "rb");
    long length;
    if (fp) {
        // Go to the end of the file to get the length of the file
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = (char*)malloc(length + 1);
        if (buf) {
            if (!fread(buf, 1, length, fp)) {
                printf("Error reading in file.\n");
                return 1;
            }
        } else {
            printf("Error allocating memory for file buffer\n");
            return 1;
        }
        fclose(fp);
        buf[length] = '\0';
    }
    if (buf) {
        char* str = buf;
        struct JSONToken* json = parseJSON(&str);
        printJSON(json, &buf,stdout);
        freeJSON(json);
    }
    free(buf);
    return 0;
}
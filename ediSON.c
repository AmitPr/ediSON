#include <stdio.h>
#include <stdlib.h>

#include "string.h"

#define WHITESPACE " \r\n\t"

void match_whitespace(char** buf) {
    while (strchr(WHITESPACE, **buf)) {
        ++(*buf);
    }
}

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
        buf = malloc(length);
        if (buf) {
            fread(buf, 1, length, fp);
        }
        fclose(fp);
    }
    if (buf) {
        match_whitespace(&buf);
        if(buf[0]=='{'){
            ++buf;
            match_whitespace(&buf);
            printf("%s:\n",buf);
        }else{
            printf("malformed JSON\n");
        }
    }
    return 0;
}
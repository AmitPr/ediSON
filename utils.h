#include "string.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
void skipWhitespace(char** buf, uint* position);
int getValueLength(char* buf);
uint skipKey(char** buf, uint* position);
bool isNumeric(char* str);
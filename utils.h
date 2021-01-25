#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "string.h"
/**
 * Skip the whitespace at the current position in a string, incrementing
 * int *position by the number of characters skipped.
 */
void skipWhitespace(char** buf, int* position);
/**
 * Skip the key part of a JSON Object, incrementing int *position by the number
 * of characters skipped.
 */
int skipKey(char** buf, int* position);
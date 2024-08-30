#ifndef FILE_UTILITIES_H
#define FILE_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lineCount(FILE *file);

char **readFile(FILE *file, int *lineCount);

#endif
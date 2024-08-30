#include "file_utilities.h"

#define MY_SIZE 1000

int lineCount(FILE *file) {
    int count = 0;
    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            count++;
        }
    }
    rewind(file);
    return count;
}

// function to read contents of a .ml file
int readFile(char *filename) {

    FILE *file = fopen(filename, "r");
    // error checking: file does not exist
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }

    // array string needs to be large enough to hold file
    char contents[MY_SIZE];

    // TEMPORARY -- prints the contents of the file
    while (fgets(contents, MY_SIZE, file) != NULL) {
        printf("%s", contents);
    } 
    

    if (ferror(file)) {
        printf("Error: Could not read file %s\n", filename);
        fclose(file);
        return -1;
    }

    // real data type only supported - float, double and long double are real data types
    
    // closes file
    fclose(file);
    return 0;
}
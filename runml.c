/*
## THE ML LANGUAGE

1. Programs are written in text files whose names end in .ml

2. Statements are written one-per-line (with no terminating semi-colon)

3. The character '#' appearing anywhere on a line introduces a comment 
which extends until the end of that line

4. Only a single datatype is supported - real numbers, such as 2.71828

5. Variable and function names (identifiers) consist of 1..12 
lowercase alphabetic characters, such as budgie

6. Variable names do not need to be defined before being used in an expression, 
and are automatically initialised to the (real) value 0.0

7. The variable names arg0, arg1, and so on, 
provide access to the program's command-line arguments which provide real-valued numbers

8. A function must have been defined before it is called in an expression

9. Each statement in a function's body (one-per-line) is indented with a tab character

10. Functions may have zero-or-more formal parameters

11. A function's parameters and any other identifiers used in a function body are local 
to that function, and become unavailable when the function's execution completes.

## STEPS TO COMPILE AND EXECUTE AN ML PROGRAM

1. Edit a text file named, for example, program.ml
2. Pass program.ml as a command-line argument to your runml program
3. runml validateas the ml program, reporting any errors
4. runml generates C11 code in a file named, for example, ml-12345.c 
(where 12345 could be a process-ID)
5. runml uses your system's C11 compiler to compile ml-12345.c
6. runml executes the compiled C11 program ml-12345, passing any optional command-line arguments (real numbers)
7. runml removes any files that it created

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Global Variable Definitions
#define MY_SIZE 100000

// function to read contents of a .ml file
int readFile(char *filename) {

    FILE *file = fopen(filename, "r");
    // error checking: file does not exist
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return -1;


    // array string needs to be large enough to hold file
    char contents[MY_SIZE];
    while (fgets(contents, MY_SIZE, file) != NULL) {
        printf("%s", contents);
    } 
    

    if (ferror(file)) {
        printf("Error: Could not read file %s\n", filename);
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    // error checking, if no. of args is less than 2 
    if (argc < 2) {
        printf("Usage: %s <filename.ml>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];

    int status = readFile(filename);

    if (status == -1) {
        return 1;
    }
    return 0;
}



    


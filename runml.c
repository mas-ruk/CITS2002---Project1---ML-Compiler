//  CITS2002 Project 1 2024
//  Student1:   23630652    Zac Doruk Maslen
//  Student2:   24000895    Alexandra Mennie
//  Platform:   Ubuntu Linux


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

// going to include a personal header that contains commonly used commands
#include "file_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[]) {
    // error checking, if no. of args is less than 2 
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.ml>\n", argv[0]); // changed to fprintf to print to stderr instead of default data stream
        return 1;
    }

    // the name of the file is at initial argument provided
    char *filename = argv[1];

    // checks if file name is .ml
    size_t length = strlen(filename); // unsigned datatype, good for storing str length 
    if (length < 3 || strcmp(filename + length - 3, ".ml") != 0) {
        fprintf(stderr, "Error: File name must end with '.ml'\n");
        return 1;
    }



    // read the file
    int status = readFile(filename);

    if (status == -1) {
        return 1;
    }
    return 0;
}

// This is a work in progress
// genuinely kinda shit but whatevs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 50

// Function to check if a variable is found in a line with correct boundaries
int isVariableFound(const char* line, const char* variable) {
    const char* pos = line;
    size_t varLen = strlen(variable);
    
    // Loop through the line and check for the variable
    while ((pos = strstr(pos, variable))) {
        char before = (pos == line) ? ' ' : *(pos - 1);
        char after = *(pos + varLen);

        // Check if before and after are not valid variable characters
        if ((before == ' ' || before == '=' || before == '(' || before == ',' || before == ';' || before == '\0') &&
            (after == ' ' || after == '=' || after == ')' || after == ',' || after == ';' || after == '\0')) {
            return 1; // Variable found
        }
        pos += varLen; // Move forward in the line
    }
    
    return 0; // Variable not found
}

// Function to find all AssiType variables in the buffer
int findAssiTypeVariables(const char* buffer, char* variables[], int* variableCount) {
    char* lines[MAX_LINES];
    int lineCount = 0;
    
    // Copy the buffer to a modifiable string
    char* bufferCopy = strdup(buffer);
    if (!bufferCopy) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // Split the buffer into lines
    char* line = strtok(bufferCopy, "\n");
    while (line) {
        lines[lineCount++] = line;
        line = strtok(NULL, "\n");
    }

    // Iterate through lines to find AssiType variables
    for (int i = 0; i < lineCount; i++) {
        char* pos = strstr(lines[i], "AssiType");
        if (pos) {
            char varName[13]; // Variable name should only be up to 12 characters long
            sscanf(pos, "AssiType %12[^; ]", varName); // Stop at space or semicolon
            variables[*variableCount] = strdup(varName); // Store the variable
            (*variableCount)++;
        }
    }

    free(bufferCopy);
    return lineCount;
}

// Function to check variable presence in all lines
void checkVariablePresence(const char* buffer, char* variables[], int variableCount, int lineCount) {
    char* lines[MAX_LINES]; // Array to store lines

    // Copy buffer to a modifiable string
    char* bufferCopy = strdup(buffer);
    if (!bufferCopy) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    // Split buffer into lines
    char* line = strtok(bufferCopy, "\n");
    int i = 0;
    while (line) {
        lines[i++] = line;
        line = strtok(NULL, "\n");
    }

    // Check each variable in all lines
    for (int v = 0; v < variableCount; v++) {
        // Print the variable name without any trailing characters
        printf("Checking variable: %s\n", variables[v]);
        for (int i = 0; i < lineCount; i++) {
            // Check for the variable name in all lines
            if (isVariableFound(lines[i], variables[v])) {
                printf("Variable '%s' found in line %d: %s\n", variables[v], i + 1, lines[i]);
            }
        }
    }

    // Free the duplicated buffer
    free(bufferCopy);
}

int main() {
    // Sample input buffer
    const char* buffer = "float x;\n"
                         "AssiType y;\n"
                         "y = y + 1;\n"
                         "AssiType z;\n"
                         "z = 3.14;\n"
                         "AssiType a;\n"
                         "print(a);\n";

    char* variables[MAX_LINES];
    int variableCount = 0;

    // Find all AssiType variables
    int lineCount = findAssiTypeVariables(buffer, variables, &variableCount);

    // Check for presence of each AssiType variable in all lines
    checkVariablePresence(buffer, variables, variableCount, lineCount);

    // Free memory allocated for variable names
    for (int i = 0; i < variableCount; i++) {
        free(variables[i]);
    }

    return 0;
}

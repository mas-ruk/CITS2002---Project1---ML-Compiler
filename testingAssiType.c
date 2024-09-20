#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void replaceAssiType(char* buffer);

int main() {
    // Sample input buffer
    char buffer[] = 
        "AssiType x\n"
        "y <- x + 2\n"
        "AssiType z\n"
        "print z\n"
        "AssiType w\n"
        "w <- 3\n"
        "AssiType a\n"
        "print a\n"
        "AssiType b\n"
        "b <- AssiType x\n"; // This should cause a replace
    
    printf("Original Buffer:\n%s\n", buffer);
    
    // Call the function to replace AssiType
    replaceAssiType(buffer);
    
    printf("Modified Buffer:\n%s\n", buffer);

    return 0;
}

void replaceAssiType(char* buffer) {
    //check assitype exists in buffer
    if (!strstr(buffer, "AssiType")) {
        return; // no replacements needed
    }
    
    char* pos = strstr(buffer, "AssiType"); //find where "AssiType" first appears
    size_t bufferLength = strlen(buffer); // length of original buffer
    
    // create new buffer for modifed string
    char* newBuffer = malloc(bufferLength + 1); // Check this
    if (!newBuffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    size_t newBufferIndex = 0;
    char varName[13]; // var name should only be 12 long -> so 12 + 1 for null character
    
    // Iterate through the original buffer line by line
    char* line = strtok(buffer, "\n");
    while (line) {
        char* linePos = line;
        while ((pos = strstr(linePos, "AssiType"))) {
            // get variable name
            sscanf(pos, "AssiType %12s", varName);

            // check if the variable is used with an operator
            bool isInt = false;
            if (strstr(line, varName) && (strstr(line, "+") || strstr(line, "-") || strstr(line, "*") || strstr(line, "/"))) {
                isInt = true;
            }

            // copy the part of the line before "AssiType" to the new buffer
            strncpy(newBuffer + newBufferIndex, line, pos - line);
            newBufferIndex += pos - line;

            // Replace "assitype" with "int" or "float"
            if (isInt) {
                strcpy(newBuffer + newBufferIndex, "int ");
            } else {
                strcpy(newBuffer + newBufferIndex, "float ");
            }
            newBufferIndex += (isInt ? 4 : 6); // length of inserted stuff

            // move past assitype in line
            linePos = pos + strlen("AssiType");
        }

        // copy rest of line
        if (linePos && *linePos) {
            strcpy(newBuffer + newBufferIndex, linePos);
            newBufferIndex += strlen(linePos);
        }

        // add newline if not last line
        newBuffer[newBufferIndex++] = '\n';

        // get next line
        line = strtok(NULL, "\n");
    }

    // null terminate new generated buffer code
    newBuffer[newBufferIndex] = '\0';

    // copy new buffer back to og buffer
    strcpy(buffer, newBuffer);

    // Free (delete) new buffer
    free(newBuffer);
}

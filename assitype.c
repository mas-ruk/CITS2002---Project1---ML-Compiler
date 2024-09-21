#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 50
#define BUFFER_SIZE 1000

typedef struct {
    char* name;
    int operated; // 1 (true) and 0 (false)
} VarInf;

int isVarFound(const char* line, const char* variable) {
    const char* pos = line;
    size_t varLen = strlen(variable);
    while ((pos = strstr(pos, variable))) {
        char before = (pos == line) ? ' ' : *(pos - 1);
        char after = *(pos + varLen);

        if ((before == ' ' || before == '=' || before == '(' || before == ',' || before == ';' || before == '\0') &&
            (after == ' ' || after == '=' || after == ')' || after == ',' || after == ';' || after == '\0')) {
            return 1; 
            }
        pos += varLen;
        }
    return 0; 
}

int findAssi(const char* buffer, VarInf vars[], int* varCount) {
    char* lines[MAX_LINES];
    int lineCount = 0;
    
    char* bufferCopy = strdup(buffer);
    if (!bufferCopy) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    char* line = strtok(bufferCopy, "\n");
    while (line) {
        lines[lineCount++] = line;
        line = strtok(NULL, "\n");
    }

    for (int i = 0; i < lineCount; i++) {
        char* pos = strstr(lines[i], "AssiType");
        if (pos) {
            char* varName = malloc(13); // 12 letters + whitespace
            sscanf(pos, "AssiType %12[^; ]", varName);
            vars[*varCount].name = varName;
            vars[*varCount].operated = 0; 
            (*varCount)++;
        }
    }

    free(bufferCopy);
    return lineCount;
}

void checkVarPres(const char* buffer, VarInf vars[], int varCount, int lineCount) {
    char* lines[MAX_LINES];

    char* bufferCopy = strdup(buffer);
    if (!bufferCopy) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    char* line = strtok(bufferCopy, "\n");
    int i = 0;
    while (line) {
        lines[i++] = line;
        line = strtok(NULL, "\n");
    }

    for (int v = 0; v < varCount; v++) {
        for (int i = 0; i < lineCount; i++) {
            if (isVarFound(lines[i], vars[v].name)) {
                if (strstr(lines[i], "=") || strstr(lines[i], "+") || strstr(lines[i], "-") || 
                    strstr(lines[i], "*") || strstr(lines[i], "/")) {
                    vars[v].operated = 1; 
                }
            }
        }
    }

    free(bufferCopy);
}

void replAssi(const char* buffer, VarInf vars[], int varCount, char* outputBuffer) {
    strcpy(outputBuffer, buffer); 

    for (int v = 0; v < varCount; v++) {
        if (vars[v].operated) {
            char oldString[BUFFER_SIZE];
            sprintf(oldString, "AssiType %s", vars[v].name);

            char newString[BUFFER_SIZE];
            sprintf(newString, "int %s", vars[v].name);

            char* pos = outputBuffer;
            while ((pos = strstr(pos, oldString))) {
                strncpy(pos, newString, strlen(newString));
                memmove(pos + strlen(newString), pos + strlen(oldString), strlen(pos + strlen(oldString)) + 1);
                pos += strlen(newString); 
            }
        }
    }
}

void conductAssiReplace(const char* buffer) {
    VarInf vars[MAX_LINES];
    int varCount = 0;
    int lineCount = findAssi(buffer, vars, &varCount);
    checkVarPres(buffer, vars, varCount, lineCount);
    char outputBuffer[BUFFER_SIZE];
    replAssi(buffer, vars, varCount, outputBuffer);
    
    printf("%s\n", outputBuffer); 
    
    for (int i = 0; i < varCount; i++) {
        free(vars[i].name);
    }

    
}

int main() {
    const char* buffer = "#include <stdio.h>\n"
                        "AssiType x;\n"
                        "AssiType y;\n"
                        "AssiType x = 8.000000;\n"
                        "AssiType y = 3.000000;\n"
                        "int main() {\n"
                        "printf(\"%f\\n\", x*y);\n"
                        "    return 0;\n"
                        "}\n";

    conductAssiReplace(buffer);

    return 0;
}

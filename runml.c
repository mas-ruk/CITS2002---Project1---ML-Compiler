//  CITS2002 Project 1 2024
//  Student1:   23630652    Zac Doruk Maslen
//  Student2:   24000895    Alexandra Mennie
//  Platform:   Ubuntu Linux

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// defining array size to take in .ml lines
#define MY_SIZE 1000
#define MAX_NODES 1000

// initalise different token types for our lexer, values described in comments
typedef enum { 
    TknIdentifier, 
    TknNumber, 
    TknFloat, 
    TknFunction, // "function"
    TknPrint, // "print"
    TknReturn, // "return"
    TknComment, // "#"
    TknNewline, // "\n" 
    TknTab, // "TABx" where x is the number of tabs so far through perusal
    TknAssignmentOperator, // "<-"
    TknTermOperator, // "+" or "-"
    TknFactorOperator, // "*" or "/"
    TknLBracket, // "(" 
    TknRBracket, // ")"
    TknComma, // ","
    TknEnd // "END" 
} TknType;

// define structure for "Token" as a 'type' and 'value' pair
typedef struct { 
    TknType type;
    char value[100]; } // size set as 100 as we shouldn't encounter a value more than 100 bytes long  
Token;

// node types for AST 
typedef enum {
    nodeProgram, // actual root program
    nodeStmt, 
    nodeExpression,
    nodeTerm,
    nodeFactor,
    nodeFunctionCall,
    nodeFunctionDef,
    nodeParam,
    nodeReturn,
    nodeAssignment,
    nodePrint
} NodeType;

// AST Structure
typedef struct AstNode {
    NodeType type;
    union {
        // to account for lines/statements in program we need to create an overarching program node
        struct {
            struct AstNode **statements; // holds all statements in program
            int lineCount;
        } program;
        
        // statement node
        struct {
            struct AstNode *expression; // used in stmt
            struct AstNode *functionCall; 
            char *identifier; // print and assignment
        } stmt;
        
        // expression node
        struct {
            struct AstNode *lVar; // 
            char *oper; // + or -
            struct AstNode *rVar;
        } exp;
        
        // term node
        struct {
            struct AstNode *lVar;
            char *oper; // x or /
            struct AstNode *rVar;
        } term;
        
        // factor node
        struct {
            float constant; // using float as we only require 6 digits of precision
            char* identifier;
            struct AstNode *funcCall;
            struct AstNode *subExp; // expressions in parentheses
        } factor;

        // func call node
        struct {
            char* identifier;
            struct AstNode **args;
            int argCount;
        } funcCall;
        
        // func def node
        struct {
            char *identifier;
            struct AstNode **params;
            int paramCount;
            struct AstNode **body;
            int stmtCount;
        } funcDef;
        
        // assignment node
        struct {
            char* identifier; // var
            struct AstNode *exp;
        } assignment;
        
        // print
        struct {
            struct AstNode *exp;
        } print;
        
        // return
        struct {
            struct AstNode *exp;
        } rtrn;
    } data;
} AstNode;

// we didnt have a way of representing the statements each in the program so here's a node for it
struct {
    struct AstNode **statements; // array for statements
    int statementCount;
} statementSeq;

// ###################################### TOKENISATION START ######################################

// initalise "Tokens" array
Token Tokens[1000]; // array that stores all tokens generated by the lexer
int TknIndex = 0; // integer value that keeps track of our current position in array
int TknCount = 0; // stores the number of tokens in our array

// function to add tokens to our "Tokens" array
void addToken(TknType type, const char *value) { 
    Tokens[TknIndex].type = type; // sets type
    strcpy(Tokens[TknIndex].value, value); // sets value 
    TknIndex++; // increases token index/position pointer
    TknCount++; // increment token count
}

// FOR TESTING PURPOSES - print the token
void print_token(Token token) {
    printf("Token Type: %d, Value: %s\n", token.type, token.value);
}

// function to check validity of identifiers
bool isValidIdentifier(const char *str) {
    int length = strlen(str);
    if (length < 1 || length > 12) { // check length between 1 and 12
        return false;
    }
    for (int i = 0; i < length; i++) { // check all characters are lowercase and alphabetical 
        if (!islower(str[i]) || !isalpha(str[i])) {
            return false;
        }
    }
    return true;
}

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

// lexer function to convert code to token list
void tokenize(const char *code) {
    const char *pointer = code; // accesses character in code
    int IndentLevel = 0; // integer to track indent level

    while (*pointer) {

        // check for comments first (to remove them from consideration and avoid errors later on)
        if (*pointer == '#') {
            addToken(TknComment, "#");  // 

            // skip everything until the newline character
            while (*pointer && *pointer != '\n') {
                pointer++;
            }
            continue;  // skip until newline, negating whole comment line from tokens array
            }

        // check for blank spaces such as tab and newline
        if (isspace(*pointer)) { 
            if (*pointer == '\t') { // if tab exists
                char TempBuffer[100]; // creates buffer to store characters, size should not exceed 100
                sprintf(TempBuffer, "TAB%d", IndentLevel++); // string creation of value for tab token
                addToken(TknTab, TempBuffer); // adds buffer as token
            } 
            else if (*pointer == '\n') { //if newline exists
                addToken(TknNewline, "\n"); 
                IndentLevel = 0;
            }
            pointer++; // loops if ' ' appears
            continue;
            }

        // check for numbers (real constant values)
        else if (isdigit(*pointer)) { // if integer number or float (aka. realconstant) exists
            char TempBuffer[100] = {0}; // intialises all characters in buffer to 0 
            int i = 0; // iterator 
            bool hasDecimalPoint = false; // needed for syntax error checking
            
            while (isdigit(*pointer)) { // increment and extract digits before decimal point
                 TempBuffer[i++] = *pointer++; // 
            }

            if (*pointer == '.') { // handle floats
                if (hasDecimalPoint) { // check for if multiple decimal points exist
                    fprintf(stderr, "! Syntax Error: Multiple decimal points in number.\n Recommendation: Check all numbers for incorrect format.\n");
                    exit(1);
                }

                hasDecimalPoint = true;
                TempBuffer[i++] = *pointer++; // decimal point added 

                while (isdigit(*pointer)) { // increment and extract digits after decimal point
                    TempBuffer[i++] = *pointer++;
                }
                
                addToken(TknFloat, TempBuffer); // if float exists

            } else { // if number exists
                addToken(TknNumber, TempBuffer);
            }

            /* check for invalid characters after the number
            if (!isdigit(*pointer)) {
                fprintf(stderr, "! Syntax Error: Invalid character '%c' after number.\n Recommendation: Check all numbers for invalid characters. Ensure all operators, identifiers, constants and words are properly seperated by spaces. \n", *pointer);
                exit(1);
            }
            */

            // I think this may be the better way of checking for invalid characters after a number,
            // bc in ml its acceptable to find whitespaces after tokens -- but just double check LMAO

            if (!isspace(*pointer) && *pointer != '+' && *pointer != '-' && *pointer != '*' && *pointer != '/' && 
            *pointer != '(' && *pointer != ')' && *pointer != ',' && *pointer != '\0') {
                fprintf(stderr, "! Syntax Error: Invalid character '%c' after number.\nRecommendation: Ensure that numbers are followed by operators, spaces, or valid symbols.\n", *pointer);
                exit(1);
            }

        }

        // check for strings (identifiers or reserved words)
        else if (isalpha(*pointer) && islower(*pointer)) { // alphabetical lower case only
            char TempBuffer[100] = {0};
            int i = 0;
            while (isalnum(*pointer)|| *pointer == '_') { // more general isalnum() allows for us to pass invalid strings into identifier checker, meaning that this specific error can be accurately flagged
            TempBuffer[i++] = *pointer++; } 
            TempBuffer[i] = '\0'; // Null-terminate the buffer

            if (strcmp(TempBuffer, "function") == 0) { // if function keyword exists
                addToken(TknFunction, TempBuffer);
            }
            else if (strcmp(TempBuffer, "print") == 0) { // if print keyword exists
                addToken(TknPrint, TempBuffer);
            } 
            else if (strcmp(TempBuffer, "return") == 0) { // if return keyword exists
                addToken(TknReturn, TempBuffer);
            }
            else if (isValidIdentifier(TempBuffer)) { // if valid identifier exists 
                addToken(TknIdentifier, TempBuffer);
            } 
            else { // if invalid string exists
                fprintf(stderr, "! Syntax Error: Invalid characters in identifier or string.\n Recommendation: Ensure all characters are lower case. Identifiers should be alphabetical only and between 1 and 12 characters long. \n");
                exit(1);
            }
            continue;
            }             

        // check for mathematical operators
        else if (*pointer == '+' || *pointer == '-' || *pointer == '*' || *pointer == '/' || *pointer == '(' || *pointer == ')'|| *pointer == ',') { // if valid character
            char TempBuffer[2] = {*pointer, '\0'};
            TknType TknType;
                switch (*pointer) {
                    case '+':
                    case '-':
                        TknType = TknTermOperator;  // if '+' and '-' (term operators) exists
                        break;
                    case '*':
                    case '/':
                        TknType = TknFactorOperator;  // if '*' and '/' (factor operators) exists
                        break;
                    case '(':
                        TknType = TknLBracket;
                        break;
                    case ')':
                        TknType = TknRBracket;  // if '(' and ')' (brackets) exists
                        break;
                    case ',':
                        TknType = TknComma;  // if ',' (comma) exists
                        break;
        } 
        
        // was missing addToken after each case, so added here
        addToken(TknType, TempBuffer);
        pointer++;

        // check for assignment operator
        } else if (*pointer == '<' && *(pointer + 1) == '-') { // if "<-" operator exists
            addToken(TknAssignmentOperator, "<-");
            pointer += 2;
        }

        // check for all other characters
        else { 
            fprintf(stderr, "! Syntax Error: Illegal character '%c' exists in file.\n Recommendation: remove invalid symbols and all uppercase to fix. \n", *pointer); // just added what character its throwing an error for 
            exit(1);
        }
    }
}

// function to read contents of a .ml file
int readFile(char *filename) {

    // opening file for reading
    FILE *file = fopen(filename, "r");
    
    // error checking: file does not exist
    if (file == NULL) {
        fprintf(stderr, "@ Error: Could not open file %s\n", filename);
        return -1;
    }

    // buffer holds each line
    char line[MY_SIZE]; 
    
    // file reading logic
    while (fgets(line, sizeof(line), file) != NULL) {
        // remove new line char 
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // tokenize
        // printf("Line: %s\n", line); // debug
        tokenize(line);
    }

    // error checking: file is empty
    if (ferror(file)) {
        printf("@ Error: Could not read file %s\n", filename);
        fclose(file);
        return -1;
    }
    
    // closes file
    fclose(file);
    
    addToken(TknEnd, "END");

    return 0;
}

//need to add a checker that checks there are max 50 unique identifiers

// ###################################### TOKENISATION END ######################################

// ###################################### PARSING START ######################################

/* 
a parser organises tokens into a structured form based on the grammer of the language
i.e. goes through the following steps: token organisation, 
validation (if sequence of tokens forms valid constructs according to ml specs),
and then generates syntax tree
i.e. checks if an assignment is correct (i.e. x = 5 is valid but 5 = x is not)
and then generates a syntax tree based on the valid constructs 
*/

/*
program:
        ( program-item )*

program-item:
        statement
        |  function identifier ( identifier )*
        ←–tab–→ statement1
        ←–tab–→ statement2
        ....

statement:
        identifier "<-" expression
        |  print  expression
        |  return expression
        |  functioncall

expression:
        term   [ ("+" | "-")  expression ]

term:
        factor [ ("*" | "/")  term ]

factor:
        realconstant
        | identifier
        | functioncall
        | "(" expression ")"

functioncall:
        identifier "(" [ expression ( "," expression )* ] ")"
*/

AstNode nodes[MAX_NODES]; // not using malloc, static allocation
int pCurrentTknIndex = 0;
int nodeCount = 0;

// Fetch the current token
Token pCurrentTkn() {
    return Tokens[pCurrentTknIndex];
}

// here is a function to grab the next token
Token getNextTkn() {
    return Tokens[pCurrentTknIndex++];
}

// Increment token index
Token pMoveToNextTkn() {
    pCurrentTknIndex++;
}

// add new node from token to tree
AstNode* createNode(NodeType type){
    if (nodeCount < MAX_NODES) {
        AstNode *node = &nodes[nodeCount++];
        node -> type = type;
        return node;
    } else {
        fprintf(stderr, "@ Error: Maximum no. of nodes reached. Memory allocation exhausted.");
        exit(EXIT_FAILURE);
    }
}

// Array to store function names
char ExistingFunctions[50][256]; // based on max 50 unique identifiers
int FunctionsCount = 0;  // Counter for the number of functions

// Function to add function names to the array
void addFunctionName(const char* identifier) {
    if (FunctionsCount < 50) { //
        strcpy(ExistingFunctions[FunctionsCount++], identifier);
    }
}

//Function to check if function identifer within the array
bool doesFunctionExist(const char* funcID) {
    for (int i = 0; i < FunctionsCount; i++) {
        if (strcmp(ExistingFunctions[i], funcID) == 0) {
            return true; // name already exists
        }
    }
return false;
}

// void pExpression(); // letting program know that expression parser exists even if not yet defined

// void pFuncCall(); // letting program know that function call parser exists even if not yet definied

AstNode* pFactor() {
    AstNode* factorNode = NULL;
    
    // check for number existence
    if (pCurrentTkn().type == TknNumber) {
        factorNode = createNode(nodeFactor);
        factorNode -> data.factor.constant = atof(pCurrentTkn().value);
        pMoveToNextTkn();
    }
    // check for float existence
    else if (pCurrentTkn().type == TknFloat) {
        factorNode = createNode(nodeFactor);
        factorNode -> data.factor.constant = atof(pCurrentTkn().value);
        pMoveToNextTkn();
    } else if (pCurrentTkn().type == TknIdentifier) {
        // if function call
        if (doesFunctionExist(pCurrentTkn().value)) { 
            factorNode = createNode(nodeFactor);
            factorNode -> data.factor.funcCall = pFuncCall();
        }
        //not function call
        else    {
            factorNode = createNode(nodeFactor);
            factorNode -> data.factor.identifier = strdup(pCurrentTkn().value);
            pMoveToNextTkn();
                if (pCurrentTkn().type != TknNewline && pCurrentTkn().type != TknEnd) {
                printf ("! SYNTAX ERROR: Expected new line after non-function name identifier\n.");
                exit(1);
                }
            }
    } else if (pCurrentTkn().type == TknLBracket) {
        pMoveToNextTkn();
        factorNode = createNode(nodeFactor);
        // check stuff within the brackets 
        factorNode -> data.factor.subExp = pExpression();
            if(pCurrentTkn().type != TknRBracket) {
                printf("! SYNTAX ERROR: Invalid factor. Expected ')' after expression.\n");
                exit(1);
            }    
        pMoveToNextTkn(); // consume ')'
    } else {
        printf("! SYNTAX ERROR: Invalid factor. Expected functioncall, real constant, identifer or '(' expression ')'.\n.");
        exit(1);
    }
    return factorNode;
}

// creating that left right operator child tree
AstNode* pTerm(){    
    AstNode* fctrNode = pFactor(); // parse first factor
    // if is factor operator must parse recursively
    if (pCurrentTkn().type == TknFactorOperator) {
        char* oper = strdup(pCurrentTkn().value); // store oper
        pMoveToNextTkn(); // move to next token
        AstNode* rVarNode = pTerm(); // parse next token
        AstNode* termNode = createNode(nodeTerm);
        termNode -> data.term.lVar = fctrNode; // node given lVar property i.e. the left factor
        termNode -> data.term.rVar = rVarNode; // right variable 
        termNode -> data.term.oper = oper; // operator assigned as well
        return termNode; 
    } return fctrNode;
}

AstNode* pExpression(){
    AstNode* termNode = pTerm(); // parse first term
    if (pCurrentTkn().type == TknTermOperator) {
        char* oper = strdup(pCurrentTkn().value);
        pMoveToNextTkn();
        AstNode* rVarNode = pExpression();
        AstNode* exprNode = createNode(nodeExpression);
        exprNode -> data.exp.lVar = termNode;
        exprNode -> data.exp.rVar = rVarNode;
        exprNode -> data.exp.oper = oper;
        return exprNode;
    } return termNode;
}

#define MAX_ARGS 1000
AstNode* pFuncCall() {
    AstNode* funcCallNode = createNode(nodeFunctionCall);

    // Consume (EDIT: STORE) the function name
    funcCallNode -> data.funcCall.identifier = strdup(Tokens[pCurrentTknIndex - 1].value);

    // throwing errors so lets do some malloc bullcrap
    funcCallNode -> data.funcCall.args = malloc(sizeof(AstNode*) * MAX_ARGS);
    funcCallNode -> data.funcCall.argCount = 0;

    // Check for the left bracket
    if (pCurrentTkn().type == TknLBracket) {
        pMoveToNextTkn();  // Consume '('
        
        // parse parameters
        if (pCurrentTkn().type != TknRBracket) {
            // Parse the first expression or parameter
            AstNode* paramNode = pExpression();
            // ok this is a hell of a line but basically what this is doing
            // is it accesses the funcCallNode that we've created 
            // and then puts in the paramNode at the index identified by argCount as far as i can tell lmao
            // cross fingers it works LMAO
            funcCallNode -> data.funcCall.args[funcCallNode -> data.funcCall.argCount++] = paramNode;

            // Check for additional parameters separated by commas
            while (pCurrentTkn().type == TknComma) {
                pMoveToNextTkn();  // Consume ','
                AstNode* paramNode = pExpression(); // Parse the next parameter
                funcCallNode -> data.funcCall.args[funcCallNode -> data.funcCall.argCount++] = paramNode;
            }
        }
        
        // Check for the right parenthesis ')'
        if (pCurrentTkn().type != TknRBracket) {
            printf("! SYNTAX ERROR: Expected ')' after function parameters.\n");
            exit(1);
        }
        pMoveToNextTkn();  // Consume ')'
    } 
    else {
        printf("! SYNTAX ERROR: Expected '(' after functioncall.\n");
        exit(1);
    }
    return funcCallNode;
}

// NOTE: assuming all functions have unique identifiers and will not rewrite over the previous function
// Parsing over assignment/function call
void pAssOrFuncCall(){
    //check for if function exists already
    if (doesFunctionExist(pCurrentTkn().value)) { 
        pFuncCall();
    }
    else {
        pMoveToNextTkn();
    // check for assignment existance
        if (pCurrentTkn().type == TknAssignmentOperator) {
            pMoveToNextTkn();
            pExpression(); // Parse right hand side of expression
        }
        else {
        printf("! SYNTAX ERROR: Expected assignment operator '<-' after non-function name identifier.\n") ;
        exit(1);
        }
    }
}

// parsing over statements
AstNode* pStmt(){
    // turning the below into a switch case to consolidate
    /* //Check for function statement
    if (pCurrentTkn().type == TknTab) { 
        pNextToken();
        pStmt(stmtNode);
    }
    // Check for identifier  
    else if (pCurrentTkn().type == TknIdentifier) {
        pAssOrFuncCall(); 
    }
    // Check for return
    else if (pCurrentTkn().type == TknReturn) {
        pNextToken();
        AstNode* returnNode = createNode(nodeReturn);
        returnNode -> data.rtrn.exp = pExpression();
        return returnNode;
    }
    //Check for print
    else if (pCurrentTkn().type == TknPrint) {
        pNextToken();
        pExperssion(); 
    }
    else {
        printf("! SYNTAX ERROR: Unexpected token in statement. Valid statement starting arguments include an identifier, 'print' or 'return'. \n");
        exit(1);
    }
    */
    AstNode* stmtNode = createNode(nodeStmt);
    switch (pCurrentTkn().type){
        case TknIdentifier:
            if (doesFunctionExist(pCurrentTkn().value)) {
                // function call
                stmtNode -> type = nodeFunctionCall;
                stmtNode -> data.funcCall.identifier = strdup(pCurrentTkn().value);
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! NEEDS FIXING
                stmtNode -> data.funcCall = pFuncCall();
            } else {
                // assignment
                pMoveToNextTkn();
                stmtNode -> type = nodeAssignment;
                stmtNode -> data.assignment.identifier = strdup(pCurrentTkn().value);
                pMoveToNextTkn(); // eat <-
                stmtNode -> data.assignment.exp = pExpression();
            }
            break;
        case TknPrint:
            pMoveToNextTkn(); // eat print nom nom nom 
            stmtNode -> type = nodePrint;
            stmtNode -> data.print.exp = pExpression();
            break;
        case TknReturn:
            pMoveToNextToken();
            stmtNode -> type = nodeReturn;
            stmtNode -> data.rtrn.exp = pExpression();
            break;
        default:
            // error rip
            printf("! SYNTAX ERROR: unexpected token lmao. valid statement starting args include print, return and function calls.");
    }
    return stmtNode;
}

// parsing over a function definition
AstNode* pFuncDef() {
        pMoveToNextTkn();  // Go to next token
        if (pCurrentTkn().type == TknIdentifier) {
            if (doesFunctionExist(pCurrentTkn().value)) {  // check for if function already exists
                printf("! SYNTAX ERROR: Function name '%s' already definied\n", pCurrentTkn().value);
                exit(1);
            }
            addFunctionName(pCurrentTkn().value); // Store function name
            pMoveToNextTkn();
            
            // Expecting open bracket
            if (pCurrentTkn().type == TknLBracket) {
                pMoveToNextTkn();  

                //check for closing bracket (empty parameter lsit)
                if (pCurrentTkn().type == TknRBracket) { // expect closed bracket
                    pMoveToNextTkn();  // void function exists here
                } else {
                // loop through function parameters (Id, comma, Id, comma...)
                while (pCurrentTkn().type == TknIdentifier) {
                    pMoveToNextTkn();  
                    
                    if (pCurrentTkn().type == TknComma) { // expect comma
                        pMoveToNextTkn(); 
                        if (pCurrentTkn().type != TknIdentifier) { // if non identifier appears
                        printf("! SYNTAX ERROR: Expected identifier after ',' in function argument\n");
                        exit(1);
                        }
                    }
                    else if (pCurrentTkn().type == TknRBracket) {
                        pMoveToNextTkn(); // function parameter list ends 
                            if (pCurrentTkn().type == TknNewline) {
                                pMoveToNextTkn();
                                pStmt(); // run stmt for next lines
                                return;
                            }
                            else {
                                printf("! SYNTAX ERROR: Expected newline after function argument closed aka. ')' \n");
                                exit(1);
                            }
                    } else {
                        printf("! SYNTAX ERROR: Expected ',' or ')' after function argument\n");
                        exit(1);
                    }
                        
            } 
                printf("! SYNTAX ERROR: Expected ')' after function argument\n");
                exit(1);
        }
        } else {
            printf("! SYNTAX ERROR: Expected '(' after function name\n");
            exit(1);
        }
    } 
    else {
        printf("! SYNTAX ERROR: Expected identifier for function name\n");
        exit(1);
    }
}

AstNode* pProgram() {
    AstNode* programNode = createNode(nodeProgram);
    
    // line count
    programNode -> data.program.lineCount = 0;

    // parsing over program
    while (pCurrentTkn().type != TknEnd) {
        AstNode* stmtNode = pStmt();
        if (stmtNode != NULL) {
            programNode -> data.program.statements[programNode->data.program.lineCount++] = stmtNode;
        }
        pMoveToNextTkn();
    }
    return programNode;
}
// ###################################### PARSING END ######################################

// ###################################### TRANSLATION TO C START ######################################
/*
coming into the translation to the C section will be an AST
i.e. for the assignment x <- 5 + 3, the AST would look like:
Assignment("x", 
    Operator(operAdd, 
        Number(5), 
        Number(3)
    )
)

*/
void writeCFile() {
    FILE *cFile = fopen("mlProgram.c", "w");
    if (cFile == NULL) {
        fprintf(stderr, "! Error: Could not create C file\n");
        return;
    }

    // writing to C file
    fprintf(cFile, "#include <stdio.h>\n\n");
    fprintf(cFile, "int main() {\n");
    fprintf(cFile, "toC(cFile, root);");
    fprintf(cFile, "    return 0;\n");
    fprintf(cFile, "}\n");

    fclose(cFile);
}

// defining translation to C function
void toC(AstNode* node, FILE *cFile) {
    // first we gotta check if the node is existing
    if (!node) {
        return;
    }

    // define a switch case to accomodate the types of nodes
    switch (node -> type){ // accessing node types
        case nodeAssignment:
            // variable assignment
            fprintf(cFile, "%s = ", node -> data.assignment.identifier); // print variable name
            toC(node->data.assignment.value, cFile); // 
            fprintf(cFile, "\n");
            break;
        case nodeFunctionCall:
            fprintf(cFile, "%s(", node -> data.functionCall.name);
            break;
        case nodeFunctionDef:
            break;
        case nodeIdentifier:
            fprintf(cFile, "%s", node -> data.identifier);
            toC(node->data.identifier, cFile);
            break;
        case nodeNumber:
            break;
        case nodePrint:
            break;
        case nodeReturn:
            break;
        case nodeOperator:
            break;
    }


}

void compileAndRunInC() {
    system("gcc -o mlProgram mlProgram.c");
    system("./mlProgram");
}

// function to remove created C file and exec file
void cleanupAfterExec() {
    remove("mlProgram.c");
    remove("mlProgram");
}

// ###################################### TRANSLATION TO C END ######################################

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
        fprintf(stderr, "! Error: File name must end with '.ml'\n");
        return 1;
    }

    // read the file
    int status = readFile(filename); // also doing tokenisation

    if (status == -1) {
        return 1;
    }

    // debugging tokens
    printf("Tokens after tokenisation\n");
    for (int i = 0; i < TknIndex; i++) {
        print_token(Tokens[i]);
    }

    // parsing
    parser();

    // translation to C
    writeCFile();
    compileAndRunInC();
    cleanupAfterExec();

    return 0;
}

/**
 * Project 2a
 * Assembler for LC-2K with Object File Generation
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
//Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000
#define MAX_SYMBOLS 1000
#define MAX_RELOCATIONS 1000

typedef struct {
    char label[MAXLINELENGTH];
    char type;
    int address;
    char section;
} LabelStruct;
typedef struct {
    char label[MAXLINELENGTH];
    char type;
    int address;
} SymbolTableStruct;
typedef struct {
    int section;
    int lineOffset;
    char opcode[MAXLINELENGTH];
    char label[MAXLINELENGTH];
} RelocationStruct;
LabelStruct labels[MAX_SYMBOLS];
int numLabels = 0;
SymbolTableStruct symbolTable[MAX_SYMBOLS];
int numSymbols = 0;
RelocationStruct relocationTable[MAX_RELOCATIONS];
int numRelocations = 0;
int textSection[MAXLINELENGTH];
int dataSection[MAXLINELENGTH];
int numText = 0, numData = 0;

int readAndParse(FILE *, char *, char *, char *, char *, char *);
int labelFinder(char *label);
int symbolFinder(char *label);
void addRelocation(int section, int lineOffset, char *opcode, char *label);
static inline int isNumber(char *);
static inline void printHexToFile(FILE *, int);
static inline int validReg(char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static int lineIsBlank(char *line);

int main(int argc, char **argv) {
    char *inFileStr, *outFileStr;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileStr = argv[1];
    outFileStr = argv[2];

    inFilePtr = fopen(inFileStr, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileStr);
        exit(1);
    }
    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);
    
    outFilePtr = fopen(outFileStr, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileStr);
        exit(1);
    }

    
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {// First pass
        if (opcode[0] =='\0') continue;
        if (label[0] != '\0') {
            for (int i = 0; i < numLabels; i++) {
                if (strcmp(labels[i].label, label) == 0) {
                    printf("error: duplicate label %s\n", label);
                    exit(1);
                }
            }
            char type;
            if (label[0] >= 'A' && label[0] <= 'Z') {
                type = 'G';
            } else {
                type ='L';
            }
            char section;
            if (strcmp(opcode,".fill") == 0) {
                section = 'D';
            } else {
                section = 'T';
            }
            int address;
            if (section == 'T') {
                address = numText;
            } else {
                address = numData;
            }
            strcpy(labels[numLabels].label, label);
            labels[numLabels].type = type;
            labels[numLabels].address = address;
            labels[numLabels].section = section;
            numLabels++;
        }
        if (strcmp(opcode, ".fill") == 0) {
            numData++;
        } else {
            numText++;
        }
    }
    rewind(inFilePtr);
    
    int textLine = 0, dataLine = 0; // Second pass (something is wrong in here)
    while (readAndParse(inFilePtr, label,opcode, arg0, arg1, arg2)) {
        if (opcode[0] == '\0') continue;
        int regA, regB, destReg, offset = 0, mCode = 0;

        if (label[0] != '\0' && label[0]>= 'A' && label[0] <= 'Z') {
            int symbolIndex = symbolFinder(label);
            if (symbolIndex == -1) {
                strcpy(symbolTable[numSymbols].label, label);
                if (strcmp(opcode, ".fill") == 0) {
                    symbolTable[numSymbols].type = 'D';
                } else {
                    symbolTable[numSymbols].type ='T';
                }
                if (strcmp(opcode,".fill") == 0) {
                    symbolTable[numSymbols].address = dataLine;
                } else {
                    symbolTable[numSymbols].address = textLine;
                }
                numSymbols++;
            } else {
                if (strcmp(opcode, ".fill") == 0) {
                    symbolTable[symbolIndex].type ='D';
                } else {
                    symbolTable[symbolIndex].type = 'T';
                }
                if (strcmp(opcode, ".fill") == 0) {
                    symbolTable[symbolIndex].address= dataLine;
                } else {
                    symbolTable[symbolIndex].address = textLine;
                }
            }
        }
        if (strcmp(opcode,".fill") == 0) {
            if (isNumber(arg0)) {
                mCode = atoi(arg0);
            } else {
                int labelIndex = labelFinder(arg0);
                if (labelIndex != -1) {
                    if (labels[labelIndex].type =='L') {
                        mCode = labels[labelIndex].address;
                        if (labels[labelIndex].section == 'D') {
                            mCode += numText;
                        }
                    } else {
                        mCode = 0; 
                        addRelocation(1, dataLine, ".fill", arg0);
                        if (symbolFinder(arg0) == -1) {
                            strcpy(symbolTable[numSymbols].label,arg0);
                            symbolTable[numSymbols].type ='U';
                            symbolTable[numSymbols].address = 0;
                            numSymbols++;
                        }
                    }
                } else {
                    if (arg0[0] >= 'a' && arg0[0] <= 'z') {
                        printf("error: undefined label %s\n", arg0);
                        exit(1);
                    } else {
                        mCode = 0; 
                        addRelocation(1, dataLine, ".fill", arg0);
                        if (symbolFinder(arg0) == -1) {
                            strcpy(symbolTable[numSymbols].label,arg0);
                            symbolTable[numSymbols].type ='U';
                            symbolTable[numSymbols].address = 0;
                            numSymbols++;
                        }
                    }
                }
            }
            dataSection[dataLine++] = mCode;
        } else {
            if (strcmp(opcode,"add") == 0 || strcmp(opcode, "nor") == 0) {
                if (!validReg(arg0)||!validReg(arg1) || !validReg(arg2)) {
                    printf("%s\n", "error: invalid reg number");
                    exit(1);
                }
                regA = atoi(arg0);
                regB = atoi(arg1);
                destReg = atoi(arg2);
                int op;
                if (strcmp(opcode,"add") == 0) {
                    op = 0;
                } else {
                    op= 1;
                }
                mCode = (op << 22) | (regA << 19)| (regB << 16) | destReg;
            } else if (strcmp(opcode, "lw") == 0 || strcmp(opcode, "sw") == 0) {
                if (!validReg(arg0) || !validReg(arg1)) {
                    printf("%s\n", "error: invalid reg number");
                    exit(1);
                }
                regA = atoi(arg0);
                regB = atoi(arg1);
                int op;
                if (strcmp(opcode, "lw") == 0) {
                    op = 2;
                } else {
                    op = 3;
                }
                if (isNumber(arg2)) {
                    offset = atoi(arg2);
                } else {
                    int labelIndex = labelFinder(arg2);
                    if (labelIndex != -1) {
                        if (labels[labelIndex].type =='L') {
                            offset = labels[labelIndex].address;
                            if (labels[labelIndex].section == 'D') {
                                offset += numText;
                            }
                        } else {
                            offset = 0;
                            addRelocation(0, textLine, opcode, arg2);
                            if (symbolFinder(arg2) == -1) {
                                strcpy(symbolTable[numSymbols].label, arg2);
                                symbolTable[numSymbols].type = 'U';
                                symbolTable[numSymbols].address = 0;
                                numSymbols++;
                            }
                        }
                    } else {
                        if (arg2[0] >= 'a'&& arg2[0] <= 'z') {
                            printf("error: undefined label %s\n", arg2);
                            exit(1);
                        } else {
                            offset = 0;
                            addRelocation(0, textLine, opcode, arg2);
                            if (symbolFinder(arg2) == -1) {
                                strcpy(symbolTable[numSymbols].label, arg2);
                                symbolTable[numSymbols].type = 'U';
                                symbolTable[numSymbols].address = 0;
                                numSymbols++;
                            }
                        }
                    }
                }
                if (offset < -32768|| offset > 32767) {
                    printf("%s\n", "error: offset not in range");
                    exit(1);
                }
                mCode = (op << 22) | (regA << 19) | (regB << 16) | (offset & 0xFFFF);
            } else if (strcmp(opcode, "beq") == 0) {
                if (!validReg(arg0) || !validReg(arg1)) {
                    printf("%s\n", "error: invalid reg number");
                    exit(1);
                }
                regA = atoi(arg0);
                regB = atoi(arg1);
                if (isNumber(arg2)) {
                    offset = atoi(arg2);
                } else {
                    int labelIndex = labelFinder(arg2);
                    if (labelIndex != -1) {
                        if (labels[labelIndex].type == 'G') {
                            printf("%s\n", "error: beq undefined symbolic address");
                            exit(1);
                        } else {
                            offset = labels[labelIndex].address - (textLine + 1);
                        }
                    } else {
                        printf("error: undefined label %s\n", arg2);
                        exit(1);
                    }
                }
                if (offset < -32768 || offset > 32767) {
                    printf("%s\n", "error: offset not in range");
                    exit(1);
                }
                mCode = (4 << 22) | (regA << 19) | (regB << 16) | (offset & 0xFFFF);
            } else if (strcmp(opcode, "jalr") == 0) {
                if (!validReg(arg0) || !validReg(arg1)) {
                    printf("%s\n", "error: invalid reg number");
                    exit(1);
                }
                regA = atoi(arg0);
                regB = atoi(arg1);
                mCode = (5 << 22) |(regA << 19) | (regB << 16);
            } else if (strcmp(opcode, "halt") == 0) {
                mCode = (6 << 22);
            } else if (strcmp(opcode,"noop") == 0) {
                mCode = (7 << 22);
            } else {
                printf("%s\n", "error: unrecognized opcode");
                exit(1);
            }
            textSection[textLine++] = mCode;
        }
    }

    fprintf(outFilePtr, "%d %d %d %d\n", numText, numData, numSymbols, numRelocations);
    for (int i = 0; i < numText; i++) {
        printHexToFile(outFilePtr, textSection[i]);
    }
    for (int i = 0; i < numData;i++) {//data secion
        printHexToFile(outFilePtr, dataSection[i]);
    }
    for (int i = 0; i < numSymbols; i++) {//symbol table
        fprintf(outFilePtr, "%s %c %d\n", symbolTable[i].label, symbolTable[i].type, symbolTable[i].address);
    }
    for (int i= 0; i < numRelocations; i++) {//relocaton table
        int lineOffset = relocationTable[i].lineOffset;
        char *opcode = relocationTable[i].opcode;
        char *label = relocationTable[i].label;
        fprintf(outFilePtr, "%d %s %s\n", lineOffset, opcode, label);
    }

    fclose(inFilePtr);
    fclose(outFilePtr);
    return 0;
}

int symbolFinder(char *label) {//findin symbol in table
    for (int i = 0; i < numSymbols; i++) {
        if (strcmp(symbolTable[i].label, label) == 0) {
            return i;
        }
    }
    return -1;
}

int labelFinder(char *label) {//finding label in labels array
    for (int i = 0; i < numLabels; i++) {
        if (strcmp(labels[i].label, label) == 0) {
            return i;
        }
    }
    return -1;
}


void addRelocation(int section,int lineOffset, char *opcode, char *label) {//adding relocation entry
    relocationTable[numRelocations].section = section;
    relocationTable[numRelocations].lineOffset = lineOffset;
    strcpy(relocationTable[numRelocations].opcode, opcode);
    strcpy(relocationTable[numRelocations].label, label);
    numRelocations++;
}

// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line) {
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for(int line_idx=0; line_idx < strlen(line); ++line_idx) {
        int line_char_is_whitespace = 0;
        for(int whitespace_idx = 0; whitespace_idx < 4; ++ whitespace_idx) {
            if(line[line_idx] == whitespace[whitespace_idx]) {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if(!line_char_is_whitespace) {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}
// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr) {
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);
    for(int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address) {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH-1) {
            printf("error: line too long\n");
            exit(1);
        }
        // Check for blank line.
        if(lineIsBlank(line)) {
            if(!blank_line_encountered) {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        } else {
            if(blank_line_encountered) {
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}
/*
* NOTE: The code defined below is not to be modifed as it is implimented correctly.
*/
/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;
    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';
    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
        /* reached end of file */
        return(0);
    }
    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH-1) {
        printf("error: line too long\n");
        exit(1);
    }
    // Ignore blank lines at the end of the file.
    if(lineIsBlank(line)) {
        return 0;
    }
    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
        /* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }
    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);
    return(1);
}
static inline int 
isNumber(char *string)
{
    int num;
    char c;
    return((sscanf(string, "%d%c",&num, &c)) == 1);
}
// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}
static inline int validReg(char *string) {
    char *endptr;
    long val = strtol(string, &endptr, 10);
    return (*endptr == '\0' && val >= 0 && val <= 7);
}

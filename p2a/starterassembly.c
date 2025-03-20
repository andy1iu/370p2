/**
 * Project 2A
 * Assembler code for LC-2K with support for object files
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Maximum line length and number of lines
#define MAXLINELENGTH 1000
#define MAXLINES 1000
#define MAXLABELS 1000

// Structure for symbols in the symbol table
typedef struct {
    char name[MAXLINELENGTH];
    int address;      // Line offset in Text or Data section
    char section;     // 'T', 'D', or 'U'
    bool isGlobal;
    bool isDefined;
} Symbol;

// Structure for local labels
typedef struct {
    char name[MAXLINELENGTH];
    int address;      // Line offset in Text or Data section
    char section;     // 'T' or 'D'
} LocalLabel;

// Structure for relocation entries
typedef struct {
    int offset;           // Line offset in Text or Data section
    char section;         // 'T' or 'D'
    char opcode[MAXLINELENGTH];
    char label[MAXLINELENGTH];
} RelocationEntry;

// Function prototypes
int readAndParse(FILE *, char *, char *, char *, char *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);
static inline void printHexToFile(FILE *, int);
static inline int validReg(char *);
bool isGlobalLabel(char *label);
void addSymbol(char *label, int address, char section, bool isDefined);
void addLocalLabel(char *label, int address, char section);
bool isLocalLabelDefined(char *label);
int getLocalLabelAddress(char *label);
bool usesSymbolicAddress(char *opcode);
bool isLabelDefined(char *label);
void addRelocationEntry(int offset, char section, char *opcode, char *label);
int assembleInstruction(char *opcode, char *arg0, char *arg1, char *arg2, int address, bool dataSection);
int assembleData(char *arg);
void writeObjectFile(FILE *outFilePtr, int numInstructions, int numData);

// Global variables
Symbol symbolTable[MAXLABELS];
int numSymbols = 0;

LocalLabel localLabels[MAXLABELS];
int numLocalLabels = 0;

int instructions[MAXLINES];
int numInstructions = 0;

int data[MAXLINES];
int numData = 0;

RelocationEntry relocationTable[MAXLINES];
int numRelocations = 0;

int
main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <object-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // Variables to keep track of the current address and section
    int address = 0;
    bool dataSection = false;

    // First pass: Collect labels and determine the separation point
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        // Check if we have reached the Data section
        if (!dataSection && !strcmp(opcode, ".fill")) {
            dataSection = true;
        } else if (dataSection && strcmp(opcode, ".fill") != 0) {
            // Error: Instructions after data
            printf("Error: Instructions found after data section.\n");
            exit(1);
        }

        // Handle labels
        if (label[0] != '\0') {
            if (isGlobalLabel(label)) {
                // Process global label
                addSymbol(label, dataSection ? numData : numInstructions, dataSection ? 'D' : 'T', true);
            } else {
                // Process local label
                addLocalLabel(label, dataSection ? numData : numInstructions, dataSection ? 'D' : 'T');
            }
        }

        // Increment address
        if (dataSection) {
            numData++;
        } else {
            numInstructions++;
        }
        address++;
    }

    // Second pass: Assemble instructions and data
    rewind(inFilePtr);

    address = 0;
    dataSection = false;
    numInstructions = 0;
    numData = 0;

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        // Check if we have reached the Data section
        if (!dataSection && !strcmp(opcode, ".fill")) {
            dataSection = true;
        }

        // Assemble instructions or data
        if (strcmp(opcode, ".fill") == 0) {
            // Handle data
            int value = assembleData(arg0);
            data[numData++] = value;
        } else {
            // Handle instruction
            int mCode = assembleInstruction(opcode, arg0, arg1, arg2, address, dataSection);
            instructions[numInstructions++] = mCode;
        }

        address++;
    }

    // Write the object file
    writeObjectFile(outFilePtr, numInstructions, numData);

    fclose(inFilePtr);
    fclose(outFilePtr);

    return(0);
}

// Functions implementation

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

// Check if a label is global
bool isGlobalLabel(char *label) {
    return (label[0] >= 'A' && label[0] <= 'Z');
}

// Add a symbol to the symbol table
void addSymbol(char *label, int address, char section, bool isDefined) {
    // Check for duplicate symbols
    for (int i = 0; i < numSymbols; i++) {
        if (strcmp(symbolTable[i].name, label) == 0) {
            if (symbolTable[i].isDefined && isDefined) {
                printf("Error: Duplicate label %s\n", label);
                exit(1);
            } else if (isDefined) {
                // Update symbol as defined
                symbolTable[i].address = address;
                symbolTable[i].section = section;
                symbolTable[i].isDefined = true;
                return;
            } else {
                return;
            }
        }
    }
    // Add new symbol
    strcpy(symbolTable[numSymbols].name, label);
    symbolTable[numSymbols].address = address;
    symbolTable[numSymbols].section = section;
    symbolTable[numSymbols].isGlobal = true;
    symbolTable[numSymbols].isDefined = isDefined;
    numSymbols++;
}

// Add a local label
void addLocalLabel(char *label, int address, char section) {
    // Check for duplicate local labels
    for (int i = 0; i < numLocalLabels; i++) {
        if (strcmp(localLabels[i].name, label) == 0) {
            printf("Error: Duplicate local label %s\n", label);
            exit(1);
        }
    }
    // Add local label
    strcpy(localLabels[numLocalLabels].name, label);
    localLabels[numLocalLabels].address = address;
    localLabels[numLocalLabels].section = section;
    numLocalLabels++;
}

// Check if a local label is defined
bool isLocalLabelDefined(char *label) {
    for (int i = 0; i < numLocalLabels; i++) {
        if (strcmp(localLabels[i].name, label) == 0) {
            return true;
        }
    }
    return false;
}

// Get the address of a local label
int getLocalLabelAddress(char *label) {
    for (int i = 0; i < numLocalLabels; i++) {
        if (strcmp(localLabels[i].name, label) == 0) {
            return localLabels[i].address;
        }
    }
    // Should not reach here if checks are done correctly
    printf("Error: Local label %s not found\n", label);
    exit(1);
}

// Check if opcode uses a symbolic address
bool usesSymbolicAddress(char *opcode) {
    return (strcmp(opcode, "lw") == 0 ||
            strcmp(opcode, "sw") == 0 ||
            strcmp(opcode, "beq") == 0 ||
            strcmp(opcode, ".fill") == 0);
}

// Check if a label is defined (global)
bool isLabelDefined(char *label) {
    for (int i = 0; i < numSymbols; i++) {
        if (strcmp(symbolTable[i].name, label) == 0) {
            return symbolTable[i].isDefined;
        }
    }
    return false;
}

// Add a relocation entry
void addRelocationEntry(int offset, char section, char *opcode, char *label) {
    relocationTable[numRelocations].offset = offset;
    relocationTable[numRelocations].section = section;
    strcpy(relocationTable[numRelocations].opcode, opcode);
    strcpy(relocationTable[numRelocations].label, label);
    numRelocations++;
}

// Assemble an instruction
int assembleInstruction(char *opcode, char *arg0, char *arg1, char *arg2, int address, bool dataSection) {
    int regA = 0, regB = 0, destReg = 0, offset = 0;
    int mCode = 0;

    // Validate registers
    if (!strcmp(opcode, "add") || !strcmp(opcode, "nor")) {
        if (!validReg(arg0) || !validReg(arg1) || !validReg(arg2)) {
            printf("Error: Invalid register number\n");
            exit(1);
        }
        regA = atoi(arg0);
        regB = atoi(arg1);
        destReg = atoi(arg2);
        if (!strcmp(opcode, "add")) {
            mCode = (0 << 22) | (regA <<19) | (regB << 16) | destReg;
        } else if (!strcmp(opcode, "nor")) {
            mCode = (1 << 22) | (regA << 19) | (regB << 16) | destReg;
        }
    } else if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw") || !strcmp(opcode, "beq")) {
        if (!validReg(arg0) || !validReg(arg1)) {
            printf("Error: Invalid register number\n");
            exit(1);
        }
        regA = atoi(arg0);
        regB = atoi(arg1);

        if (isNumber(arg2)) {
            offset = atoi(arg2);
        } else {
            // Handle symbolic addresses
            if (isGlobalLabel(arg2)) {
                // If undefined, use 0 and add to relocation table
                if (!isLabelDefined(arg2)) {
                    addSymbol(arg2, 0, 'U', false);
                }
                // Add to relocation table if not beq
                if (strcmp(opcode, "beq") != 0) {
                    addRelocationEntry(address, 'T', opcode, arg2);
                    offset = 0;
                } else {
                    // beq cannot use undefined global labels
                    printf("Error: beq cannot use undefined global label %s\n", arg2);
                    exit(1);
                }
            } else {
                // Local label must be defined
                if (!isLocalLabelDefined(arg2)) {
                    printf("Error: Undefined local label %s\n", arg2);
                    exit(1);
                }
                int labelAddress = getLocalLabelAddress(arg2);
                if (!strcmp(opcode, "beq")) {
                    offset = labelAddress - address - 1;
                } else {
                    offset = labelAddress;
                    // Add to relocation table
                    addRelocationEntry(address, 'T', opcode, arg2);
                }
            }
        }
        if (offset < -32768 || offset > 32767) {
            printf("Error: Offset out of range: %d\n", offset);
            exit(1);
        }
        offset = offset & 0xFFFF;
        if (!strcmp(opcode, "lw")) {
            mCode = (2 << 22) | (regA << 19) | (regB << 16) | offset;
        } else if (!strcmp(opcode, "sw")) {
            mCode = (3 << 22) | (regA << 19) | (regB << 16) | offset;
        } else if (!strcmp(opcode, "beq")) {
            mCode = (4 << 22) | (regA << 19) | (regB << 16) | offset;
        }
    } else if (!strcmp(opcode, "jalr")) {
        if (!validReg(arg0) || !validReg(arg1)) {
            printf("Error: Invalid register number\n");
            exit(1);
        }
        regA = atoi(arg0);
        regB = atoi(arg1);
        mCode = (5 << 22) | (regA << 19) | (regB << 16);
    } else if (!strcmp(opcode, "halt")) {
        mCode = (6 << 22);
    } else if (!strcmp(opcode,"noop")) {
        mCode = (7<< 22);
    } else {
        printf("Error: Unrecognized opcode %s\n", opcode);
        exit(1);
    }

    return mCode;
}

// Assemble data (.fill)
int assembleData(char *arg) {
    int value = 0;
    if (isNumber(arg)) {
        value = atoi(arg);
    } else {
        // Handle symbolic address
        if (isGlobalLabel(arg)) {
            if (!isLabelDefined(arg)) {
                addSymbol(arg, 0, 'U', false);
            }
            // Add to relocation table
            addRelocationEntry(numData, 'D', ".fill", arg);
            value = 0;
        } else {
            // Local label
            if (!isLocalLabelDefined(arg)) {
                printf("Error: Undefined local label %s\n", arg);
                exit(1);
            }
            value = getLocalLabelAddress(arg);
        }
    }
    return value;
}

// Write the object file
void writeObjectFile(FILE *outFilePtr, int numInstructions, int numData) {
    // Write Header
    fprintf(outFilePtr, "%d %d %d %d\n", numInstructions, numData, numSymbols, numRelocations);

    // Write Text Section
    for (int i = 0; i < numInstructions; i++) {
        fprintf(outFilePtr, "0x%08X\n", instructions[i]);
    }

    // Write Data Section
    for (int i = 0; i < numData; i++) {
        fprintf(outFilePtr, "0x%08X\n", data[i]);
    }

    // Write Symbol Table
    for (int i = 0; i < numSymbols; i++) {
        fprintf(outFilePtr, "%s %c %d\n",
                symbolTable[i].name,
                symbolTable[i].section,
                symbolTable[i].isDefined ? symbolTable[i].address : 0);
    }

    // Write Relocation Table
    for (int i = 0; i < numRelocations; i++) {
        int offset = relocationTable[i].offset;
        fprintf(outFilePtr, "%d %s %s\n",
                offset,
                relocationTable[i].opcode,
                relocationTable[i].label);
    }
}


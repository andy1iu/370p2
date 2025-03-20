/**
 * Project 2
 * LC-2K Linker
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 500
#define MAXLINELENGTH 1000
#define MAXFILES 6

static inline void printHexToFile(FILE *, int);

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;

struct SymbolTableEntry {
	char label[7];
	char location;
	unsigned int offset;
};

struct RelocationTableEntry {
    unsigned int file;
	unsigned int offset;
	char inst[6];
	char label[7];
};

struct FileData {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	unsigned int textStartingLine; // in final executable
	unsigned int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	int text[MAXSIZE * MAXFILES];
	int data[MAXSIZE * MAXFILES];
	SymbolTableEntry symbolTable[MAXSIZE * MAXFILES];
	RelocationTableEntry relocTable[MAXSIZE * MAXFILES];
};


//
// Helper: find symbol in combined symbol table
// Returns the absolute address if found, or -1 if not found.
// 
int findSymbolAddress(CombinedFiles *combined, const char *label) {
    for (unsigned int i = 0; i < combined->symbolTableSize; i++) {
        if (!strcmp(combined->symbolTable[i].label, label)) {
            // If location == 'T', address is text offset
            // If location == 'D', address is textSize + data offset
            if (combined->symbolTable[i].location == 'T') {
                return combined->symbolTable[i].offset; 
            } else if (combined->symbolTable[i].location == 'D') {
                return (combined->textSize + combined->symbolTable[i].offset);
            }
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
	char *inFileStr, *outFileStr;
	FILE *inFilePtr, *outFilePtr; 
	unsigned int i, j;

    if (argc <= 2 || argc > 8 ) {
        printf("error: usage: %s <MAIN-object-file> ... <object-file> ... <output-exe-file>, with at most 5 object files\n",
				argv[0]);
		exit(1);
	}

	outFileStr = argv[argc - 1];

	outFilePtr = fopen(outFileStr, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileStr);
		exit(1);
	}

	FileData files[MAXFILES];
	CombinedFiles combined;
	memset(&combined, 0, sizeof(CombinedFiles));

  // read in all files and combine into a "master" file
	for (i = 0; i < argc - 2; ++i) {
		inFileStr = argv[i+1];

		inFilePtr = fopen(inFileStr, "r");
		printf("opening %s\n", inFileStr);

		if (inFilePtr == NULL) {
			printf("error in opening %s\n", inFileStr);
			exit(1);
		}

		char line[MAXLINELENGTH];
		unsigned int textSize, dataSize, symbolTableSize, relocationTableSize;

		// parse first line of file
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d",
				&textSize, &dataSize, &symbolTableSize, &relocationTableSize);

		files[i].textSize = textSize;
		files[i].dataSize = dataSize;
		files[i].symbolTableSize = symbolTableSize;
		files[i].relocationTableSize = relocationTableSize;

		// read in text section
		int instr;
		for (j = 0; j < textSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = strtol(line, NULL, 0);
			files[i].text[j] = instr;
		}

		// read in data section
		int data;
		for (j = 0; j < dataSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = strtol(line, NULL, 0);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		unsigned int addr;
		for (j = 0; j < symbolTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
					label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < relocationTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
					&addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file	= i;
		}
		fclose(inFilePtr);
	} // end reading files

	// *** INSERT YOUR CODE BELOW ***
	//    Begin the linking process
	//    Happy coding!!!
	// -----------------------------------------------------
	// 1) Merge the text and data sections in order:
	//    - We'll record where each file's text & data now starts
	//    - Then copy them into the combined text[] and data[] arrays
	// -----------------------------------------------------
	// 2) Merge text and data sections in the order read
    unsigned int currentTextStart = 0;
    unsigned int currentDataStart = 0;
    for (i = 0; i < (unsigned int)(argc - 2); i++) {
        files[i].textStartingLine = currentTextStart;
        files[i].dataStartingLine = currentDataStart;

        // Copy text
        for (j = 0; j < files[i].textSize; j++) {
            combined.text[currentTextStart + j] = files[i].text[j];
        }
        // Copy data
        for (j = 0; j < files[i].dataSize; j++) {
            combined.data[currentDataStart + j] = files[i].data[j];
        }

        currentTextStart += files[i].textSize;
        currentDataStart += files[i].dataSize;
    }
    combined.textSize = currentTextStart;
    combined.dataSize = currentDataStart;

    // 3) Build global symbol table (skip 'U')
    for (i = 0; i < (unsigned int)(argc - 2); i++) {
        for (j = 0; j < files[i].symbolTableSize; j++) {
            SymbolTableEntry *sym = &files[i].symbolTable[j];
            if (sym->location == 'U') {
                // 'U' means undefined reference => not a definition
                continue;
            }
            // check duplicates
            for (unsigned int k = 0; k < combined.symbolTableSize; k++) {
                if (!strcmp(combined.symbolTable[k].label, sym->label)) {
                    printf("error: duplicate label '%s' found in multiple files\n",
                           sym->label);
                    exit(1);
                }
            }
            // no duplicate => add
            strcpy(combined.symbolTable[combined.symbolTableSize].label, sym->label);
            combined.symbolTable[combined.symbolTableSize].location = sym->location;
            if (sym->location == 'T') {
                combined.symbolTable[combined.symbolTableSize].offset =
                    files[i].textStartingLine + sym->offset;
            } 
            else if (sym->location == 'D') {
                combined.symbolTable[combined.symbolTableSize].offset =
                    files[i].dataStartingLine + sym->offset;
            } 
            else {
                // If there's some other letter, assume we keep the offset as is
                combined.symbolTable[combined.symbolTableSize].offset = sym->offset;
            }
            combined.symbolTableSize++;
        }
    }

    // 4) Resolve relocation entries in text or data
    for (i = 0; i < (unsigned int)(argc - 2); i++) {
        for (j = 0; j < files[i].relocationTableSize; j++) {
            RelocationTableEntry *rel = &files[i].relocTable[j];

            // First find the symbol's final absolute address
            int symbolAddr = findSymbolAddress(&combined, rel->label);
			if (symbolAddr < 0) {
				// The symbol isn’t in the global table, so assume it's a local label.
				// For a text-section relocation, add the file’s textStartingLine to the immediate value.
				int localOffset = combined.text[files[i].textStartingLine + rel->offset] & 0xFFFF;
				symbolAddr = files[i].textStartingLine + localOffset;
			}

            /*
             * Now figure out if this relocation offset refers to TEXT or DATA.
             *   If rel->inst == ".fill", it means data offset
             *   Otherwise, it’s likely an instruction offset (lw, sw, beq, etc.)
             */
            if (!strcmp(rel->inst, ".fill")) {
                // data fix-up
                unsigned int finalDataIndex = files[i].dataStartingLine + rel->offset;
                // For a .fill of a label, we store the absolute address.
                combined.data[finalDataIndex] = symbolAddr;
            }
            else {
                // text fix-up
                unsigned int finalInstrIndex = files[i].textStartingLine + rel->offset;
                int instr = combined.text[finalInstrIndex];

                if (!strcmp(rel->inst, "beq")) {
                    // beq offset = symbolAddr - (PC+1)
                    int offset = symbolAddr - ((int)finalInstrIndex + 1);
                    // Clear old 16 bits, insert offset
                    instr &= 0xFFFF0000;
                    instr |= (offset & 0xFFFF);
                }
                else if (!strcmp(rel->inst, "lw") || !strcmp(rel->inst, "sw")) {
                    // place absolute address in bottom 16 bits
                    instr &= 0xFFFF0000;
                    instr |= (symbolAddr & 0xFFFF);
                }
                // else handle more instructions if needed

                combined.text[finalInstrIndex] = instr;
            }
        }
    }

    // 5) Insert "Stack" label at first free location after text & data
    {
        const char *stackLabel = "Stack";
        // check if it's already defined
        for (unsigned int k = 0; k < combined.symbolTableSize; k++) {
            if (!strcmp(combined.symbolTable[k].label, stackLabel)) {
                printf("error: 'Stack' label already defined\n");
                exit(1);
            }
        }
        // add it
        strcpy(combined.symbolTable[combined.symbolTableSize].label, stackLabel);
        combined.symbolTable[combined.symbolTableSize].location = 'D'; 
        combined.symbolTable[combined.symbolTableSize].offset = combined.dataSize;
        combined.symbolTableSize++;
    }

    // 6) Write out final machine code
    //    text instructions first, then data
    for (i = 0; i < combined.textSize; i++) {
        printHexToFile(outFilePtr, combined.text[i]);
    }
    for (i = 0; i < combined.dataSize; i++) {
        printHexToFile(outFilePtr, combined.data[i]);
    }

    fclose(outFilePtr);
    return 0;
} // main

// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}

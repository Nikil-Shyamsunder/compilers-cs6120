#include <stdio.h>

#define MAX_OPCODE 100

static unsigned long long total_instruction_count = 0;
static unsigned long long opcode_count[MAX_OPCODE] = {0};

void incrementCounter(unsigned int opcode) {
    total_instruction_count++;
    if (opcode < MAX_OPCODE) {
        opcode_count[opcode]++;
    }
}

void printInstructionCount() {
    printf("Total dynamic instruction count: %llu\n", total_instruction_count);
    printf("\nOpcode frequencies:\n");
    for (int i = 0; i < MAX_OPCODE; i++) {
        if (opcode_count[i] > 0) {
            printf("  Opcode %d: %llu\n", i, opcode_count[i]);
        }
    }
}

__attribute__((destructor))
void cleanup() {
    printInstructionCount();
}

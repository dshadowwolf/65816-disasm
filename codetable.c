#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "map.h"
#include "codetable.h"

extern const opcode_t opcodes[256];

codeentry_t* make_line(uint32_t offset, uint8_t opcode, ...) {
    codeentry_t* line = malloc(sizeof(codeentry_t));
    line->offset = offset;
    line->code = malloc(sizeof(opcode_t));
    memccpy(line->code, &opcodes[opcode], 1, sizeof(opcode_t));
    line->flags = 0; // no flags set
    line->lblname = NULL; // no label name
    va_list args;
    va_start(args, opcode);
    if (line->code->flags & BlockMoveAddress) {
        // Block Move Addressing requires two parameters
        uint8_t param1 = (uint8_t)va_arg(args, int);
        uint8_t param2 = (uint8_t)va_arg(args, int);
        line->params[0] = param1; // first parameter
        line->params[1] = param2; // second parameter
    } else {
        // all other opcodes have a single parameter
        // store them as a 16-bit value even when they are 8-bit
        // this is to maintain consistency in the code table
        // which should allow for a full simulation of the 65C816
        uint16_t param = (uint16_t)va_arg(args, int);
        line->params[0] = param; // first parameter
        line->params[1] = 0; // no second parameter
    }
    return line;    
}

void add_line(uint32_t offset, uint8_t opcode) {
    codeentry_t* line = make_line(offset, opcode);
    add_entry(offset, line);
}

#define SET_FLAG(var, flag) (var) |= (flag)
#define CLEAR_FLAG(var, flag) (var) &= ~(flag)
#define CHECK_FLAG(var, flag) !!((var) & (flag))

void make_label(uint32_t offset_source, uint32_t offset_target, const char* label) {
    codeentry_t* line = (codeentry_t*)find_node(offset_target);
    char *target_label = malloc(strlen(label) + 6);
    memset(target_label, 0, strlen(label) + 6);
    sprintf(target_label, "%s_%04X", label, offset_target);
    if (line == NULL) {
        // create a new line if it doesn't exist
        add_line(offset_target, 0xEA); // 0xEA is a placeholder for no opcode -- (NOP)
        line = (codeentry_t*)find_node(offset_target);
    }
    if (!CHECK_FLAG(line->flags, LABELED)) {
        line->flags |= LABELED; // set the labeled flag
        line->lblname = strdup(target_label); // copy the label name
    } else {
        target_label = strdup(line->lblname);
    }

    codeentry_t* source_line = (codeentry_t*)find_node(offset_source);
    if (source_line == NULL) {
        // create a new source line if it doesn't exist
        add_line(offset_source, 0xEA); // 0xEA is a placeholder
        source_line = (codeentry_t*)find_node(offset_source);
    }
    source_line->flags |= LABEL_SOURCE; // set the label source flag
    source_line->lblname = strdup(target_label); // copy the label name
    free(target_label);
}

#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG
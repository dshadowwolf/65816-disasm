#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "ops.h"

typedef enum code_flags_s {
    LABELED = 4096, // labeled code entry
    LABEL_SOURCE = 8192, // place where a label originated
} code_flags_t;

typedef struct codeentry_s {
    uint32_t offset;
    opcode_t* code;
    uint16_t params[2];
    uint8_t flags;
    char *lblname; // label name, if any
} codeentry_t;

extern const opcode_t opcodes[256];

codeentry_t* make_line(uint32_t offset, uint8_t opcode, ...) {
    codeentry_t* line = malloc(sizeof(codeentry_t));
    line->offset = offset;
    line->code = &opcodes[opcode];
    line->flags = 0; // no flags set
    line->lblname = NULL; // no label name
    va_list args;
    va_start(args, opcode);
    line->params[0] = va_arg(args, int); // first parameter
    line->params[1] = va_arg(args, int); // second parameter
    va_end(args);
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
    if (line == NULL) {
        // create a new line if it doesn't exist
        add_line(offset_target, 0xEA); // 0xEA is a placeholder for no opcode -- (NOP)
        line = (codeentry_t*)find_node(offset_target);
    }
    if (!CHECK_FLAG(line->flags, LABELED)) {
        line->flags |= LABELED; // set the labeled flag
        line->lblname = strdup(label); // copy the label name
    }

    codeentry_t* source_line = (codeentry_t*)find_node(offset_source);
    if (source_line == NULL) {
        // create a new source line if it doesn't exist
        add_line(offset_source, 0xEA); // 0xEA is a placeholder
        source_line = (codeentry_t*)find_node(offset_source);
    }
    source_line->flags |= LABEL_SOURCE; // set the label source flag
    source_line->lblname = strdup(label); // copy the label name
}

#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG
#include <stdint.h>
#include <stdlib.h>

#include "map.h"
#include "ops.h"

typedef enum code_flags_s {
    LABELED = 4096, // labeled code entry
    LABEL_SOURCE = 8192, // place where a label originated
} code_flags_t;

typedef struct codeentry_s {
    uint32_t offset;
    opcode_t* code;
    uint8_t flags;
} codeentry_t;

extern const opcode_t opcodes[256];

codeentry_t* make_line(uint32_t offset, uint8_t opcode) {
    codeentry_t* line = malloc(sizeof(codeentry_t));
    line->offset = offset;
    line->code = &opcodes[opcode];
    return line;
}

void add_line(uint32_t offset, uint8_t opcode) {
    codeentry_t* line = make_line(offset, opcode);
    add_entry(offset, line);
}

#define SET_FLAG(var, flag) (var) |= (flag)
#define CLEAR_FLAG(var, flag) (var) &= ~(flag)
#define CHECK_FLAG(var, flag) !!((var) & (flag))




#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG
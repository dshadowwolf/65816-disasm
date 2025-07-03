#include <stdint.h>
#include "ops.h"
typedef enum code_flags_s {
    LABELED = 8192,       // labeled code entry
    LABEL_SOURCE = 16384, // place where a label originated
} code_flags_t;

typedef struct codeentry_s {
    uint32_t offset;
    opcode_t* code;
    uint16_t params[2];
    uint32_t flags;
    char *lblname; // label name, if any
} codeentry_t;

codeentry_t* make_line(uint32_t offset, uint8_t opcode, ...);
void add_line(uint32_t offset, uint8_t opcode);
void make_label(uint32_t offset_source, uint32_t offset_target, const char* label);
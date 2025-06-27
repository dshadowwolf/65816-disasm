#include <stdint.h>

typedef struct codeentry_s codeentry_t;

codeentry_t* make_line(uint32_t offset, uint8_t opcode, ...);
void add_line(uint32_t offset, uint8_t opcode);
void make_label(uint32_t offset_source, uint32_t offset_target, const char* label);
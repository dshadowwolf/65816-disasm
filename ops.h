#include "machine.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct opcode_s {
    const char *opcode;
    const uint8_t psize;
    int (*munge)(int);
    void (*state)(unsigned char);
    void (*extra)(uint32_t, uint32_t);
    int (*reader)(bool);
    const uint32_t flags;
    state_t* (*op)(state_t *, uint16_t, uint16_t);
} opcode_t;

typedef enum flags_s {
    Implied = 0,               // addressing mode is implied, no parameters
    DirectPage,                // Direct Page addressing mode
    Immediate,                 // parameter is immediate value
    Indirect  = 4,             // Indirect addressing mode
    IndexedX = 8,              // Indexed addressing mode with X register
    IndexedY = 16,             // Indexed addressing mode with Y register
    Absolute = 32,             // Absolute addressing mode
    AbsoluteLong = 64,         // Absolute Long addressing mode
    IndexedLong = 128,         // Indexed Long addressing mode
    PCRelative = 256,          // PC Relative addressing mode
    StackRelative = 512,       // Stack Relative addressing mode
    PCRelativeLong = 1024,     // PC Relative Long addressing mode
    BlockMoveAddress = 2048,   // Block Move parameters needed -- used for MVP/MVN
    IndirectLong = 4096        // Indirect Long Access
} flags_t;

int m_set(int sz);
int x_set(int sz);
int base(int sz);

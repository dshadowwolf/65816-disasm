#include <stdint.h>
typedef struct opcode_s {
    const char *opcode;             // mnemonic
    const uint8_t psize;            // parameter size
    int (*munge)(int);              // function to munge the parameter size
    void (*state)(unsigned char);   // function to set the processor state
    //void (*extra)(uint32_t, void*); // function to handle extra state -- helps with JMP/BRA type instructions
    const uint16_t flags;           // flags for the opcode
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
    BlockMoveAddress = 2048    // Block Move parameters needed -- used for MVP/MVN
} flags_t;

int m_set(int sz);
int x_set(int sz);
int base(int sz);

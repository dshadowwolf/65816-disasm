#ifndef __FLAGS_HPP__
#define __FLAGS_HPP__

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

typedef enum read_types_s {
    NONE = 0,
    READ_8 = 1,               // Read 8-bit value
    READ_16 = 2,              // Read 16-bit value
    READ_24 = 3,              // Read 24-bit value
    READ_8_16 = 4,            // Read 8 or 16-bit value
    READ_BMA = 5              // Read Block Move Address
} read_types_t;

#endif // __FLAGS_HPP__

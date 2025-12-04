#ifndef __MACHINE_H__
#define __MACHINE_H__
#include <stdint.h>
#include <stdbool.h>

typedef union shared_register_u {
    struct {
        uint8_t low;
        uint8_t high;
    };
    uint16_t full;
} shared_register_t;

typedef struct processor_state_s {
    shared_register_t A;      // Accumulator
    uint16_t X;               // Index Register X
    uint16_t Y;               // Index Register Y
    uint16_t PC;              // Program Counter
    uint16_t SP;              // Stack Pointer
    uint16_t DP;              // Direct Page Register
    uint8_t P;                // Processor Status Register
    uint8_t PBR;              // Program Bank Register
    uint8_t DBR;              // Data Bank Register
    bool emulation_mode;      // Emulation Mode Flag
    bool interrupts_disabled; // Interrupt Disable Flag
} processor_state_t;

typedef struct state_s {
    processor_state_t processor;
    uint8_t* memory[64];
} state_t;

typedef state_t* (operation)(state_t*, uint16_t, uint16_t);

typedef enum processor_flags_e {
    CARRY = 0x01,
    ZERO = 0x02,
    INTERRUPT_DISABLE = 0x04,
    DECIMAL_MODE = 0x08,
    BREAK_COMMAND = 0x10,
    X_FLAG = 0x10,
    M_FLAG = 0x20,
    OVERFLOW = 0x40,
    NEGATIVE = 0x80,
} processor_flags_t;

#endif // __MACHINE_H__

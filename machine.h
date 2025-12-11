#ifndef __MACHINE_H__
#define __MACHINE_H__
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
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

// from here to the next comment is experimental design work
#define READ_BYTE(region, addr) (region->read_byte(region, addr))
#define WRITE_BYTE(region, addr, val) (region->write_byte(region, addr, val))
#define READ_WORD(region, addr) (region->read_word(region, addr))
#define WRITE_WORD(region, addr, val) (region->write_word(region, addr, val))

typedef enum mem_flags_e {
    MEM_READONLY = 0x01,
    MEM_READWRITE = 0x02,
    MEM_DEVICE = 0x04,
    MEM_SPECIAL = 0x08
} mem_flags_t;

typedef struct memory_region_s {
    uint32_t flags;
    uint16_t start_offset;
    uint16_t end_offset;
    uint8_t *data;
    uint8_t (*read_byte)(struct memory_region_s*, uint16_t);
    void (*write_byte)(struct memory_region_s*, uint16_t, uint8_t);
    uint16_t (*read_word)(struct memory_region_s*, uint16_t);
    void (*write_word)(struct memory_region_s*, uint16_t, uint16_t);
    struct memory_region_s *next;
} memory_region_t;

typedef struct memory_bank_s {
    memory_region_t *regions;
} memory_bank_t;
// end experimental design work

typedef struct machine_state_s {
    processor_state_t processor;
    memory_bank_t *memory_banks[256]; // Array of memory banks
} machine_state_t;

typedef machine_state_t* (operation)(machine_state_t*, uint16_t, uint16_t);

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

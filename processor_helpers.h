#ifndef PROCESSOR_HELPERS_H
#define PROCESSOR_HELPERS_H

#include <stdint.h>
#include "machine.h"

// raw flag operations
bool is_flag_set(state_t *machine, uint8_t flag);
state_t* set_flag(state_t *machine, uint8_t flag);
state_t* clear_flag(state_t *machine, uint8_t flag);

// Flag checking and setting helpers (8-bit)
void check_and_set_carry_8(state_t* machine, uint16_t result);
void check_and_set_zero_8(state_t *machine, uint16_t result);
void check_and_set_negative_8(state_t *machine, uint16_t result);

// Flag checking and setting helpers (16-bit)
void check_and_set_carry_16(state_t *machine, uint32_t result);
void check_and_set_zero_16(state_t *machine, uint32_t result);
void check_and_set_negative_16(state_t *machine, uint32_t result);

// Combined flag setting helpers
void set_flags_nz_8(state_t *machine, uint16_t result);
void set_flags_nz_16(state_t *machine, uint32_t result);
void set_flags_nzc_8(state_t *machine, uint16_t result);
void set_flags_nzc_16(state_t *machine, uint32_t result);

// Memory bank management
uint8_t *get_memory_bank(state_t *machine, uint8_t bank);

// Stack operations
void push_byte(state_t *machine, uint8_t value);
uint8_t pop_byte(state_t *machine);
void push_word(state_t *machine, uint16_t value);
uint16_t pop_word(state_t *machine);

// Memory read helpers
uint16_t read_word(uint8_t *memory, uint16_t address);
uint8_t read_byte(uint8_t *memory, uint16_t address);
void write_byte(uint8_t *memory, uint16_t address, uint8_t value);
void write_word(uint8_t *memory, uint16_t address, uint16_t value);

// Long address structure and helper
typedef struct long_address_s {
    uint8_t bank;
    uint16_t address;
} long_address_t;

long_address_t get_long_address(state_t *machine, uint16_t offset, uint16_t bank);

// Direct page addressing helpers
uint16_t get_dp_address(state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect(state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect_indexed_x(state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect_indexed_y(state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indexed_x(state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long(state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long_indexed_x(state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long_indexed_y(state_t *machine, uint16_t dp_offset);

// Absolute addressing helper
uint16_t get_absolute_address(state_t *machine, uint16_t address);

#endif // PROCESSOR_HELPERS_H

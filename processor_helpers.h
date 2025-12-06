#ifndef PROCESSOR_HELPERS_H
#define PROCESSOR_HELPERS_H

#include <stdint.h>
#include "machine.h"

// raw flag operations
bool is_flag_set(machine_state_t *machine, uint8_t flag);
machine_state_t* set_flag(machine_state_t *machine, uint8_t flag);
machine_state_t* clear_flag(machine_state_t *machine, uint8_t flag);

// Flag checking and setting helpers (8-bit)
void check_and_set_carry_8(machine_state_t* machine, uint16_t result);
void check_and_set_zero_8(machine_state_t *machine, uint16_t result);
void check_and_set_negative_8(machine_state_t *machine, uint16_t result);

// Flag checking and setting helpers (16-bit)
void check_and_set_carry_16(machine_state_t *machine, uint32_t result);
void check_and_set_zero_16(machine_state_t *machine, uint32_t result);
void check_and_set_negative_16(machine_state_t *machine, uint32_t result);

// Combined flag setting helpers
void set_flags_nz_8(machine_state_t *machine, uint16_t result);
void set_flags_nz_16(machine_state_t *machine, uint32_t result);
void set_flags_nzc_8(machine_state_t *machine, uint16_t result);
void set_flags_nzc_16(machine_state_t *machine, uint32_t result);

// Memory bank management
uint8_t *get_memory_bank(machine_state_t *machine, uint8_t bank);

// Stack operations
void push_byte(machine_state_t *machine, uint8_t value);
uint8_t pop_byte(machine_state_t *machine);
void push_word(machine_state_t *machine, uint16_t value);
uint16_t pop_word(machine_state_t *machine);

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

long_address_t get_long_address(machine_state_t *machine, uint16_t offset, uint16_t bank);

// Direct page addressing helpers
uint16_t get_dp_address(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect_indexed_x(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect_indexed_y(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indexed_x(machine_state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long(machine_state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long_indexed_x(machine_state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long_indexed_y(machine_state_t *machine, uint16_t dp_offset);

// Stack relative addressing helpers
uint16_t get_stack_relative_address(machine_state_t *machine, uint8_t offset);
uint16_t get_stack_relative_address_indexed_y(machine_state_t *machine, uint8_t offset);
uint16_t get_stack_relative_address_indirect(machine_state_t *machine, uint8_t offset);
uint16_t get_stack_relative_address_indirect_indexed_y(machine_state_t *machine, uint8_t offset);

// Absolute addressing helper
uint16_t get_absolute_address(machine_state_t *machine, uint16_t address);
uint16_t get_absolute_address_indexed_x(machine_state_t *machine, uint16_t address);
uint16_t get_absolute_address_indexed_y(machine_state_t *machine, uint16_t address);
long_address_t get_absolute_address_long(machine_state_t *machine, uint16_t address, uint8_t bank);
long_address_t get_absolute_address_long_indexed_x(machine_state_t *machine, uint16_t address, uint8_t bank);
long_address_t get_absolute_address_long_indexed_y(machine_state_t *machine, uint16_t address, uint8_t bank);
long_address_t get_absolute_address_long_indirect(machine_state_t *machine, uint16_t address, uint8_t bank);
uint16_t get_absolute_address_indirect(machine_state_t *machine, uint16_t address);
uint16_t get_absolute_address_indirect_indexed_y(machine_state_t *machine, uint16_t address);
uint16_t get_absolute_address_indirect_indexed_x(machine_state_t *machine, uint16_t address);

// experimental/future features
memory_region_t *find_memory_region(machine_state_t *machine, uint8_t bank, uint16_t address);
memory_region_t *find_stack_memory_region(machine_state_t *machine);
memory_region_t *find_current_memory_region(machine_state_t *machine, uint16_t address);
void push_byte_new(machine_state_t *machine, uint8_t value);
void push_word_new(machine_state_t *machine, uint16_t value);
uint8_t pop_byte_new(machine_state_t *machine);
uint16_t pop_word_new(machine_state_t *machine);
void write_byte_new(machine_state_t *machine, uint16_t address, uint8_t value);
void write_word_new(machine_state_t *machine, uint16_t address, uint16_t value);
uint8_t read_byte_new(machine_state_t *machine, uint16_t address);
uint16_t read_word_new(machine_state_t *machine, uint16_t address);
uint8_t read_byte_dp_sr(machine_state_t *machine, uint16_t address);
uint16_t read_word_dp_sr(machine_state_t *machine, uint16_t address);
void write_byte_dp_sr(machine_state_t *machine, uint16_t address, uint8_t value);
void write_word_dp_sr(machine_state_t *machine, uint16_t address, uint16_t value);
void write_byte_long(machine_state_t *machine, long_address_t long_addr, uint8_t value);
void write_word_long(machine_state_t *machine, long_address_t long_addr, uint16_t value);
uint8_t read_byte_long(machine_state_t *machine, long_address_t long_addr);
uint16_t read_word_long(machine_state_t *machine, long_address_t long_addr);
uint16_t get_dp_address_indirect_new(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect_indexed_x_new(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_dp_address_indirect_indexed_y_new(machine_state_t *machine, uint16_t dp_offset);
long_address_t get_dp_address_indirect_long_new(machine_state_t *machine, uint16_t dp_offset);
uint16_t get_stack_relative_address_indirect_new(machine_state_t *machine, uint8_t offset);
uint16_t get_stack_relative_address_indirect_indexed_y_new(machine_state_t *machine, uint8_t offset);
#endif // PROCESSOR_HELPERS_H

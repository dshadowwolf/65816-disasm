#include "processor_helpers.h"
#include "machine.h"
#include <stdint.h>
#include <stdlib.h>


bool is_flag_set(state_t *machine, uint8_t flag) {
    return (machine->processor.P & flag) != 0;
}

state_t* set_flag(state_t *machine, uint8_t flag) {
    machine->processor.P |= flag;
    return machine;
}

state_t* clear_flag(state_t *machine, uint8_t flag) {
    machine->processor.P &= ~flag;
    return machine;
}

void check_and_set_carry_8(state_t* machine, uint16_t result) {
    if (result & 0x100) {
        set_flag(machine, CARRY);
    } else {
        clear_flag(machine, CARRY);
    }
}

void check_and_set_zero_8(state_t *machine, uint16_t result) {
    if ((result & 0xFF) == 0) {
        set_flag(machine, ZERO);
    } else {
        clear_flag(machine, ZERO);
    }
}

void check_and_set_negative_8(state_t *machine, uint16_t result) {
    if (result & 0x80) {
        set_flag(machine, NEGATIVE);
    } else {
        clear_flag(machine, NEGATIVE);
    }
}

void check_and_set_carry_16(state_t *machine, uint32_t result) {
    if (result & 0x10000) {
        set_flag(machine, CARRY);
    } else {
        clear_flag(machine, CARRY);
    }
}

void check_and_set_zero_16(state_t *machine, uint32_t result) {
    if ((result & 0xFFFF) == 0) {
        set_flag(machine, ZERO);
    } else {
        clear_flag(machine, ZERO);
    }
}

void check_and_set_negative_16(state_t *machine, uint32_t result) {
    if (result & 0x8000) {
        set_flag(machine, NEGATIVE);
    } else {
        clear_flag(machine, NEGATIVE);
    }
}

uint8_t *get_memory_bank(state_t *machine, uint8_t bank) {
    if (machine->memory[bank] == NULL) {
        machine->memory[bank] = (uint8_t*)malloc(65536 * sizeof(uint8_t));
    }
    return machine->memory[bank];
}

void set_flags_nz_8(state_t *machine, uint16_t result) {
    check_and_set_zero_8(machine, result);
    check_and_set_negative_8(machine, result);
}

void set_flags_nz_16(state_t *machine, uint32_t result) {
    check_and_set_zero_16(machine, result);
    check_and_set_negative_16(machine, result);
}

void set_flags_nzc_8(state_t *machine, uint16_t result) {
    check_and_set_carry_8(machine, result);
    set_flags_nz_8(machine, result);
}

void set_flags_nzc_16(state_t *machine, uint32_t result) {
    check_and_set_carry_16(machine, result);
    set_flags_nz_16(machine, result);
}

void push_byte(state_t *machine, uint8_t value) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    memory_bank[0x0100 | (state->SP & 0xFF)] = value;
    state->SP--;
}

uint8_t pop_byte(state_t *machine) {
    processor_state_t *state = &machine->processor;
    state->SP++;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    return memory_bank[0x0100 | (state->SP & 0xFF)];
}

uint16_t read_word(uint8_t *memory, uint16_t address) {
    uint8_t low_byte = memory[address];
    uint8_t high_byte = memory[(address + 1) & 0xFFFF];
    return (high_byte << 8) | low_byte;
}

long_address_t get_long_address(state_t *machine, uint16_t offset, uint16_t bank) {
    long_address_t long_addr;
    long_addr.bank = bank & 0xFF;
    long_addr.address = offset & 0xFFFF;
    return long_addr;
}

uint16_t get_dp_address(state_t *machine, uint16_t dp_offset) {
    return (machine->processor.DP + dp_offset) & 0xFFFF;
}

uint16_t get_dp_address_indirect(state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    uint8_t *memory_bank = get_memory_bank(machine, machine->processor.PBR);
    uint8_t low_byte = memory_bank[dp_address];
    uint8_t high_byte = memory_bank[(dp_address + 1) & 0xFFFF];
    return (high_byte << 8) | low_byte;
}

uint16_t get_dp_address_indirect_indexed_x(state_t *machine, uint16_t dp_offset) {
    uint16_t effective_address = get_dp_address_indirect(machine, dp_offset);
    return (effective_address + machine->processor.X) & 0xFFFF;
}

uint16_t get_dp_address_indirect_indexed_y(state_t *machine, uint16_t dp_offset) {
    uint16_t effective_address = get_dp_address_indirect(machine, dp_offset);
    return (effective_address + machine->processor.Y) & 0xFFFF;
}

uint16_t get_dp_address_indirect_long(state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    uint8_t *memory_bank = get_memory_bank(machine, machine->processor.PBR);
    uint8_t low_byte = memory_bank[dp_address];
    uint8_t mid_byte = memory_bank[(dp_address + 1) & 0xFFFF];
    uint8_t high_byte = memory_bank[(dp_address + 2) & 0xFFFF];
    return (high_byte << 16) | (mid_byte << 8) | low_byte;
}

uint16_t get_absolute_address(state_t *machine, uint16_t address) {
    return address & 0xFFFF;
}
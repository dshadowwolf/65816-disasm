#include "processor_helpers.h"
#include "machine.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

bool is_flag_set(machine_state_t *machine, uint8_t flag) {
    return (machine->processor.P & flag) != 0;
}

machine_state_t* set_flag(machine_state_t *machine, uint8_t flag) {
    machine->processor.P |= flag;
    return machine;
}

machine_state_t* clear_flag(machine_state_t *machine, uint8_t flag) {
    machine->processor.P &= ~flag;
    return machine;
}

void check_and_set_carry_8(machine_state_t* machine, uint16_t result) {
    if (result & 0x100) {
        set_flag(machine, CARRY);
    } else {
        clear_flag(machine, CARRY);
    }
}

void check_and_set_zero_8(machine_state_t *machine, uint16_t result) {
    if ((result & 0xFF) == 0) {
        set_flag(machine, ZERO);
    } else {
        clear_flag(machine, ZERO);
    }
}

void check_and_set_negative_8(machine_state_t *machine, uint16_t result) {
    if (result & 0x80) {
        set_flag(machine, NEGATIVE);
    } else {
        clear_flag(machine, NEGATIVE);
    }
}

void check_and_set_carry_16(machine_state_t *machine, uint32_t result) {
    if (result & 0x10000) {
        set_flag(machine, CARRY);
    } else {
        clear_flag(machine, CARRY);
    }
}

void check_and_set_zero_16(machine_state_t *machine, uint32_t result) {
    if ((result & 0xFFFF) == 0) {
        set_flag(machine, ZERO);
    } else {
        clear_flag(machine, ZERO);
    }
}

void check_and_set_negative_16(machine_state_t *machine, uint32_t result) {
    if (result & 0x8000) {
        set_flag(machine, NEGATIVE);
    } else {
        clear_flag(machine, NEGATIVE);
    }
}

uint8_t *get_memory_bank(machine_state_t *machine, uint8_t bank) {
    if (machine->memory[bank] == NULL) {
        machine->memory[bank] = (uint8_t*)malloc(65536 * sizeof(uint8_t));
    }
    return machine->memory[bank];
}

void set_flags_nz_8(machine_state_t *machine, uint16_t result) {
    check_and_set_zero_8(machine, result);
    check_and_set_negative_8(machine, result);
}

void set_flags_nz_16(machine_state_t *machine, uint32_t result) {
    check_and_set_zero_16(machine, result);
    check_and_set_negative_16(machine, result);
}

void set_flags_nzc_8(machine_state_t *machine, uint16_t result) {
    check_and_set_carry_8(machine, result);
    set_flags_nz_8(machine, result);
}

void set_flags_nzc_16(machine_state_t *machine, uint32_t result) {
    check_and_set_carry_16(machine, result);
    set_flags_nz_16(machine, result);
}

/*
 * From here to the ending comment is work to implement memory regions and banks
 */
memory_region_t *find_memory_region(machine_state_t *machine, uint8_t bank, uint16_t address) {
    memory_bank_t *mem_bank = machine->memory_banks[bank];
    if (mem_bank == NULL) {
        return NULL;
    }
    memory_region_t *region = mem_bank->regions;
    while (region != NULL) {
        if (address >= region->start_offset && address <= region->end_offset) {
            return region;
        }
        region = region->next;
    }
    return NULL; // No matching region found
}

memory_region_t *find_stack_memory_region(machine_state_t *machine) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address = state->emulation_mode ? (0x0100 | (state->SP & 0xFF)) : (state->SP & 0xFFFF);
    return find_memory_region(machine, 0, sp_address); // bank 0 for stack
}

memory_region_t *find_current_memory_region(machine_state_t *machine, uint16_t address) {
    processor_state_t *state = &machine->processor;
    uint8_t bank = state->DBR;
    return find_memory_region(machine, bank, address);
}

void push_byte_new(machine_state_t *machine, uint8_t value) {
    processor_state_t *state = &machine->processor;
    memory_region_t *region = find_stack_memory_region(machine);
    uint16_t sp_address = state->emulation_mode ? (0x0100 | (state->SP & 0xFF)) : (state->SP & 0xFFFF);
    
    if (region != NULL) {
        WRITE_BYTE(region, sp_address, value);
    } else {
        // Fallback to direct memory bank access when no region is configured
        uint8_t *memory_bank = get_memory_bank(machine, state->emulation_mode ? 0 : state->DBR);
        write_byte(memory_bank, sp_address, value);
    }
    
    if (state->emulation_mode) {
        state->SP = (state->SP - 1) & 0x1FF;
    } else {
        state->SP = (state->SP - 1) & 0xFFFF;
    }
}

void push_word_new(machine_state_t *machine, uint16_t value) {
    push_byte_new(machine, (value >> 8) & 0xFF);  // Push high byte first
    push_byte_new(machine, value & 0xFF);         // Push low byte second
}

uint8_t pop_byte_new(machine_state_t *machine) {
    processor_state_t *state = &machine->processor;
    memory_region_t *region = find_stack_memory_region(machine);
    
    if (state->emulation_mode) {
        state->SP = (state->SP + 1) & 0x1FF;
    } else {
        state->SP = (state->SP + 1) & 0xFFFF;
    }
    
    uint16_t sp_address = state->emulation_mode ? (0x0100 | (state->SP & 0xFF)) : (state->SP & 0xFFFF);
    
    if (region != NULL) {
        return READ_BYTE(region, sp_address);
    } else {
        // Fallback to direct memory bank access when no region is configured
        uint8_t *memory_bank = get_memory_bank(machine, state->emulation_mode ? 0 : state->DBR);
        return read_byte(memory_bank, sp_address);
    }
}

uint16_t pop_word_new(machine_state_t *machine) {
    uint8_t low_byte = pop_byte_new(machine);     // Pop low byte first
    uint8_t high_byte = pop_byte_new(machine);    // Pop high byte second
    return (high_byte << 8) | low_byte;
}

void write_byte_new(machine_state_t *machine, uint16_t address, uint8_t value) {
    memory_region_t *region = find_current_memory_region(machine, address);
    if (region != NULL) {
        WRITE_BYTE(region, address, value);
    }
}

void write_word_new(machine_state_t *machine, uint16_t address, uint16_t value) {
    memory_region_t *region = find_current_memory_region(machine, address);
    if (region != NULL) {
        WRITE_WORD(region, address, value);
    }
}

uint8_t read_byte_new(machine_state_t *machine, uint16_t address) {
    memory_region_t *region = find_current_memory_region(machine, address);
    if (region != NULL) {
        uint8_t value = READ_BYTE(region, address);
        return value;
    }
    return 0; // Default return if region not found
}

uint16_t read_word_new(machine_state_t *machine, uint16_t address) {
    memory_region_t *region = find_current_memory_region(machine, address);
    if (region != NULL) {
        uint16_t value = READ_WORD(region, address);
        return value;
    }
    return 0; // Default return if region not found
}

uint8_t read_byte_dp_sr(machine_state_t *machine, uint16_t address) {
    memory_region_t *region = find_stack_memory_region(machine);
    if (region != NULL) {
        return READ_BYTE(region, address);
    }
    return 0; // Default return if region not found
}

uint16_t read_word_dp_sr(machine_state_t *machine, uint16_t address) {
    memory_region_t *region = find_stack_memory_region(machine);
    if (region != NULL) {
        return READ_WORD(region, address);
    }
    return 0; // Default return if region not found
}

void write_byte_long(machine_state_t *machine, long_address_t long_addr, uint8_t value) {
    memory_region_t *region = find_memory_region(machine, long_addr.bank, long_addr.address);
    if (region != NULL) {
        WRITE_BYTE(region, long_addr.address, value);
    }
}

void write_word_long(machine_state_t *machine, long_address_t long_addr, uint16_t value) {
    memory_region_t *region = find_memory_region(machine, long_addr.bank, long_addr.address);
    if (region != NULL) {
        WRITE_WORD(region, long_addr.address, value);
    }
}

uint8_t read_byte_long(machine_state_t *machine, long_address_t long_addr) {
    memory_region_t *region = find_memory_region(machine, long_addr.bank, long_addr.address);
    if (region != NULL) {
        return READ_BYTE(region, long_addr.address);
    }
    return 0; // Default return if region not found
}

uint16_t read_word_long(machine_state_t *machine, long_address_t long_addr) {
    memory_region_t *region = find_memory_region(machine, long_addr.bank, long_addr.address);
    if (region != NULL) {
        return READ_WORD(region, long_addr.address);
    }
    return 0; // Default return if region not found
}

void write_byte_dp_sr(machine_state_t *machine, uint16_t address, uint8_t value) {
    memory_region_t *region = find_stack_memory_region(machine);
    if (region != NULL) {
        WRITE_BYTE(region, address, value);
    }
}

void write_word_dp_sr(machine_state_t *machine, uint16_t address, uint16_t value) {
    memory_region_t *region = find_stack_memory_region(machine);
    if (region != NULL) {
        WRITE_WORD(region, address, value);
    }
}

uint16_t get_dp_address_indirect_new(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    return read_word_new(machine, dp_address);
}

uint16_t get_dp_address_indirect_indexed_x_new(machine_state_t *machine, uint16_t dp_offset) {
    // (DP,X) - Indexed Indirect: add X to DP offset, then read pointer
    uint16_t dp_address = get_dp_address(machine, (dp_offset + machine->processor.X) & 0xFF);
    return read_word_dp_sr(machine, dp_address);
}

long_address_t get_dp_address_indirect_long_new(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    uint16_t addr = read_word_dp_sr(machine, dp_address);
    uint8_t bank = read_byte_dp_sr(machine, (dp_address + 2) & 0xFFFF);
    return get_long_address(machine, addr, bank);
}

long_address_t get_dp_address_indirect_long_indexed_y_new(machine_state_t *machine, uint16_t dp_offset) {
    long_address_t long_addr = get_dp_address_indirect_long_new(machine, dp_offset);
    long_addr.address = (long_addr.address + machine->processor.Y) & 0xFFFF;
    return long_addr;
}

uint16_t get_dp_address_indirect_indexed_y_new(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t effective_address = get_dp_address_indirect_new(machine, dp_offset);
    return (effective_address + machine->processor.Y) & 0xFFFF;
}

uint16_t get_stack_relative_address_indirect_indexed_y_new(machine_state_t *machine, uint8_t offset) {
    uint16_t pointer_address = get_stack_relative_address(machine, offset);
    uint16_t effective_address = read_word_dp_sr(machine, pointer_address);
    return (effective_address + machine->processor.Y) & 0xFFFF;
}

long_address_t get_absolute_long_indexed_x_new(machine_state_t *machine, uint16_t address, uint8_t bank) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = (address + (state->X & 0xFFFF)) & 0xFFFF;
    return get_long_address(machine, effective_address, bank);
}

uint16_t get_dp_address_indexed_y(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    return (dp_address + machine->processor.Y) & 0xFFFF;
}

/* End experimental design work */

void push_byte(machine_state_t *machine, uint8_t value) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->emulation_mode?0:state->DBR);
    
    if (state->emulation_mode) {
        // Emulation mode: stack is in page 1 (0x0100-0x01FF)
        write_byte(memory_bank, 0x0100 | (state->SP & 0xFF), value);
        state->SP = (state->SP - 1) & 0x1FF;
    } else {
        // Native mode: stack can be anywhere in bank 0
        write_byte(memory_bank, state->SP & 0xFFFF, value);
        state->SP = (state->SP - 1) & 0xFFFF;
    }
}

uint8_t pop_byte(machine_state_t *machine) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->emulation_mode?0:state->DBR);
    
    if (state->emulation_mode) {
        // Emulation mode: stack is in page 1 (0x0100-0x01FF)
        state->SP = (state->SP + 1) & 0x1FF;
        return read_byte(memory_bank, 0x0100 | (state->SP & 0xFF));
    } else {
        // Native mode: stack can be anywhere in bank 0
        state->SP = (state->SP + 1) & 0xFFFF;
        return read_byte(memory_bank, state->SP & 0xFFFF);
    }
}

void push_word(machine_state_t *machine, uint16_t value) {
    push_byte(machine, (value >> 8) & 0xFF);  // Push high byte first
    push_byte(machine, value & 0xFF);         // Push low byte second
}

uint16_t pop_word(machine_state_t *machine) {
    uint8_t low_byte = pop_byte(machine);     // Pop low byte first
    uint8_t high_byte = pop_byte(machine);    // Pop high byte second
    return (high_byte << 8) | low_byte;
}

uint16_t read_word(uint8_t *memory, uint16_t address) {
    uint8_t low_byte = memory[address];
    uint8_t high_byte = memory[(address + 1) & 0xFFFF];
    return (high_byte << 8) | low_byte;
}

uint8_t read_byte(uint8_t *memory, uint16_t address) {
    return memory[address];
}

void write_byte(uint8_t *memory, uint16_t address, uint8_t value) {
    memory[address] = value;
}

void write_word(uint8_t *memory, uint16_t address, uint16_t value) {
    memory[address] = value & 0xFF;               // Low byte
    memory[(address + 1) & 0xFFFF] = (value >> 8) & 0xFF; // High byte
}

long_address_t get_long_address(machine_state_t *machine, uint16_t offset, uint16_t bank) {
    long_address_t long_addr;
    long_addr.bank = bank & 0xFF;
    long_addr.address = offset & 0xFFFF;
    return long_addr;
}

uint16_t get_dp_address(machine_state_t *machine, uint16_t dp_offset) {
    return (machine->processor.DP + dp_offset) & 0xFFFF;
}

uint16_t get_dp_address_indirect(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    uint8_t *memory_bank = get_memory_bank(machine, machine->processor.emulation_mode?0:machine->processor.DBR);
    return read_word(memory_bank, dp_address);
}

uint16_t get_dp_address_indirect_indexed_x(machine_state_t *machine, uint16_t dp_offset) {
    // (DP,X) - Indexed Indirect: add X to DP offset, then read pointer
    uint16_t dp_address = get_dp_address(machine, (dp_offset + machine->processor.X) & 0xFF);
    uint8_t *memory_bank = get_memory_bank(machine, machine->processor.emulation_mode?0:machine->processor.DBR);
    return read_word(memory_bank, dp_address);
}

uint16_t get_dp_address_indirect_indexed_y(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t effective_address = get_dp_address_indirect(machine, dp_offset);
    return (effective_address + machine->processor.Y) & 0xFFFF;
}

long_address_t get_dp_address_indirect_long(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    uint8_t *memory_bank = get_memory_bank(machine, 0); // Direct page always in bank 0
    uint16_t addr = read_word(memory_bank, dp_address);
    uint8_t bank = read_byte(memory_bank, (dp_address + 2) & 0xFFFF);
    return get_long_address(machine, addr, bank);
}

uint16_t get_absolute_address(machine_state_t *machine, uint16_t address) {
    return address & 0xFFFF;
}
uint16_t get_absolute_address_indexed_x(machine_state_t *machine, uint16_t address) {
    return (address + machine->processor.X) & 0xFFFF;

}
uint16_t get_absolute_address_indexed_y(machine_state_t *machine, uint16_t address) {
    return (address + machine->processor.Y) & 0xFFFF;
}

long_address_t get_absolute_address_long(machine_state_t *machine, uint16_t address, uint8_t bank) {
    return get_long_address(machine, address, bank);
}

long_address_t get_absolute_address_long_indexed_x(machine_state_t *machine, uint16_t address, uint8_t bank) {
    long_address_t long_addr = get_absolute_address_long(machine, address, bank);
    long_addr.address = (long_addr.address + machine->processor.X) & 0xFFFF;
    return long_addr;
}

long_address_t get_absolute_address_long_indexed_y(machine_state_t *machine, uint16_t address, uint8_t bank) {
    long_address_t long_addr = get_absolute_address_long(machine, address, bank);
    long_addr.address = (long_addr.address + machine->processor.Y) & 0xFFFF;
    return long_addr;
}

uint16_t get_absolute_address_indirect(machine_state_t *machine, uint16_t address) {
    uint8_t *memory_bank = get_memory_bank(machine, machine->processor.DBR);
    return read_word(memory_bank, address);

}

uint16_t get_absolute_address_indirect_indexed_y(machine_state_t *machine, uint16_t address) {
    uint16_t effective_address = get_absolute_address_indirect(machine, address);
    return (effective_address + machine->processor.Y) & 0xFFFF;

}

uint16_t get_absolute_address_indirect_indexed_x(machine_state_t *machine, uint16_t address) {
    uint16_t effective_address = get_absolute_address_indirect(machine, address);
    return (effective_address + machine->processor.X) & 0xFFFF;
}

long_address_t get_absolute_address_long_indirect(machine_state_t *machine, uint16_t address, uint8_t bank) {
    uint8_t *memory_bank = get_memory_bank(machine, bank);
    uint8_t low_byte = memory_bank[address];
    uint8_t high_byte = memory_bank[(address + 1) & 0xFFFF];
    uint8_t bank_byte = memory_bank[(address + 2) & 0xFFFF];
    return get_long_address(machine, (high_byte << 8) | low_byte, bank_byte);
}
uint16_t get_dp_address_indexed_x(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    return (dp_address + machine->processor.X) & 0xFFFF;
}

long_address_t get_dp_address_indirect_long_indexed_x(machine_state_t *machine, uint16_t dp_offset) {
    long_address_t long_addr = get_dp_address_indirect_long(machine, dp_offset);
    long_addr.address = (long_addr.address + machine->processor.X) & 0xFFFF;
    return long_addr;
}

long_address_t get_dp_address_indirect_long_indexed_y(machine_state_t *machine, uint16_t dp_offset) {
    long_address_t long_addr = get_dp_address_indirect_long(machine, dp_offset);
    long_addr.address = (long_addr.address + machine->processor.Y) & 0xFFFF;
    return long_addr;
}

uint16_t get_stack_relative_address(machine_state_t *machine, uint8_t offset) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) {
        sp_address = 0x0100 | (state->SP & 0xFF);
    } else {
        sp_address = state->SP & 0xFFFF;
    }
    return (sp_address + offset) & 0xFFFF;
}

uint16_t get_stack_relative_address_indexed_y(machine_state_t *machine, uint8_t offset) {
    uint16_t base_address = get_stack_relative_address(machine, offset);
    return (base_address + machine->processor.Y) & 0xFFFF;
}

uint16_t get_stack_relative_address_indirect(machine_state_t *machine, uint8_t offset) {
    uint16_t base_address = get_stack_relative_address(machine, offset);
    uint8_t *memory_bank = get_memory_bank(machine, 0); // Stack relative always in bank 0
    return read_word(memory_bank, base_address);
}

uint16_t get_stack_relative_address_indirect_indexed_y(machine_state_t *machine, uint8_t offset) {
    uint16_t pointer_address = get_stack_relative_address(machine, offset);
    uint8_t *memory_bank = get_memory_bank(machine, 0); // Stack relative always in bank 0
    uint16_t effective_address = read_word(memory_bank, pointer_address);
    return (effective_address + machine->processor.Y) & 0xFFFF;
}

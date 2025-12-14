#include "processor_helpers.h"
#include "machine.h"
#include <stdint.h>
#include <stdbool.h>
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
        if (state->emulation_mode) {
            state->SP = (state->SP - 1) & 0x1FF;
        } else {
            state->SP = (state->SP - 1) & 0xFFFF;
        }
    } else {
        fprintf(stderr, "Region not found for push_byte_new at SP=$%04X\n", sp_address);
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
    
    uint8_t result;
    if (region != NULL) {
        result = READ_BYTE(region, sp_address);
    } else {
        return 0xFF;
    }
    
    return result;
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

long_address_t get_long_address(machine_state_t *machine, uint16_t offset, uint16_t bank) {
    long_address_t long_addr;
    long_addr.bank = bank & 0xFF;
    long_addr.address = offset & 0xFFFF;
    return long_addr;
}

uint16_t get_dp_address(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = (machine->processor.DP + dp_offset) & 0xFFFF;
    return dp_address;
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

uint16_t get_dp_address_indexed_x(machine_state_t *machine, uint16_t dp_offset) {
    uint16_t dp_address = get_dp_address(machine, dp_offset);
    return (dp_address + machine->processor.X) & 0xFFFF;
}

long_address_t get_dp_address_indirect_long_indexed_x(machine_state_t *machine, uint16_t dp_offset) {
    long_address_t long_addr = get_dp_address_indirect_long_new(machine, dp_offset);
    long_addr.address = (long_addr.address + machine->processor.X) & 0xFFFF;
    return long_addr;
}

long_address_t get_dp_address_indirect_long_indexed_y(machine_state_t *machine, uint16_t dp_offset) {
    long_address_t long_addr = get_dp_address_indirect_long_new(machine, dp_offset);
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

uint16_t get_absolute_address_indirect_new(machine_state_t *machine, uint16_t address) {
    return read_word_new(machine, address);
}

long_address_t get_absolute_address_long_indirect_new(machine_state_t *machine, uint16_t address, uint8_t bank) {
    uint16_t addr = read_word_long(machine, get_long_address(machine, address, bank));
    uint8_t bank_b = read_byte_long(machine, get_long_address(machine, address+2, bank));
    return get_long_address(machine, addr, bank_b);
}

// BCD (Binary Coded Decimal) arithmetic helpers
// In BCD mode, each nibble represents a decimal digit 0-9

uint16_t bcd_add_8(uint8_t a, uint8_t b, uint16_t carry_in, bool *carry_out) {
    // Add two BCD bytes with carry
    uint16_t low_nibble = (a & 0x0F) + (b & 0x0F) + carry_in;
    uint16_t high_nibble = (a >> 4) + (b >> 4);
    
    // Adjust low nibble if > 9
    if (low_nibble > 9) {
        low_nibble += 6;
        high_nibble++;
    }
    
    // Adjust high nibble if > 9
    if (high_nibble > 9) {
        high_nibble += 6;
        *carry_out = true;
    } else {
        *carry_out = false;
    }
    
    return ((high_nibble & 0x0F) << 4) | (low_nibble & 0x0F);
}

uint32_t bcd_add_16(uint16_t a, uint16_t b, uint32_t carry_in, bool *carry_out) {
    // Add two BCD words (4 nibbles each) with carry
    uint32_t result = 0;
    uint32_t carry = carry_in;
    
    // Process each nibble from right to left
    for (int i = 0; i < 4; i++) {
        uint32_t nibble_a = (a >> (i * 4)) & 0x0F;
        uint32_t nibble_b = (b >> (i * 4)) & 0x0F;
        uint32_t sum = nibble_a + nibble_b + carry;
        
        if (sum > 9) {
            sum += 6;
            carry = 1;
        } else {
            carry = 0;
        }
        
        result |= ((sum & 0x0F) << (i * 4));
    }
    
    *carry_out = (carry != 0);
    return result;
}

uint16_t bcd_subtract_8(uint8_t a, uint8_t b, uint16_t carry_in, bool *carry_out) {
    // Subtract two BCD bytes with borrow (carry_in = 1 means no borrow)
    uint16_t low_nibble = (a & 0x0F) - (b & 0x0F) - (carry_in ? 0 : 1);
    uint16_t high_nibble = (a >> 4) - (b >> 4);
    
    // Adjust low nibble if negative
    if (low_nibble & 0x8000) {  // Borrow occurred
        low_nibble -= 6;
        high_nibble--;
    }
    
    // Adjust high nibble if negative
    if (high_nibble & 0x8000) {  // Borrow occurred
        high_nibble -= 6;
        *carry_out = false;  // Borrow out
    } else {
        *carry_out = true;  // No borrow
    }
    
    return ((high_nibble & 0x0F) << 4) | (low_nibble & 0x0F);
}

uint32_t bcd_subtract_16(uint16_t a, uint16_t b, uint32_t carry_in, bool *carry_out) {
    // Subtract two BCD words with borrow (carry_in = 1 means no borrow)
    uint32_t result = 0;
    uint32_t borrow = (carry_in ? 0 : 1);
    
    // Process each nibble from right to left
    for (int i = 0; i < 4; i++) {
        int32_t nibble_a = (a >> (i * 4)) & 0x0F;
        int32_t nibble_b = (b >> (i * 4)) & 0x0F;
        int32_t diff = nibble_a - nibble_b - borrow;
        
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        
        result |= ((diff & 0x0F) << (i * 4));
    }
    
    *carry_out = (borrow == 0);  // Carry out is set if no final borrow
    return result;
}

// High-level ADC/SBC helpers that handle both binary and decimal modes

void adc_8bit(machine_state_t *machine, uint8_t value) {
    processor_state_t *state = &machine->processor;
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    
    if (is_flag_set(machine, DECIMAL_MODE)) {
        // Decimal (BCD) mode
        uint8_t a_val = state->A.low;
        bool carry_out = false;
        uint16_t result = bcd_add_8(a_val, value, carry, &carry_out);
        state->A.low = (uint8_t)(result & 0xFF);
        
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        set_flags_nz_8(machine, state->A.low);
        // Overflow flag is undefined in decimal mode
    } else {
        // Binary mode
        uint8_t a_val = state->A.low;
        uint16_t result = (uint16_t)a_val + (uint16_t)value + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        
        check_and_set_carry_8(machine, result);
        set_flags_nz_8(machine, state->A.low);
        // Set Overflow flag: V = (A^result) & (M^result) & 0x80
        if (((a_val ^ result) & (value ^ result) & 0x80)) {
            set_flag(machine, OVERFLOW);
        } else {
            clear_flag(machine, OVERFLOW);
        }
    }
}

void adc_16bit(machine_state_t *machine, uint16_t value) {
    processor_state_t *state = &machine->processor;
    uint32_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    
    if (is_flag_set(machine, DECIMAL_MODE)) {
        // Decimal (BCD) mode
        uint16_t a_val = state->A.full;
        bool carry_out = false;
        uint32_t result = bcd_add_16(a_val, value, carry, &carry_out);
        state->A.full = (uint16_t)(result & 0xFFFF);
        
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        set_flags_nz_16(machine, state->A.full);
        // Overflow flag is undefined in decimal mode
    } else {
        // Binary mode
        uint16_t a_val = state->A.full;
        uint32_t result = (uint32_t)a_val + (uint32_t)value + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        
        check_and_set_carry_16(machine, result);
        set_flags_nz_16(machine, state->A.full);
        // Set Overflow flag: V = (A^result) & (M^result) & 0x8000
        if (((a_val ^ result) & (value ^ result) & 0x8000)) {
            set_flag(machine, OVERFLOW);
        } else {
            clear_flag(machine, OVERFLOW);
        }
    }
}

void sbc_8bit(machine_state_t *machine, uint8_t value) {
    processor_state_t *state = &machine->processor;
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    
    if (is_flag_set(machine, DECIMAL_MODE)) {
        // Decimal (BCD) mode
        uint8_t a_val = state->A.low;
        bool carry_out = false;
        uint16_t result = bcd_subtract_8(a_val, value, carry, &carry_out);
        state->A.low = (uint8_t)(result & 0xFF);
        
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        set_flags_nz_8(machine, state->A.low);
        // Overflow flag is undefined in decimal mode
    } else {
        // Binary mode
        uint8_t a_val = state->A.low;
        uint16_t result = (uint16_t)a_val - (uint16_t)value + carry - 1;
        state->A.low = result & 0xFF;
        
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        
        set_flags_nz_8(machine, state->A.low);
        // Set Overflow flag: V = (A^M) & (A^result) & 0x80
        if (((a_val ^ value) & (a_val ^ result) & 0x80)) {
            set_flag(machine, OVERFLOW);
        } else {
            clear_flag(machine, OVERFLOW);
        }
    }
}

void sbc_16bit(machine_state_t *machine, uint16_t value) {
    processor_state_t *state = &machine->processor;
    uint32_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    
    if (is_flag_set(machine, DECIMAL_MODE)) {
        // Decimal (BCD) mode
        uint16_t a_val = state->A.full;
        bool carry_out = false;
        uint32_t result = bcd_subtract_16(a_val, value, carry, &carry_out);
        state->A.full = (uint16_t)(result & 0xFFFF);
        
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        set_flags_nz_16(machine, state->A.full);
        // Overflow flag is undefined in decimal mode
    } else {
        // Binary mode
        uint16_t a_val = state->A.full;
        uint32_t result = (uint32_t)a_val - (uint32_t)value + carry - 1;
        state->A.full = result & 0xFFFF;
        
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        
        set_flags_nz_16(machine, state->A.full);
        // Set Overflow flag: V = (A^M) & (A^result) & 0x8000
        if (((a_val ^ value) & (a_val ^ result) & 0x8000)) {
            set_flag(machine, OVERFLOW);
        } else {
            clear_flag(machine, OVERFLOW);
        }
    }
}

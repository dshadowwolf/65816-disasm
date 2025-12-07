#include "processor.h"
#include "machine.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "processor_helpers.h"

// TODO: refactor more to use the get_dp_address_XXX helpers

machine_state_t* XCE_CB(machine_state_t *machine, uint16_t unused1, uint16_t unused2) {
    bool carry = is_flag_set(machine, CARRY); // Check Carry flag (bit 0)
    bool emulation = machine->processor.emulation_mode;
    
    // Swap carry flag with emulation mode
    if (emulation) {
        set_flag(machine, CARRY);
    } else {
        clear_flag(machine, CARRY);
    }
    
    if (carry) {
        // Switch to emulation mode
        machine->processor.emulation_mode = true;
        machine->processor.SP &= 0x00FF;
        machine->processor.SP |= 0x0100; // Set high byte of SP in emulation mode
        machine->processor.X &= 0x00FF;   // in emulation mode X and Y are 8-bit and lose the high byte
        machine->processor.Y &= 0x00FF;
    } else {
        // Switch to native mode
        machine->processor.emulation_mode = false;
    }
    return machine;
}

machine_state_t* SEP_CB(machine_state_t *machine, uint16_t flag, uint16_t unused2) {
    if (flag & X_FLAG) {
        machine->processor.X &= 0x00FF; // Set X register to 8-bit
        machine->processor.Y &= 0x00FF; // Set Y register to 8-bit
    }
    return set_flag(machine, flag);
}

machine_state_t* REP_CB(machine_state_t *machine, uint16_t flag, uint16_t unused2) {
    return clear_flag(machine, flag);
}

machine_state_t* CLC_CB(machine_state_t *machine, uint16_t unused1, uint16_t unused2) {
    return clear_flag(machine, CARRY);
}

machine_state_t* SEC_CB(machine_state_t *machine, uint16_t unused1, uint16_t unused2) {
    return set_flag(machine, CARRY);
}

// Causes a software interrupt. The PC is loaded from the interrupt vector table from $00FFE6 in native mode, or $00FFF6 in emulation mode.
machine_state_t* BRK           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    // In native mode push the PBR onto the stack,
    // followed by the PC then the P register
    uint16_t pc = state->PC + 2; // BRK is a 2-byte instruction
    if (!state->emulation_mode) {
        push_byte_new(machine, state->PBR); // Push PBR
    }
    push_word_new(machine, pc); // Push PC
    push_byte_new(machine, state->P); // Push status register
    state->PBR = 0; // BRK forces PBR to 0
    clear_flag(machine, DECIMAL_MODE); // Clear Break flag
    // Set Interrupt Disable flag
    set_flag(machine, INTERRUPT_DISABLE);
    uint16_t vector_address = state->emulation_mode ? 0xFFFE : 0xFFE6;
    state->PC = read_word_new(machine, vector_address);
    return machine;
}

machine_state_t* ORA_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_x_new(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }

    return machine;
}

machine_state_t* COP           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    // In native mode push the PBR onto the stack,
    // followed by the PC then the P register
    uint16_t pc = state->PC + 2; // COP is a 2-byte instruction
    if (!state->emulation_mode) {
        push_byte_new(machine, state->PBR); // Push PBR
    }
    push_word_new(machine, pc); // Push PC
    push_byte_new(machine, state->P); // Push status register
    // Set Interrupt Disable flag
    set_flag(machine, INTERRUPT_DISABLE);
    state->PBR = 0; // BRK forces PBR to 0
    clear_flag(machine, DECIMAL_MODE); // Clear Break flag
    uint16_t vector = state->emulation_mode?0xFFF4:0xFFE4;
    state->PC = read_word_new(machine, vector);
    return machine;
}

machine_state_t* ORA_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    uint16_t address = get_stack_relative_address(machine, arg_one); // get the address we're working on
    uint8_t value = read_byte_dp_sr(machine, address);                       // load the operand value

    // do the operation, dependent on the size of the accumulator (the M_FLAG being set or clear)
    if (is_flag_set(machine, M_FLAG)) {
        machine->processor.A.low |= value;
        set_flags_nz_8(machine, machine->processor.A.low);
    } else {
        machine->processor.A.full |= value;
        set_flags_nz_16(machine, machine->processor.A.full);
    }
    return machine;
}

machine_state_t* TSB_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Set Bits - Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);

    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte_dp_sr(machine, dp_address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte_dp_sr(machine, dp_address, value | state->A.low);
    } else {
        uint16_t value = read_word_dp_sr(machine, dp_address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value | state->A.full;
        write_word_dp_sr(machine, dp_address, new_value);
    }
    return machine;
}

machine_state_t* ORA_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte_dp_sr(machine, dp_address);

    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte_dp_sr(machine, dp_address);
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t value = read_word_dp_sr(machine, dp_address);
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ASL_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte_dp_sr(machine, dp_address);

    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = ((uint16_t)value) << 1;
        write_byte_dp_sr(machine, dp_address, (uint8_t)(result & 0xFF));
        set_flags_nzc_8(machine, result);
    } else {
        uint32_t full_value = (uint32_t)(value << 1);
        write_word_dp_sr(machine, dp_address, (uint16_t)(full_value & 0xFFFF));
        set_flags_nzc_16(machine, full_value);
    }
    return machine;
}

machine_state_t* ORA_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    long_address_t effective_address = get_dp_address_indirect_long_new(machine, arg_one);
    uint8_t value = read_byte_long(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low |= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full |= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;    
}

machine_state_t* PHP           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if(state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP;
    push_byte_new(machine, state->P);
    return machine;
}

machine_state_t* ORA_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t value = (uint8_t)(arg_one & 0xFF);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ASL           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(state->A.low << 1);
        state->A.low = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        uint32_t full_result = (uint32_t)(state->A.full << 1);
        state->A.full = (uint16_t)(full_result & 0xFFFF);
        set_flags_nzc_16(machine, full_result);
    }
    return machine;
}

machine_state_t* PHD           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    if (is_flag_set(machine, M_FLAG)) {
        push_byte_new(machine, machine->processor.DP & 0xFF);
    } else {
        push_word_new(machine, machine->processor.DP);
    }
    return machine;
}

machine_state_t* TSB_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Set Bits - Absolute
    processor_state_t *state = &machine->processor;
    
    uint16_t address = get_absolute_address(machine, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte_new(machine, address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte_new(machine, address, value | state->A.low);
    } else {
        uint16_t value = read_word_new(machine, address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value | state->A.full;
        write_word_new(machine, address, new_value);
    }
    return machine;
}

machine_state_t* ORA_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t value = read_byte_new(machine, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ASL_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);

    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t value = read_byte_new(machine, address);
        uint16_t result = (uint16_t)(value << 1);
        write_byte_new(machine, address, (uint8_t)(result & 0xFF));
        set_flags_nzc_8(machine, result);
    } else {
        // 16-bit mode - read and write 16 bits
        uint16_t value = read_word_new(machine, address);
        uint32_t result = (uint32_t)(value << 1);
        write_word_new(machine, address, (uint16_t)(result & 0xFFFF));
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

// OR Accumulator with Memory, Absolute Long Addressing
// Highest 8 bits of address come from arg_two
// arg_one is the low 16 bits of the address
machine_state_t* ORA_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    long_address_t address = get_absolute_address_long(machine, arg_one, (uint8_t)(arg_two & 0xFF));
    uint8_t *memory_bank = get_memory_bank(machine, address.bank);
    uint8_t value = read_byte_long(machine, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* BPL_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, NEGATIVE)) {
        state->PC = (state->PC + (int8_t)(arg_one & 0xFF)) & 0xFFFF;
    }
    return machine;
}

machine_state_t* ORA_DP_I_IY      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_y(machine, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte_new(machine, effective_address);
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t value = read_word_new(machine, effective_address);
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }

    return machine;
}

machine_state_t* ORA_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_new(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ORA_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y_new(machine, arg_one);
    uint8_t value = read_byte_new(machine, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t value16 = read_word_new(machine, address);
        uint16_t result = state->A.full | value16;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* TRB_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Reset Bits - Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte_new(machine, dp_address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte_new(machine, dp_address, value & (~state->A.low));
    } else {
        uint16_t value = read_word_new(machine, dp_address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value & (~state->A.full);
        write_word_new(machine, dp_address, new_value);
    }
    return machine;
}

machine_state_t* ORA_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ASL_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ASL Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        write_byte_new(machine, effective_address, (uint8_t)(result & 0xFF));
        set_flags_nzc_8(machine, result);
    } else {
        uint32_t full_value = (uint32_t)(value << 1);
        write_word_new(machine, effective_address, (uint16_t)(full_value & 0xFFFF));
        set_flags_nzc_16(machine, full_value);
    }
    return machine;
}

machine_state_t* ORA_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Direct Page Indirect Long Indexed with Y)
    processor_state_t *state = &machine->processor;
    long_address_t effective_address_long = get_dp_address_indirect_long_indexed_y_new(machine, arg_one);
    // Add Y to the address portion
    uint8_t value = read_byte_long(machine, effective_address_long);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ORA_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Absolute Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_absolute_address_indexed_y(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* INC           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Increment Accumulator
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low = (state->A.low + 1) & 0xFF;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = (state->A.full + 1) & 0xFFFF;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* TCS           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // transfer 16-bit A to S
    processor_state_t *state = &machine->processor;
    state->SP = state->A.full;
    return machine;
}

machine_state_t* TRB_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Reset Bits - Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte_new(machine, address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte_new(machine, address, value & (~state->A.low));  // Clear bits
    } else {
        uint16_t value = read_word_new(machine, address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value & (~state->A.full);
        write_word_new(machine, address, new_value);
    }
    return machine;
}

machine_state_t* ORA_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Absolute Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* ASL_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ASL Absolute Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        write_byte_new(machine, address, (uint8_t)(result & 0xFF));
        check_and_set_carry_8(machine, result);
    } else {
        // Handle 16-bit mode
        uint16_t value16 = read_word_new(machine, address);
        uint32_t result = (uint32_t)(value16 << 1);
        write_word_new(machine, address, (uint16_t)(result & 0xFFFF));
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* ORA_ABL_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Absolute Long Indexed with X)
    processor_state_t *state = &machine->processor;
    long_address_t effective_address = get_absolute_address_long_indexed_x(machine, arg_one, (uint8_t)(arg_two & 0xFF));
    uint8_t value = read_byte_long(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* JSR_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to Subroutine
    processor_state_t *state = &machine->processor;
    uint16_t return_address = (state->PC - 1) & 0xFFFF;
    state->PC = arg_one & 0xFFFF; // Set PC to target address
    // Push return address onto stack
    push_word(machine, return_address);
    return machine;
}

machine_state_t* AND_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* JSL_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to Subroutine Long
    processor_state_t *state = &machine->processor;
    uint16_t return_address = (state->PC - 1) & 0xFFFF;
    // Push 24-bit return address: bank byte first, then 16-bit address
    push_byte_new(machine, state->PBR);
    push_word_new(machine, return_address);
    state->PC = arg_one & 0xFFFF; // Set PC to target address (ignoring bank for now)
    return machine;
}

machine_state_t* AND_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Stack Relative)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address(machine, arg_one);
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* BIT_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // test bits in memory with accumulator, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = machine->memory[state->PBR][dp_address];
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low & value;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full & value;
        check_and_set_zero_16(machine, result);
    }
    return machine;
}

machine_state_t* AND_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Direct Page)
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), dp_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* ROL_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Roll Left, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), dp_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFE;
        }
        write_byte(get_memory_bank(machine, state->DBR), dp_address, (uint8_t)(result & 0xFF));
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // Handle 16-bit mode
        uint32_t result = (uint32_t)(read_word(get_memory_bank(machine, state->DBR), dp_address) << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x0001;
        } else {
            result &= 0xFFFE;
        }
        write_word(get_memory_bank(machine, state->DBR), dp_address, (uint16_t)(result & 0xFFFF));
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* AND_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    uint8_t *dp_bank = get_memory_bank(machine, state->DBR);
    long_address_t effective_address = get_dp_address_indirect_long(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, effective_address.bank);
    uint8_t value = read_byte(memory_bank, effective_address.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value16 = read_word(memory_bank, effective_address.address);
        state->A.full &= value16;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* PLP           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Processor Status from Stack
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;

    state->P = pop_byte_new(machine);
    if (state->emulation_mode) {
        set_flag(machine, M_FLAG);
        set_flag(machine, X_FLAG);
    }
    return machine;
}

machine_state_t* AND_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Immediate
    processor_state_t *state = &machine->processor;
    uint8_t value = (uint8_t)(arg_one & 0xFF);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* ROL           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Accumulator Left
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(state->A.low << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFE;
        }
        state->A.low = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        // Handle 16-bit accumulator case here
        uint32_t result = (uint32_t)(state->A.full << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x0001;
        } else {
            result &= 0xFFFE;
        }
        state->A.full = (uint16_t)(result & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* PLD           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Direct Page Register from Stack
    if (is_flag_set(machine, M_FLAG)) {
        machine->processor.DP = pop_byte_new(machine);
        machine->processor.DP &= 0x00FF; // Zero upper byte in 8-bit mode
    } else {
        machine->processor.DP = pop_word_new(machine);
    }

    return machine;
}

machine_state_t* BIT_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test bits against accumulator, Absolute addressing
    // Sets Z based on A & M, N from bit 7 of M, V from bit 6 of M
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        uint8_t result = state->A.low & value;
        set_flags_nz_8(machine, result);
        if (value & 0x40) set_flag(machine, OVERFLOW);
        else clear_flag(machine, OVERFLOW);
    } else {
        uint16_t value = read_word(memory_bank, address);
        uint16_t result = state->A.full & value;
        set_flags_nz_16(machine, result);
        if (value & 0x4000) set_flag(machine, OVERFLOW);
        else clear_flag(machine, OVERFLOW);
    }
    return machine;
}

machine_state_t* AND_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Addressing)
    processor_state_t *state = &machine->processor;
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), get_absolute_address(machine, arg_one));
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* ROL_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Left, Absolute Addressing
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFE;
        }
        set_flags_nzc_8(machine, result);
        write_byte(memory_bank, address, (uint8_t)(result & 0xFF));
    } else {
        uint32_t result = (uint32_t)(state->A.full << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFFFFFFFE;
        }
        set_flags_nzc_16(machine, result);
        write_word(memory_bank, address, result);
    }
    return machine;
}

machine_state_t* AND_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Long Addressing)
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, (uint8_t)(arg_two & 0xFF));
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint8_t value = read_word(memory_bank, addr.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* BMI_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Callback for "Branch if Minus"
    processor_state_t *state = &machine->processor;
    int8_t offset = (int8_t)(arg_one & 0xFF);
    if (is_flag_set(machine, NEGATIVE)) {
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

machine_state_t* AND_DP_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Direct Page Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* AND_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Direct Page Indirect)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* AND_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Stack Relative Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* BIT_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test bits in memory with accumulator, Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low & value;
        set_flags_nz_8(machine, result);
        // BIT also copies bit 7 to N and bit 6 to V
        check_and_set_negative_8(machine, result);
        if (value & 0x40) set_flag(machine, OVERFLOW);
        else clear_flag(machine, OVERFLOW);
    } else {
        uint16_t value16 = (uint16_t)read_word(get_memory_bank(machine, state->DBR), address);
        uint16_t result = state->A.full & value16;
        check_and_set_zero_16(machine, result);
        // BIT also copies bit 15 to N and bit 14 to V
        check_and_set_negative_16(machine, result);
        if (value16 & 0x4000) set_flag(machine, OVERFLOW);
        else clear_flag(machine, OVERFLOW);
    }
    return machine;
}

machine_state_t* ROL_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Left, Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFE;
        }
        write_byte(memory_bank, effective_address, (uint8_t)(result & 0xFF));
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // Handle 16-bit mode
        uint32_t result = (uint32_t)(read_word(memory_bank, effective_address) << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x0001;
        } else {
            result &= 0xFFFE;
        }
        write_word(memory_bank, effective_address, (uint16_t)(result & 0xFFFF));
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

// TODO: Fix for long addressing
machine_state_t* AND_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Direct Page Indirect Long Indexed with Y)
    processor_state_t *state = &machine->processor;
    long_address_t effective_address = get_dp_address_indirect_long_indexed_y(machine, arg_one);
    uint8_t *effective_bank = get_memory_bank(machine, effective_address.bank);
    uint8_t value = read_byte(effective_bank, effective_address.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value16 = read_word( effective_bank, effective_address.address );
        state->A.full &= value16;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* AND_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* DEC           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Decrement Accumulator
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low = (state->A.low - 1) & 0xFF;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = (state->A.full - 1) & 0xFFFF;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* TSC           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer Stack Pointer to Accumulator
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low = (uint8_t)(state->SP & 0xFF);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = state->SP & 0xFFFF;
        // Set Zero and Negative flags
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* BIT_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test bits against accumulator, Absolute Indexed addressing
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low & value;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full & value;
        check_and_set_zero_16(machine, result);
    }
    return machine;
}

machine_state_t* AND_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND accumulator with Memory (Absolute Indexed Addressing)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* ROL_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Left, Absolute Indexed Addressing
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        uint16_t result = (uint16_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        }
        write_byte(memory_bank, address, (uint8_t)(result & 0xFF));
        check_and_set_carry_8(machine, result);
        set_flags_nz_8(machine, (uint8_t)(result & 0xFF));
    } else {
        // Handle 16-bit mode - read and write 16 bits
        uint16_t value = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x0001;
        }
        write_word(memory_bank, address, (uint16_t)(result & 0xFFFF));
        check_and_set_carry_16(machine, result);
        set_flags_nz_16(machine, (uint16_t)(result & 0xFFFF));
    }
    return machine;
}

machine_state_t* AND_ABL_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Long Indexed with X)
    processor_state_t *state = &machine->processor;
    long_address_t address = get_absolute_address_long_indexed_x(machine, arg_one, (uint8_t)(arg_two & 0xFF));
    uint8_t *memory_bank = get_memory_bank(machine, address.bank);
    uint8_t value = read_byte(memory_bank, address.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* RTI           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Return from Interrupt
    processor_state_t *state = &machine->processor;
    
    // Pop status register
    state->P = pop_byte(machine);
    
    // Pop program counter (16-bit)
    state->PC = pop_word(machine);
    
    // In native mode, also restore PBR
    if (!state->emulation_mode) {
        state->PBR = pop_byte(machine);
    }
    return machine;
}

machine_state_t* EOR_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value16 = read_word(memory_bank, effective_address);
        state->A.full ^= value16;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* WDM           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // WDM - Reserved for future use
    // TODO: trap on this, its technically an illegal instruction
    return machine;
}

machine_state_t* EOR_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Stack Relative)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_stack_relative_address(machine, arg_one);
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* MVP           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // MVP - Block Move Positive (MVP srcbank, dstbank) - actually decrements addresses!
    processor_state_t *state = &machine->processor;
    uint8_t *source_bank = get_memory_bank(machine, arg_two & 0xFF);
    uint8_t *dest_bank = get_memory_bank(machine, arg_one & 0xFF);
    uint16_t count = state->A.full + 1;
    uint16_t source_index = state->X;
    uint16_t dest_index = state->Y;
    for (uint16_t i = 0; i < count; i++) {
        write_byte(dest_bank, dest_index & 0xFFFF, read_byte(source_bank, source_index & 0xFFFF));
        source_index = (source_index - 1) & 0xFFFF;
        dest_index = (dest_index - 1) & 0xFFFF;
    }
    state->X = source_index & 0xFFFF;
    state->Y = dest_index & 0xFFFF;
    state->A.full = 0xFFFF;  // A becomes $FFFF after completion
    return machine;
}

machine_state_t* EOR_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Addressing)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte(memory_bank, dp_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LSR_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Direct Page Addressing
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte(memory_bank, dp_address);
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        if (value & 0x01) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        value >>= 1;
        write_byte(memory_bank, dp_address, value);
        set_flags_nz_8(machine, value);
    } else {
        // 16-bit mode
        uint16_t value16 = read_word(memory_bank, dp_address);
        // Set Carry flag based on bit 0
        if (value16 & 0x0001) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        value16 >>= 1;
        write_word(memory_bank, dp_address, value16);
        set_flags_nz_16(machine, value16);
    }
    return machine;
}

// TODO: Fix for long addressing
machine_state_t* EOR_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Long)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    long_address_t dp_address = get_dp_address_indirect_long(machine, arg_one);
    uint8_t *target_bank = get_memory_bank(machine, dp_address.bank);
    uint8_t value = read_byte(target_bank, dp_address.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* PHA           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Accumulator onto Stack
    if (machine->processor.emulation_mode || is_flag_set(machine, M_FLAG)) {
        push_byte(machine, machine->processor.A.low);
    } else {
        push_word(machine, machine->processor.A.full);
    }
    return machine;
}

machine_state_t* EOR_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Immediate Value
    processor_state_t *state = &machine->processor;
    uint8_t value = (uint8_t)arg_one;
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LSR           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        if (state->A.low & 0x01) {
            set_flag(machine, CARRY);
        } else {    
            clear_flag(machine, CARRY);
        }
        state->A.low >>= 1;
        set_flags_nz_8(machine, state->A.low);
    } else {
        // 16-bit mode
        // Set Carry flag based on bit 0
        if (state->A.full & 0x0001) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        state->A.full >>= 1;
        // Set Zero and Negative flags
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* PHK           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Program Bank onto Stack
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);  // Stack is always in bank 0
    uint16_t sp_address;

    push_byte(machine, state->PBR);
    return machine;
}

machine_state_t* JMP_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Direct Jump (Callback)
    processor_state_t *state = &machine->processor;
    state->PC = arg_one;
    return machine;
}

machine_state_t* EOR_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Addressing)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LSR_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Absolute Addressing
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        check_and_set_carry_8(machine, value);
        value >>= 1;
        write_byte(memory_bank, address, value);
        // Set Zero and Negative flags
        set_flags_nz_8(machine, value);
    } else {
        // 16-bit mode
        uint16_t value16 = read_word(memory_bank, address);
        // Set Carry flag based on bit 0
        check_and_set_carry_16(machine, value16);
        value16 >>= 1;
        write_word(memory_bank, address, value16);
        // Set Zero and Negative flags
        set_flags_nz_16(machine, value16);
    }
    return machine;
}

machine_state_t* EOR_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Long Addressing)
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, (uint8_t)(arg_two & 0xFF));
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint8_t value = read_byte(memory_bank, addr.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* BVC_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Overflow Clear (Callback)
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, OVERFLOW)) {
        state->PC = arg_one & 0xFFFF;
    }
    return machine;
}

machine_state_t* EOR_DP_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* EOR_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* EOR_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Stack Relative Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* MVN           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Move Block Negative (MVN srcbank, dstbank) - actually increments addresses!
    processor_state_t *state = &machine->processor;
    uint8_t *source_bank = get_memory_bank(machine, arg_two & 0xFF);
    uint8_t *dest_bank = get_memory_bank(machine, arg_one & 0xFF);
    uint16_t count = state->A.full + 1;
    uint16_t source_index = state->X;
    uint16_t dest_index = state->Y;
    for (uint16_t i = 0; i < count; i++) {
        write_byte(dest_bank, dest_index & 0xFFFF, read_byte(source_bank, source_index & 0xFFFF));
        source_index = (source_index + 1) & 0xFFFF;
        dest_index = (dest_index + 1) & 0xFFFF;
    }
    state->X = source_index;
    state->Y = dest_index;
    state->A.full = 0xFFFF;  // A becomes $FFFF after completion
    return machine;
}

machine_state_t* EOR_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LSR_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        check_and_set_carry_8(machine, value);
        value >>= 1;
        write_byte(memory_bank, effective_address, value);
        // Set Zero and Negative flags
        set_flags_nz_8(machine, value);
        clear_flag(machine, NEGATIVE);
    } else {
        // 16-bit mode
        uint16_t value16 = read_word(memory_bank, effective_address);
        // Set Carry flag based on bit 0
        check_and_set_carry_16(machine, value16);
        value16 >>= 1;
        write_word(memory_bank, effective_address, value16);
        // Set Zero and Negative flags
        set_flags_nz_16(machine, value16);
    }
    return machine;
}

machine_state_t* EOR_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Long Indexed with Y)
    processor_state_t *state = &machine->processor;
    long_address_t address = get_dp_address_indirect_long_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, address.bank);
    uint8_t value = read_byte(memory_bank, address.address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value16 = read_word(memory_bank, address.address);
        state->A.full ^= value16;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* CLI           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Clear Interrupt Disable Flag
    clear_flag(machine, INTERRUPT_DISABLE);
    machine->processor.interrupts_disabled = false;
    return machine;
}

machine_state_t* EOR_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address_indexed_y(machine, arg_one);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* PHY           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Y Register onto Stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        push_byte_new(machine, state->Y & 0xFF);
    } else {
        push_word_new(machine, state->Y);
    }
    return machine;
}

machine_state_t* TCD           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer Accumulator to Direct Page Register
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        state->DP = state->A.low;
    } else {
        state->DP = state->A.full;
    }
    return machine;
}

machine_state_t* JMP_AL        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to Absolute Long Address
    // arg_one = address (16-bit), arg_two = bank (8-bit)
    processor_state_t *state = &machine->processor;
    state->PC = arg_one & 0xFFFF;
    state->PBR = arg_two & 0xFF;
    return machine;
}

machine_state_t* EOR_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LSR_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Absolute Indexed with X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = value >> 1;
        check_and_set_carry_8(machine, value);
        write_byte(memory_bank, address, result);
        clear_flag(machine, NEGATIVE);
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = read_word(memory_bank, address);
        check_and_set_carry_16(machine, result);
        write_word(memory_bank, address, result);
        clear_flag(machine, NEGATIVE);
    }
    return machine;
}

machine_state_t* EOR_AL_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Long Indexed with X)
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long_indexed_x(machine, arg_one, (uint8_t)(arg_two & 0xFF));
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, addr.address);
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value = read_word(memory_bank, addr.address);
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* RTS           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Return from Subroutine
    processor_state_t *state = &machine->processor;
    state->PC = pop_word(machine);
    return machine;
}

machine_state_t* ADC_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint8_t value = read_byte(memory_bank, effective_address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        check_and_set_carry_8(machine, result);
    } else {
        uint16_t value16 = read_word(memory_bank, effective_address);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        check_and_set_carry_16(machine, result);
    }
    
    return machine;
}

machine_state_t* PER           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Program Counter Relative
    processor_state_t *state = &machine->processor;
    uint16_t pc_relative = (state->PC + (int8_t)arg_one) & 0xFFFF;
    push_word(machine, pc_relative);
    return machine;
}

machine_state_t* ADC_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Stack Relative)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, state->A.low);
    } else {
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        set_flags_nzc_16(machine, state->A.full);
    }
    
    return machine;
}

machine_state_t* STZ           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Store Zero
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *bank = get_memory_bank(machine, state->DBR);
    if (!is_flag_set(machine, M_FLAG)) {
        write_word(bank, address, 0x00);
    } else {
        write_byte(bank, address, 0x00);
    }
    return machine;
}

machine_state_t* ADC_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte(memory_bank, dp_address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, state->A.low);
    } else {
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        set_flags_nzc_16(machine, state->A.full);
    }
    
    return machine;
}

machine_state_t* ROR_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Direct Page
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = read_byte(memory_bank, dp_address);
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t carry_in = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
        uint16_t result = (value >> 1) | carry_in;
        write_byte(memory_bank, dp_address, (uint8_t)(result & 0xFF));
        clear_flag(machine, NEGATIVE);
        set_flags_nzc_8(machine, (uint8_t)(result & 0xFF));
    } else {
        // 16-bit mode
        uint16_t carry_in = is_flag_set(machine, CARRY) ? 0x8000 : 0x0000;
        uint32_t result = (value >> 1) | carry_in;
        write_word(memory_bank, dp_address, (uint16_t)(result & 0xFFFF));
        clear_flag(machine, NEGATIVE);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* ADC_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Long)
    processor_state_t *state = &machine->processor;
    uint8_t *dp_bank = get_memory_bank(machine, state->DBR);
    long_address_t address = get_dp_address_indirect_long(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, address.bank);
    uint8_t value = read_byte(memory_bank, address.address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, state->A.low);
    } else {
        uint16_t value16 = read_word(memory_bank, address.address);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        set_flags_nzc_16(machine, state->A.full);
    }
    
    return machine;
}

machine_state_t* PLA           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Accumulator from Stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = pop_byte(machine);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = pop_word(machine);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* ADC_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Immediate)
    processor_state_t *state = &machine->processor;
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(arg_one & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(arg_one & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* ROR           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Accumulator
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t carry_in = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
        // Set Carry flag based on bit 0
        if (state->A.low & 0x01) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        state->A.low = (state->A.low >> 1) | carry_in;
    } else {
        // 16-bit mode
        uint16_t carry_in = is_flag_set(machine, CARRY) ? 0x8000 : 0x0000;
        // Set Carry flag based on bit 0
        if (state->A.full & 0x0001) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        state->A.full = (state->A.full >> 1) | carry_in;
    }
    return machine;
}

machine_state_t* RTL           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Return from Subroutine Long
    processor_state_t *state = &machine->processor;
    // Pop 24-bit return address: 16-bit address first, then bank byte
    uint16_t return_address = pop_word_new(machine);
    uint8_t bank_byte = pop_byte_new(machine);
    state->PC = return_address;
    state->PBR = bank_byte;
    return machine;
}

machine_state_t* JMP_ABS_I     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump, Absolute Indirect
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indirect(machine, arg_one);
    state->PC = address;
    return machine;
}

machine_state_t* ADC_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Absolute)
    processor_state_t *state = &machine->processor;
    uint16_t base_address = get_absolute_address(machine, arg_one);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t value = read_byte_new(machine, base_address);
        uint16_t result = (uint16_t)state->A.low + (uint16_t)value + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value = read_word_new(machine, base_address);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)value + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* ROR_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t base_address = get_absolute_address(machine, arg_one);
    uint8_t value = read_byte_new(machine, base_address);
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t carry_in = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
        check_and_set_carry_8(machine, value);
        uint16_t result = (value >> 1) | carry_in;
        set_flags_nz_8(machine, result);
        write_byte_new(machine, base_address, result);
    } else {
        // 16-bit mode
        uint16_t carry_in = is_flag_set(machine, CARRY) ? 0x8000 : 0x0000;
        check_and_set_carry_16(machine, value);
        uint16_t value16 = read_word_new(machine, base_address);
        value16 = (value16 >> 1) | carry_in;
        set_flags_nz_16(machine, value16);
        write_word_new(machine, base_address, value16);
    }
    return machine;
}

// TODO: Fix for Long Addressing
machine_state_t* ADC_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Absolute Long)
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, arg_two);
    uint8_t value = read_byte_long(machine, addr);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(arg_one & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* BVS_PCR       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Overflow Set, Program Counter Relative Long
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, OVERFLOW)) {
        int16_t offset = (int8_t)arg_one;
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

machine_state_t* ADC_DP_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indirect_indexed_y_new(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value16 = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16 & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* ADC_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_new(machine, arg_one);
    uint8_t value = read_byte_new(machine, effective_address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value16 = read_word_new(machine, effective_address);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16 & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* ADC_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Stack Relative Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y_new(machine, arg_one);
    uint8_t value = read_byte_new(machine, address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value16 = read_word_new(machine, address);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16 & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* STZ_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Store Zero, Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    write_byte_new(machine, address, 0x00);
    return machine;
}

machine_state_t* ADC_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t value = read_byte_new(machine, address);
        uint16_t result = (uint16_t)state->A.low + (uint16_t)value + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value = read_word_new(machine, address);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)value + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* ROR_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
    uint8_t carry_out = value & 0x01;

    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        write_byte_new(machine, address, ((uint8_t)result) & 0xFF);
    } else {
        // 16-bit mode
        uint32_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        write_word_new(machine, address, result & 0xFFFF);
    }

    return machine;
}

machine_state_t* ADC_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Long Indexed by Y)
    processor_state_t *state = &machine->processor;
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;

    // get address somehow
    long_address_t address = get_dp_address_indirect_long_indexed_y_new(machine, arg_one);
    uint8_t value = read_byte_long(machine, address);

    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

machine_state_t* SEI           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) { // 0x78
    // Set Enable Interrupts
    processor_state_t *state = &machine->processor;
    state->interrupts_disabled = true;
    set_flag(machine, INTERRUPT_DISABLE);
    return machine;
}

machine_state_t* ADC_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t effective_addr = get_absolute_address_indexed_y(machine, arg_one);
    uint8_t carry_in = is_flag_set(machine, CARRY) ? 1 : 0;
    uint16_t value = read_byte_new(machine, effective_addr);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = state->A.low + value + carry_in;
        set_flags_nzc_8(machine, result);
        check_and_set_negative_8(machine, result);
        state->A.low = (uint8_t)(result & 0xFF);
    } else {
        uint32_t result = state->A.full + value + carry_in;
        set_flags_nzc_16(machine, result);
        state->A.full = (uint16_t)(result & 0xFFFF);
    }
    return machine;
}

machine_state_t* PLY           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Y register from the stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = pop_byte_new(machine);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = pop_word_new(machine);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* TDC           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer D Register to C
    processor_state_t *state = &machine->processor;
    state->A.full = state->DP;
    return machine;
}

// TODO: This passes testing, but the test is possibly broken as its not
//       passing if we use get_absolute_address_indirect_indexed_x()
machine_state_t* JMP_ABS_I_IX  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump (Absolute Indirect Indexed by X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->PBR);
    uint16_t indexed_address = get_absolute_address_indexed_x(machine, arg_one);
    uint16_t target_address = read_word_new(machine, indexed_address);
    state->PC = target_address;
    return machine;
}

machine_state_t* ADC_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint16_t mask = is_flag_set(machine, X_FLAG) ? 0xFF : 0xFFFF;
    uint8_t value = read_byte_new(machine, address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
    uint8_t carry_out = value & 0x01;

    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = state->A.low + value + carry;
        set_flags_nzc_8(machine, result);
        check_and_set_negative_8(machine, result);
        state->A.low = (uint8_t)(result & 0xFF);
    } else {
        uint32_t result = state->A.full + value + carry;
        set_flags_nzc_16(machine, result);
        state->A.full = (uint16_t)(result & 0xFFFF);
    }
    return machine;
}

machine_state_t* ROR_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Absolute Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte_new(machine, address);
    uint16_t carry = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
    uint8_t carry_out = value & 0x01;

    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        write_byte_new(machine, address, ((uint8_t)result) & 0xFF);
    } else {
        // 16-bit mode
        uint32_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        write_word_new(machine, address, result & 0xFFFF);
    }

    return machine;
}

machine_state_t* ADC_AL_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry, Absolute Long Indexed X
    processor_state_t *state = &machine->processor;
    uint16_t mask = is_flag_set(machine, X_FLAG) ? 0xFF : 0xFFFF;
    long_address_t effective_addr = get_absolute_long_indexed_x_new(machine, arg_one, arg_two);
    uint8_t carry_in = is_flag_set(machine, CARRY) ? 1 : 0;
    uint16_t value = read_byte_long(machine, effective_addr);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = state->A.low + value + carry_in;
        set_flags_nzc_8(machine, result);
        check_and_set_negative_8(machine, result);
        state->A.low = (uint8_t)(result & 0xFF);
    } else {
        uint32_t result = state->A.full + value + carry_in;
        set_flags_nzc_16(machine, result);
        state->A.full = (uint16_t)(result & 0xFFFF);
    }
    return machine;
}

machine_state_t* BRA_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch Immediate
    processor_state_t *state = &machine->processor;
    state->PC = arg_one & 0xFFFF;
    return machine;
}

machine_state_t* STA_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    // (DP,X) means: add X to DP+offset, THEN read pointer
    uint16_t address = get_dp_address_indirect_indexed_x_new(machine, arg_one);

    if( is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, state->A.low);
    } else {
        write_word_new(machine, address, state->A.full);
    }
    return machine;
}

machine_state_t* BRL_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch always Long
    // offset is arg_one, program bank is arg_two
    processor_state_t *state = &machine->processor;
    state->PBR = ((uint8_t)arg_two) & 0xFF;
    state->PC = arg_one;
    return machine;
}

machine_state_t* STA_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, state->A.low);
    } else {
        write_word_new(machine, address, state->A.full);
    }

    return machine;
}

machine_state_t* STY_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        write_byte_new(machine, effective_address, (uint8_t)state->Y & 0xFF);
    } else {
        write_word_new(machine, effective_address, state->Y & 0xFFFF);
    }

    return machine;
}

machine_state_t* STA_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, state->A.low);
    } else {
        write_word_new(machine, address, state->A.full);
    }

    return machine;
}

machine_state_t* STX_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        write_byte_new(machine, address, state->X & 0xFF);
    } else {
        write_word_new(machine, address, state->X & 0xFFFF);
    }

    return machine;
}

machine_state_t* STA_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_dp_address_indirect_long_new(machine, arg_one);
    printf("STA_DP_IL to bank %02X addr %04X\n", addr.bank, addr.address);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_long(machine, addr, state->A.low);
    } else {
        write_word_long(machine, addr, state->A.full);
    }

    return machine;
}

machine_state_t* DEY           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        // we're 8-bit on the index register either way...
        uint16_t ns = state->Y;
        ns -= 1;
        set_flags_nz_8(machine, ns);
        check_and_set_negative_8(machine, ns);
        state->Y = ((uint8_t)ns & 0xFF);
    } else {
        uint32_t ns = state->Y;
        ns -= 1;
        set_flags_nz_16(machine, ns);
        state->Y = ((uint16_t)ns & 0xFFFF);
    }
    
    return machine;
}

machine_state_t* BIT_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // BIT Immediate - only sets Z flag based on A & value
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = arg_one & 0xFF;
        uint8_t result = state->A.low & value;
        if (result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
    } else {
        uint16_t value = arg_one & 0xFFFF;
        uint16_t result = state->A.full & value;
        if (result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
    }
    return machine;
}

machine_state_t* TXA           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;

    if (state->emulation_mode || (is_flag_set(machine, X_FLAG) && is_flag_set(machine, M_FLAG))) {
        state->A.low = state->X & 0xFF;
    } else if (is_flag_set(machine, X_FLAG) && !is_flag_set(machine, M_FLAG)) {
        state->A.full = (uint16_t)state->X & 0xFF;
    } else {
        state->A.full = state->X;
    }

    return machine;
}

machine_state_t* PHB           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Data Bank register onto stack
    push_byte_new(machine, machine->processor.DBR);
    return machine;
}

machine_state_t* STY_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (is_flag_set(machine, X_FLAG) || state->emulation_mode) {
        write_byte_new(machine, address, (uint8_t)(state->Y & 0xFF));
    } else {
        write_word_new(machine, address, (uint16_t)(state->Y & 0xFFFF));
    }
    return machine;
}

machine_state_t* STA_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (is_flag_set(machine, M_FLAG) || state->emulation_mode) {
        write_byte_new(machine, address, state->A.low);
    } else {
        write_word_new(machine, address, state->A.full);
    }
    return machine;
}

machine_state_t* STX_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (is_flag_set(machine, X_FLAG) || state->emulation_mode) {
        write_byte_new(machine, address, (uint8_t)(state->X & 0xFF));
    } else {
        write_word_new(machine, address, (uint16_t)(state->X & 0xFFFF));
    }
    return machine;
}

machine_state_t* STA_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, arg_two);
    if (is_flag_set(machine, M_FLAG) || state->emulation_mode) {
        write_byte_long(machine, addr, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_long(machine, addr, state->A.full);
    }
    return machine;
}

machine_state_t* BCC_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, CARRY)) {
        state->PC = arg_one & 0xFFFF;
    }
    return machine;
}

machine_state_t* STA_DP_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect_indexed_y_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_new(machine, address, state->A.full);
    }
    return machine;
}

machine_state_t* STA_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_new(machine, address, state->A.full);
    }

    return machine;
}

machine_state_t* STA_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A, Stack-relative, Indirect, Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y_new(machine, arg_one);
    printf("STA_SR_I_IY address: %04X\n", address);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_new(machine, address, state->A.full);
    }

    return machine;
}

machine_state_t* STY_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Y register, Direct Page, Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        write_byte_new(machine, effective_address, (uint8_t)state->Y & 0xFF);
    } else {
        write_word_new(machine, effective_address, state->Y);
    }
    return machine;
}

machine_state_t* STA_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Direct Page, Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        write_byte_new(machine, effective_address, (uint8_t)state->A.low);
    } else {
        write_word_new(machine, effective_address, state->A.full);
    }
    return machine;}

machine_state_t* STX_DP_IY     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Y register, Direct Page, Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_y(machine, arg_one);

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        write_byte_new(machine, effective_address, (uint8_t)state->X & 0xFF);
    } else {
        write_word_new(machine, effective_address, state->X);
    }
    return machine;
}

machine_state_t* STA_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Direct Page, Indirect Long Indexed by Y
    processor_state_t *state = &machine->processor;
    long_address_t dp_address = get_dp_address_indirect_long_indexed_y_new(machine, arg_one);

    if (state->emulation_mode | is_flag_set(machine, M_FLAG)) {
        write_byte_long(machine, dp_address, state->A.low);
    } else {
        write_word_long(machine, dp_address, state->A.full);
    }
    return machine;
}

machine_state_t* TYA           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer Y to A
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || (is_flag_set(machine, X_FLAG) && is_flag_set(machine, M_FLAG))) {
        state->A.low = state->Y & 0xFF;
        set_flags_nz_8(machine, state->A.low);
    } else if (is_flag_set(machine, X_FLAG) && !is_flag_set(machine, M_FLAG)) {
        state->A.full = (uint16_t)state->Y & 0xFF;
        set_flags_nz_16(machine, state->A.full);
    } else {
        state->A.full = state->Y;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* STA_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_absolute_address_indexed_y(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, effective_address, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_new(machine, effective_address, state->A.full);
    }
    return machine;
}

machine_state_t* TXS           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer X to Stack Pointer
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->SP = (state->X & 0xFF) | 0x100;
    } else {
        state->SP = state->X & 0xFFFF;
    }
    return machine;
}

machine_state_t* TXY           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer X to Y
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || (is_flag_set(machine, X_FLAG) && is_flag_set(machine, X_FLAG))) {
        state->Y = state->X & 0xFF;
        set_flags_nz_8(machine, state->Y);
    } else if (is_flag_set(machine, X_FLAG) && !is_flag_set(machine, X_FLAG)) {
        state->Y = (uint16_t)(state->X & 0xFF);
        set_flags_nz_16(machine, state->Y);
    } else {
        state->Y = state->X;
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* STZ_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Zero, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, 0x00);
    } else {
        write_word_new(machine, address, 0x0000);
    }
    return machine;
}

machine_state_t* STA_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Absolute Indexed X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_new(machine, address, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_new(machine, address, state->A.full);
    }
    return machine;
}

machine_state_t* STZ_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Zero, Absolute Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte(memory_bank, address, 0x00);
    } else {
        write_word(memory_bank, address, 0x0000);
    }
    return machine;
}

machine_state_t* STA_ABL_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Absolute Long Indexed X
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long_indexed_x(machine, arg_one, (uint8_t)arg_two);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        write_byte_long(machine, addr, (uint8_t)(state->A.low & 0xFF));
    } else {
        write_word_long(machine, addr, state->A.full);
    }
    return machine;
}

machine_state_t* LDY_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y Immediate
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = (uint8_t)(arg_one & 0xFF);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = arg_one & 0xFFFF;
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* LDA_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address_indirect_indexed_x_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, dp_address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, dp_address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDX_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X Immediate
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = (uint8_t)(arg_one & 0xFF);
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = arg_one & 0xFFFF;
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* LDA_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Stack Relative
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDY_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = read_byte_new(machine, dp_address);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = read_word_new(machine, dp_address);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* LDA_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, dp_address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, dp_address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDX_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = read_byte_new(machine, dp_address);
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = read_word_new(machine, dp_address);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* LDA_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    long_address_t address = get_dp_address_indirect_long_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_long(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_long(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* TAY           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer A to Y
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        // 8-bit index register
        state->Y = state->A.full & 0xFF;
        set_flags_nz_8(machine, state->Y);
    } else {
        // 16-bit index register
        state->Y = state->A.full & 0xFFFF;
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* LDA_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A Immediate
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = (uint8_t)(arg_one & 0xFF);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = arg_one & 0xFFFF;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* TAX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer A to X
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        // 8-bit index register
        state->X = state->A.full & 0xFF;
        set_flags_nz_8(machine, state->X);
    } else {
        // 16-bit index register
        state->X = state->A.full & 0xFFFF;
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* PLB           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // PuLl Data Bank register from stack
    processor_state_t *state = &machine->processor;
    state->DBR = pop_byte_new(machine);
    set_flags_nz_8(machine, state->DBR);
    return machine;
}

machine_state_t* LDY_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = read_word_new(machine, address);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* LDA_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDX_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = read_word_new(machine, address);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* LDA_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Long
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, arg_two);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_long(machine, addr);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_long(machine, addr);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* BCS_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Carry Set, Callback
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, CARRY)) {
        state->PC = arg_one & 0xFFFF;
    }
    return machine;
}

machine_state_t* LDA_DP_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect_indexed_y_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDA_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDA_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Stack Relative Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDY_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = read_word_new(machine, address);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* LDA_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, dp_address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, dp_address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDX_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Direct Page Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = read_byte_new(machine, address);
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = read_word_new(machine, address);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* LDA_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Long Indexed by Y
    processor_state_t *state = &machine->processor;
    long_address_t address = get_dp_address_indirect_long_indexed_y_new(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_long(machine, address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_long(machine, address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* CLV           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CLear oVerflow flag
    clear_flag(machine, OVERFLOW);
    return machine;
}

machine_state_t* LDA_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_absolute_address_indexed_y(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte_new(machine, effective_address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word_new(machine, effective_address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* TSX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer Stack Pointer to X
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = state->SP & 0xFF;
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = state->SP & 0xFFFF;
        check_and_set_zero_16(machine, state->X);        
    }
    return machine;
}

machine_state_t* TYX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer Y to X
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = state->Y & 0xFF;
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = state->Y & 0xFFFF;
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* LDY_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = read_byte(memory_bank, effective_address);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = read_word(memory_bank, effective_address);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* LDA_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte(memory_bank, effective_address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word(memory_bank, effective_address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* LDX_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_y(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = read_byte(memory_bank, effective_address);
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = read_word(memory_bank, effective_address);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

machine_state_t* LDA_AL_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Long Indexed by X
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long_indexed_x(machine, arg_one, (uint8_t)arg_two);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t effective_address = addr.address;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = read_byte(memory_bank, effective_address);
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full = read_word(memory_bank, effective_address);
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* CPY_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare Y, Immediate
    processor_state_t *state = &machine->processor;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = (uint8_t)(arg_one & 0xFF);
        uint8_t result = (state->Y & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = arg_one & 0xFFFF;
        uint16_t result = (state->Y & 0xFFFF) - (value_to_compare & 0xFFFF);
        if ((state->Y & 0xFFFF) >= (value_to_compare & 0xFFFF)) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        set_flags_nz_16(machine, result);
    }
    return machine;
}

machine_state_t* CMP_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t addr = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, addr);
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, addr);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* CMP_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Stack Relative
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* CPY_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare Y, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint8_t result = (state->Y & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint16_t result = (state->Y & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* CMP_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* DEC_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        value = (value - 1) & 0xFF;
        write_byte(memory_bank, address, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, address);
        value = (value - 1) & 0xFFFF;
        write_word(memory_bank, address, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* CMP_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    long_address_t address = get_dp_address_indirect_long(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, address.bank);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address.address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, address.address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* INY           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INcrement Y
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = (state->Y + 1) & 0xFF;
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = (state->Y + 1) & 0xFFFF;
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

machine_state_t* CMP_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Immediate
    processor_state_t *state = &machine->processor;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)(arg_one & 0xFF);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = arg_one & 0xFFFF;
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* DEX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrememt X
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = (state->X - 1) & 0xFF;
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = (state->X - 1) & 0xFFFF;
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

// TODO: Implement when full hardware emulation begins
machine_state_t* WAI           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // WAit for Interrupt
    processor_state_t *state = &machine->processor;
    // ??? what here ?
    return machine;
}

machine_state_t* CPY_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare Y, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint8_t result = (state->Y & 0xFF) - (value_to_compare & 0xFF);
        if ((state->Y & 0xFF) >= (value_to_compare & 0xFF)) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        set_flags_nz_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint16_t result = (state->Y & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* CMP_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* DEC_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        value = (value - 1) & 0xFF;
        write_byte(memory_bank, address, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, address);
        value = (value - 1) & 0xFFFF;
        write_word(memory_bank, address, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* CMP_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Long
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, arg_two);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, addr.address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, addr.address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* BNE_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Not Equal (Zero flag clear)
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, ZERO)) {
        int8_t offset = (int8_t)(arg_one & 0xFF);
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

machine_state_t* CMP_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* CMP_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Stack Relative Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* PEI_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Effective Indirect, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect(machine, arg_one);
    push_word(machine, address);
    return machine;
}

machine_state_t* CMP_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* DEC_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        value = (value - 1) & 0xFF;
        write_byte(memory_bank, address, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, address);
        value = (value - 1) & 0xFFFF;
        write_word(memory_bank, address, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* CMP_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_dp_address_indirect_long_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, addr.address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, addr.address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* CLD_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CLear Decimal Flag
    clear_flag(machine, DECIMAL_MODE);
    return machine;
}

machine_state_t* CMP_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_y(machine, arg_one);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, effective_address);
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, effective_address);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* PHX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // PusH X
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        push_byte(machine, state->X & 0xFF);
    } else {
        push_word(machine, state->X);
    }
    return machine;
}

// TODO: Implement when full hardware emulation begins
machine_state_t* STP           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SToP the processor
    // ??? what here ?
    return machine;
}

machine_state_t* JMP_ABS_IL    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // JuMP to Absolute Indirect Long address
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long_indirect(machine, arg_one, arg_two);
    uint16_t effective_address = addr.address;
    state->PC = effective_address;
    state->PBR = addr.bank;
    return machine;
}

machine_state_t* CMP_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, effective_address);
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, effective_address);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* DEC_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, effective_address);
        value = (value - 1) & 0xFF;
        write_byte(memory_bank, effective_address, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, effective_address);
        value = (value - 1) & 0xFFFF;
        write_word(memory_bank, effective_address, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* CMP_ABL_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Long Indexed by X
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long_indexed_x(machine, arg_one, arg_two);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t effective_address = addr.address;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = read_byte(memory_bank, effective_address);
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, effective_address);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* CPX_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare X, Immediate
    processor_state_t *state = &machine->processor;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = (uint8_t)(arg_one & 0xFF);
        uint16_t result = (uint16_t)(state->X & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = arg_one & 0xFFFF;
        uint32_t result = (uint32_t)(state->X & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* SBC_DP_I_IX   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, effective_address);
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = read_word(memory_bank, effective_address);
        uint32_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint32_t result = (uint32_t)state->A.full - (uint32_t)value_to_subtract - borrow;
        state->A.full = result & 0xFFFF;
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* SBC_SR        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Stack Relative
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint32_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint32_t result = (uint32_t)state->A.full - (uint32_t)value_to_subtract - borrow;
        state->A.full = result & 0xFFFF;
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* CPX_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare X, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint8_t result = (state->X & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint16_t result = (state->X & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

machine_state_t* SBC_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* INC_DP        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        value = (value + 1) & 0xFF;
        write_byte(memory_bank, address, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, address);
        value = (value + 1) & 0xFFFF;
        write_word(memory_bank, address, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* SBC_DP_IL     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    long_address_t address = get_dp_address_indirect_long(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, address.bank);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address.address);
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = read_word(memory_bank, address.address);
        uint32_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint32_t result = (uint32_t)state->A.full - (uint32_t)value_to_subtract - borrow;
        state->A.full = result & 0xFFFF;
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* INX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INcrement X
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = (state->X + 1) & 0xFF;
        set_flags_nz_8(machine, state->X & 0xFF);
    } else {
        state->X = (state->X + 1) & 0xFFFF;
        set_flags_nz_16(machine, state->X & 0xFFFF);
    }
    return machine;
}

machine_state_t* SBC_IMM       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Immediate
    processor_state_t *state = &machine->processor;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)(arg_one & 0xFF);
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred (result >= 0)
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = arg_one & 0xFFFF;
        uint32_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint32_t result = (uint32_t)state->A.full - (uint32_t)value_to_subtract - borrow;
        state->A.full = result & 0xFFFF;
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

machine_state_t* NOP           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) { // opcode 0xEA
    // No OPeration
    // New simplest ever instruction
    return machine;
}

machine_state_t* XBA           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // eXchange B and A
    processor_state_t *state = &machine->processor;
    uint8_t temp = state->A.low;
    state->A.low = state->A.high;
    state->A.high = temp;
    // Set flags based on new A low byte
    set_flags_nz_8(machine, state->A.low & 0xFF);
    return machine;
}

machine_state_t* CPX_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare X, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address(machine, arg_one);
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = read_byte(memory_bank, address);
        uint16_t result = (uint16_t)(state->X & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(state->X & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

machine_state_t* SBC_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = get_absolute_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* INC_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement value, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = get_absolute_address(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        value = (value + 1) & 0xFF;
        write_byte(memory_bank, arg_one, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, address);
        value = (value + 1) & 0xFFFF;
        write_word(memory_bank, arg_one, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* SBC_ABL       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Long
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long(machine, arg_one, arg_two);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, addr.address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, addr.address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* BEQ_CB        (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Equal (Zero flag set)
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, ZERO)) {
        int8_t offset = (int8_t)(arg_one & 0xFF);
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

machine_state_t* SBC_DP_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* SBC_DP_I      (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* SBC_SR_I_IY   (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Stack Relative Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t address = get_stack_relative_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* PEA_ABS       (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Effective Absolute Address
    uint16_t effective_address = get_absolute_address(machine, arg_one);
    push_word(machine, effective_address);
    return machine;
}

machine_state_t* SBC_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* INC_DP_IX     (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        value = (value + 1) & 0xFF;
        write_byte(memory_bank, address, value);
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, address);
        value = (value + 1) & 0xFFFF;
        write_word(memory_bank, address, value);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* SBC_DP_IL_IY  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Long Indexed by Y
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_dp_address_indirect_long_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, addr.address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, addr.address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* SED           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SEt Decimal flag
    set_flag(machine, DECIMAL_MODE);
    return machine;
}

machine_state_t* SBC_ABS_IY    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_y(machine, arg_one);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, effective_address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, effective_address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* PLX           (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // PuLl X from stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = pop_byte(machine);
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = pop_word(machine);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

// TODO: Verify correct behavior of JSR_ABS_I_IX
//       Test is possibly incorrect
machine_state_t* JSR_ABS_I_IX  (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to SubRoutine, Absolute Indexed Indirect (Indexed by X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->PBR);
    uint16_t indexed_address = get_absolute_address_indexed_x(machine, arg_one);
    uint16_t target_address = read_word(memory_bank, indexed_address);
    
    // Push return address minus 1 onto stack
    uint16_t return_address = (state->PC - 1) & 0xFFFF;
    push_word(machine, return_address);
    
    state->PC = target_address;
    return machine;
}

machine_state_t* SBC_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, effective_address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, effective_address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

machine_state_t* INC_ABS_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_absolute_address_indexed_x(machine, arg_one);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, effective_address);
        value = (value + 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = read_word(memory_bank, effective_address);
        value = (value + 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

machine_state_t* SBC_ABL_IX    (machine_state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Long Indexed by X
    processor_state_t *state = &machine->processor;
    long_address_t addr = get_absolute_address_long_indexed_x(machine, arg_one, arg_two);
    uint8_t *memory_bank = get_memory_bank(machine, addr.bank);
    uint16_t effective_address = addr.address;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = read_byte(memory_bank, effective_address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = read_word(memory_bank, effective_address);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

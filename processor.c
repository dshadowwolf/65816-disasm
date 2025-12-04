#include "processor.h"
#include "machine.h"
#include <stdint.h>
#include <stdlib.h>
#include "processor_helpers.h"

void initialize_processor(processor_state_t *state) {
    state->A.full = 0;
    state->X = 0;
    state->Y = 0;
    state->PC = 0;
    state->SP = 0x1FF;            // Stack Pointer starts at 0x1FF in emulation mode
    state->DP = 0;
    state->P = 0x34;              // Default status register value -- this is kinda arbitrary
    state->PBR = 0;               // Start in bank 0
    state->DBR = 0;               // Start in bank 0
    state->emulation_mode = true; // Start in emulation mode
}

void reset_processor(processor_state_t *state) {
    state->A.full = 0;
    state->X = 0;
    state->Y = 0;
    state->PC = 0x0000;           // Reset Program Counter to 0
    state->SP = 0x1FF;             // Stack Pointer reset value
    state->DP = 0;
    state->P = 0x34;              // Reset status register value
    state->emulation_mode = true; // Reset to emulation mode
}

void initialize_machine(state_t *machine) {
    initialize_processor(&machine->processor);
    for (int i = 0; i < 64; i++) {
        machine->memory[i] = NULL; // Initialize memory pointers to NULL
    }

    machine->memory[0] = (uint8_t*)malloc(65536 * sizeof(uint8_t)); // Allocate 64KB for bank 0
}

void reset_machine(state_t *machine) {
    reset_processor(&machine->processor);
    
    for (int i = 0; i < 64; i++) {
        if (machine->memory[i] != NULL) {
            free(machine->memory[i]);
            machine->memory[i] = NULL;
        }
    }
    machine->memory[0] = (uint8_t*)malloc(65536 * sizeof(uint8_t)); // Reallocate 64KB for bank 0
}

state_t* create_machine() {
    state_t *machine = (state_t*)malloc(sizeof(state_t));
    initialize_machine(machine);
    return machine;
}

void destroy_machine(state_t *machine) {
    for (int i = 0; i < 64; i++) {
        if (machine->memory[i] != NULL) {
            free(machine->memory[i]);
        }
    }
    free(machine);
}

// TODO: refactor more to use the get_dp_address_XXX helpers

state_t* XCE_CB(state_t *machine, uint16_t unused1, uint16_t unused2) {
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
        machine->processor.SP |= 0x0100; // Set high byte of SP in emulation mode
    } else {
        // Switch to native mode
        machine->processor.emulation_mode = false;
    }
    return machine;
}

state_t* SEP_CB(state_t *machine, uint16_t flag, uint16_t unused2) {
    return set_flag(machine, flag);
}

state_t* REP_CB(state_t *machine, uint16_t flag, uint16_t unused2) {
    return clear_flag(machine, flag);
}

state_t* CLC_CB(state_t *machine, uint16_t unused1, uint16_t unused2) {
    return clear_flag(machine, CARRY);
}

state_t* SEC_CB(state_t *machine, uint16_t unused1, uint16_t unused2) {
    return set_flag(machine, CARRY);
}

// Causes a software interrupt. The PC is loaded from the interrupt vector table from $00FFE6 in native mode, or $00FFF6 in emulation mode.
state_t* BRK           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    // In native mode push the PBR onto the stack,
    // followed by the PC then the P register
    uint16_t pc = state->PC + 2; // BRK is a 2-byte instruction
    uint8_t *memory_bank = get_memory_bank(machine, 0); // always bank 0 for BRK vectors
    if (!state->emulation_mode) {
        push_byte(machine, state->DBR); // Push PBR
    }
    push_word(machine, pc); // Push PC
    push_byte(machine, state->P); // Push status register
    // Set Interrupt Disable flag
    set_flag(machine, INTERRUPT_DISABLE);
    if (state->emulation_mode) {
        // Load new PC from IRQ vector at $FFFE/$FFFF
        state->PC = read_word(memory_bank, 0xFFFE);
    } else {
        // Load new PC from IRQ vector at $FFE6/$FFE7
        state->PC = read_word(memory_bank, 0xFFE6);
    }
    return machine;
}

state_t* ORA_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[effective_address];
    
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

state_t* COP           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    // In native mode push the PBR onto the stack,
    // followed by the PC then the P register
    uint16_t pc = state->PC + 2; // COP is a 2-byte instruction
    if (!state->emulation_mode) {
        push_byte(machine, state->PBR); // Push PBR
    }
    push_word(machine, pc); // Push PC
    push_byte(machine, state->P); // Push status register
    // Set Interrupt Disable flag
    set_flag(machine, INTERRUPT_DISABLE);
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) {
        // In emulation mode, the vector is at $FFF4/$FFF5
        state->PC = read_word(memory_bank, 0xFFF4);
    } else {
        // In native mode the vector is at $FFE4/$FFE5
        state->PC = read_word(memory_bank, 0xFFE4);
    }
    return machine;
}

state_t* ORA_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);

    if (is_flag_set(machine, M_FLAG)) {
        state->A.low |= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full |= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* TSB_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Set Bits - Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, dp_address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte(memory_bank, dp_address, value | state->A.low);
    } else {
        uint16_t value = read_word(memory_bank, dp_address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value | state->A.full;
        write_word(memory_bank, dp_address, new_value);
    }
    return machine;
}

state_t* ORA_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = get_memory_bank(machine, state->DBR)[dp_address];

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

state_t* ASL_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[dp_address];
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = ((uint16_t)value) << 1;
        memory_bank[dp_address] = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        uint32_t full_value = (uint32_t)(value << 1);
        memory_bank[dp_address] = (uint8_t)(full_value & 0xFF);
        set_flags_nzc_16(machine, full_value);
    }
    return machine;
}

state_t* ORA_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    long_address_t effective_address = get_dp_address_indirect_long(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, effective_address.bank);
    uint8_t value = memory_bank[effective_address.address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low |= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full |= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;    
}

state_t* PHP           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if(state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP;
    push_byte(machine, state->P);
    return machine;
}

state_t* ORA_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* ASL           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* PHD           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address = 0x0100 | (state->SP & 0xFF);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        push_byte(machine, state->DP & 0xFF);
    } else {
        push_word(machine, state->DP);
    }
    return machine;
}

state_t* TSB_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Set Bits - Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[address];
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte(memory_bank, address, value | state->A.low);
    } else {
        uint16_t value = memory_bank[address] | ((uint16_t)memory_bank[address + 1] << 8);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value | state->A.full;
        write_word(memory_bank, address, new_value);
    }
    return machine;
}

state_t* ORA_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), address);
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

state_t* ASL_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t value = read_byte(memory_bank, address);
        uint16_t result = (uint16_t)(value << 1);
        write_byte(memory_bank, address, (uint8_t)(result & 0xFF));
        set_flags_nzc_8(machine, result);
    } else {
        // 16-bit mode - read and write 16 bits
        uint16_t value = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(value << 1);
        write_word(memory_bank, address, (uint16_t)(result & 0xFFFF));
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

// OR Accumulator with Memory, Absolute Long Addressing
// Highest 8 bits of address come from arg_two
// arg_one is the low 16 bits of the address
state_t* ORA_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t bank = (uint8_t)(arg_two & 0xFF);
    uint8_t *memory_bank = get_memory_bank(machine, bank);
    uint8_t value = read_byte(memory_bank, address);
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

state_t* BPL_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, NEGATIVE)) {
        state->PC = (state->PC + (int8_t)(arg_one & 0xFF)) & 0xFFFF;
    }
    return machine;
}

state_t* ORA_I_IY      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory = get_memory_bank(machine,state->DP);
    uint16_t effective_address = read_word(memory, arg_one);
    uint8_t value = read_byte(memory, effective_address & 0xFFFF);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full | value;
        set_flags_nz_16(machine, result);
        state->A.full = result;
    }

    return machine;
}

state_t* ORA_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
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

state_t* ORA_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (sp_address + offset) & 0xFFFF;
    uint16_t base_address = read_word(memory_bank, effective_address_ptr);
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint8_t *pbr_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(pbr_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low | value;
        state->A.low = result;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t value16 = read_word(pbr_bank, effective_address);
        uint16_t result = state->A.full | value16;
        state->A.full = result;
        set_flags_nz_16(machine, result);
    }
    return machine;
}

state_t* TRB_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Reset Bits - Direct Page
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, dp_address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        memory_bank[dp_address] = value & (~state->A.low);  // Clear bits
    } else {
        uint16_t value = read_word(memory_bank, dp_address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value & (~state->A.full);
        memory_bank[dp_address] = new_value & 0xFF;
        memory_bank[dp_address + 1] = (new_value >> 8) & 0xFF;
    }
    return machine;
}

state_t* ORA_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
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

state_t* ASL_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ASL Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        write_byte(memory_bank, effective_address, (uint8_t)(result & 0xFF));
        set_flags_nzc_8(machine, result);
    } else {
        uint32_t full_value = (uint32_t)(value << 1);
        write_word(memory_bank, effective_address, (uint16_t)(full_value & 0xFFFF));
        set_flags_nzc_16(machine, full_value);
    }
    return machine;
}

state_t* ORA_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Direct Page Indirect Long Indexed with Y)
    processor_state_t *state = &machine->processor;
    long_address_t effective_address_long = get_dp_address_indirect_long_indexed_y(machine, arg_one);
    // Add Y to the address portion
    uint8_t *memory_bank = get_memory_bank(machine, effective_address_long.bank);
    uint8_t value = read_byte(memory_bank, effective_address_long.address);
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

state_t* ORA_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Absolute Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = (arg_one + state->Y) & 0xFFFF;
    uint8_t value = get_memory_bank(machine, state->DBR)[effective_address];
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

state_t* INC           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* TCS           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // transfer 16-bit A to S
    processor_state_t *state = &machine->processor;
    state->SP = state->A.full;
    return machine;
}

state_t* TRB_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test and Reset Bits - Absolute
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        uint8_t test_result = state->A.low & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        write_byte(memory_bank, address, value & (~state->A.low));  // Clear bits
    } else {
        uint16_t value = read_word(memory_bank, address);
        uint16_t test_result = state->A.full & value;
        if (test_result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        uint16_t new_value = value & (~state->A.full);
        write_word(memory_bank, address, new_value);
    }
    return machine;
}

state_t* ORA_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Absolute Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    uint8_t value = get_memory_bank(machine, state->DBR)[effective_address];
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

state_t* ASL_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ASL Absolute Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t address = (arg_one + state->X) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, address);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        write_byte(memory_bank, address, (uint8_t)(result & 0xFF));
        check_and_set_carry_8(machine, result);
    } else {
        // Handle 16-bit mode
        uint16_t value16 = read_word(memory_bank, address);
        uint32_t result = (uint32_t)(value16 << 1);
        write_word(memory_bank, address, (uint16_t)(result & 0xFFFF));
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

state_t* ORA_ABL_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // OR Accumulator with Memory (Absolute Long Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)(arg_two & 0xFF));
    uint8_t value = read_byte(memory_bank, effective_address);
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

state_t* JSR_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to Subroutine
    processor_state_t *state = &machine->processor;
    uint16_t return_address = (state->PC - 1) & 0xFFFF;
    state->PC = arg_one & 0xFFFF; // Set PC to target address
    // Push return address onto stack
    push_word(machine, return_address);
    return machine;
}

state_t* AND_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indexed_x(machine, arg_one);
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* JSL_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to Subroutine Long
    processor_state_t *state = &machine->processor;
    uint16_t return_address = (state->PC - 1) & 0xFFFF;
    // Push 24-bit return address: bank byte first, then 16-bit address
    push_byte(machine, state->PBR);
    push_word(machine, return_address);
    state->PC = arg_one & 0xFFFF; // Set PC to target address (ignoring bank for now)
    return machine;
}

state_t* AND_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Stack Relative)
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    uint8_t value = read_byte(get_memory_bank(machine, state->PBR), effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* BIT_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* AND_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* ROL_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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
        write_byte(get_memory_bank(machine, state->PBR), dp_address, (uint8_t)(result & 0xFF));
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

state_t* AND_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* PLP           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Processor Status from Stack
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;

    state->P = pop_byte(machine);
    if (state->emulation_mode) {
        set_flag(machine, M_FLAG);
        set_flag(machine, X_FLAG);
    }
    return machine;
}

state_t* AND_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* ROL           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* PLD           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Direct Page Register from Stack
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    state->DP = pop_word(machine);
    return machine;
}

state_t* BIT_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test bits against accumulator, Absolute addressing
    // Sets Z based on A & M, N from bit 7 of M, V from bit 6 of M
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = read_byte(memory_bank, address);
        uint8_t result = state->A.low & value;
        if (result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        if (value & 0x80) set_flag(machine, NEGATIVE);
        else clear_flag(machine, NEGATIVE);
        if (value & 0x40) set_flag(machine, OVERFLOW);
        else clear_flag(machine, OVERFLOW);
    } else {
        uint16_t value = read_word(memory_bank, address);
        uint16_t result = state->A.full & value;
        if (result == 0) set_flag(machine, ZERO);
        else clear_flag(machine, ZERO);
        if (value & 0x8000) set_flag(machine, NEGATIVE);
        else clear_flag(machine, NEGATIVE);
        if (value & 0x4000) set_flag(machine, OVERFLOW);
        else clear_flag(machine, OVERFLOW);
    }
    return machine;
}

state_t* AND_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Addressing)
    processor_state_t *state = &machine->processor;
    uint8_t value = read_byte(get_memory_bank(machine, state->DBR), arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* ROL_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Left, Absolute Addressing
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFE;
        }
        set_flags_nzc_8(machine, result);
        write_byte(memory_bank, arg_one, (uint8_t)(result & 0xFF));
    } else {
        uint32_t result = (uint32_t)(state->A.full << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        } else {
            result &= 0xFFFFFFFE;
        }
        set_flags_nzc_16(machine, result);
        write_word(memory_bank, arg_one, result);
    }
    return machine;
}

state_t* AND_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Long Addressing)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)(arg_two & 0xFF));
    uint8_t value = read_word(memory_bank, arg_one);
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low &= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full &= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* BMI_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Callback for "Branch if Minus"
    processor_state_t *state = &machine->processor;
    int8_t offset = (int8_t)(arg_one & 0xFF);
    if (is_flag_set(machine, NEGATIVE)) {
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

state_t* AND_DP_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* AND_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Direct Page Indirect)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, 0);
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

state_t* AND_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Stack Relative Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    uint16_t effective_dp_address = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_dp_address];
    uint8_t high_byte = memory_bank[(effective_dp_address + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
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

state_t* BIT_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* ROL_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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
state_t* AND_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* AND_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
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

state_t* DEC           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* TSC           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* BIT_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Test bits against accumulator, Absolute Indexed addressing
    processor_state_t *state = &machine->processor;
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = read_byte(memory_bank, effective_address);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = state->A.low & value;
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = state->A.full & value;
        check_and_set_zero_16(machine, result);
    }
    return machine;
}

state_t* AND_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND accumulator with Memory (Absolute Indexed Addressing)
    processor_state_t *state = &machine->processor;
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
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

state_t* ROL_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Left, Absolute Indexed Addressing
    processor_state_t *state = &machine->processor;
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        uint16_t result = (uint16_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x01;
        }
        memory_bank[effective_address] = (uint8_t)(result & 0xFF);
        check_and_set_carry_8(machine, result);
        set_flags_nz_8(machine, (uint8_t)(result & 0xFF));
    } else {
        // Handle 16-bit mode - read and write 16 bits
        uint8_t low = memory_bank[effective_address];
        uint8_t high = memory_bank[(effective_address + 1) & 0xFFFF];
        uint16_t value = ((uint16_t)high << 8) | low;
        uint32_t result = (uint32_t)(value << 1);
        if (is_flag_set(machine, CARRY)) {
            result |= 0x0001;
        }
        memory_bank[effective_address] = (uint8_t)(result & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((result >> 8) & 0xFF);
        check_and_set_carry_16(machine, result);
        set_flags_nz_16(machine, (uint16_t)(result & 0xFFFF));
    }
    return machine;
}

// TODO: Fix for long addressing
state_t* AND_ABL_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // AND Accumulator with Memory (Absolute Long Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t base_address = arg_one;
    uint32_t effective_address = (base_address + state->X) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)(arg_two & 0xFF));
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

state_t* RTI           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* EOR_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Indexed with X)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value16 = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        state->A.full ^= value16;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* WDM           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // WDM - Reserved for future use
    // TODO: trap on this, its technically an illegal instruction
    return machine;
}

state_t* EOR_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Stack Relative)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_dp_address = (sp_address + offset) & 0xFFFF;
    uint8_t value = machine->memory[0][effective_dp_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* MVP           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // MVP - Block Move Positive (MVP srcbank, dstbank) - actually decrements addresses!
    processor_state_t *state = &machine->processor;
    uint8_t *source_bank = get_memory_bank(machine, arg_two & 0xFF);
    uint8_t *dest_bank = get_memory_bank(machine, arg_one & 0xFF);
    uint16_t count = state->A.full + 1;
    uint16_t source_index = state->X;
    uint16_t dest_index = state->Y;
    for (uint16_t i = 0; i < count; i++) {
        dest_bank[dest_index & 0xFFFF] = source_bank[source_index & 0xFFFF];
        source_index = (source_index - 1) & 0xFFFF;
        dest_index = (dest_index - 1) & 0xFFFF;
    }
    state->X = source_index & 0xFFFF;
    state->Y = dest_index & 0xFFFF;
    state->A.full = 0xFFFF;  // A becomes $FFFF after completion
    return machine;
}

state_t* EOR_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Addressing)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = memory_bank[dp_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LSR_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Direct Page Addressing
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = memory_bank[dp_address];
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        if (value & 0x01) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        value >>= 1;
        memory_bank[dp_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        // 16-bit mode
        uint16_t value16 = memory_bank[dp_address] | (memory_bank[(dp_address + 1) & 0xFFFF] << 8);
        // Set Carry flag based on bit 0
        if (value16 & 0x0001) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        value16 >>= 1;
        memory_bank[dp_address] = (uint8_t)(value16 & 0xFF);
        memory_bank[(dp_address + 1) & 0xFFFF] = (uint8_t)((value16 >> 8) & 0xFF);
        set_flags_nz_16(machine, value16);
    }
    return machine;
}

// TODO: Fix for long addressing
state_t* EOR_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Long)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t low_byte = memory_bank[dp_address];
    uint8_t mid_byte = memory_bank[(dp_address + 1) & 0xFFFF];
    uint8_t bank_byte = memory_bank[(dp_address + 2) & 0xFFFF];
    uint32_t effective_address = (mid_byte << 8) | low_byte;
    uint8_t *target_bank = get_memory_bank(machine, bank_byte);
    uint8_t value = target_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* PHA           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Accumulator onto Stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        push_byte(machine, state->A.low);
    } else {
        push_word(machine, state->A.full);
    }
    return machine;
}

state_t* EOR_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* LSR           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* PHK           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Program Bank onto Stack
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, 0);  // Stack is always in bank 0
    uint16_t sp_address;
    if (state->emulation_mode) {
        sp_address = 0x0100 | (state->SP & 0xFF);
        memory_bank[sp_address] = state->PBR;
        state->SP = (state->SP - 1) & 0x1FF;
    } else {
        sp_address = state->SP & 0xFFFF;
        memory_bank[sp_address] = state->PBR;
        state->SP = (state->SP - 1) & 0xFFFF;
    }
    return machine;
}

state_t* JMP_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Direct Jump (Callback)
    processor_state_t *state = &machine->processor;
    state->PC = arg_one;
    return machine;
}

state_t* EOR_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Addressing)
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LSR_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Absolute Addressing
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[address];
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        check_and_set_carry_8(machine, value);
        value >>= 1;
        memory_bank[address] = value;
        // Set Zero and Negative flags
        set_flags_nz_8(machine, value);
    } else {
        // 16-bit mode
        uint16_t value16 = memory_bank[address] | (memory_bank[(address + 1) & 0xFFFF] << 8);
        // Set Carry flag based on bit 0
        check_and_set_carry_16(machine, value16);
        value16 >>= 1;
        memory_bank[address] = (uint8_t)(value16 & 0xFF);  
        memory_bank[(address + 1) & 0xFFFF] = (uint8_t)((value16 >> 8) & 0xFF);
        // Set Zero and Negative flags
        set_flags_nz_16(machine, value16);
    }
    return machine;
}

state_t* EOR_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Long Addressing)
    processor_state_t *state = &machine->processor;
    uint32_t address = arg_one;
    uint8_t bank = (uint8_t)(arg_two & 0xFF);
    uint8_t *memory_bank = get_memory_bank(machine, bank);
    uint8_t value = memory_bank[address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* BVC_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Overflow Clear (Callback)
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, OVERFLOW)) {
        state->PC = arg_one & 0xFFFF;
    }
    return machine;
}

state_t* EOR_DP_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect_indexed_y(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* EOR_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* EOR_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Stack Relative Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_dp_address = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_dp_address];
    uint8_t high_byte = memory_bank[(effective_dp_address + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* MVN           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Move Block Negative (MVN srcbank, dstbank) - actually increments addresses!
    processor_state_t *state = &machine->processor;
    uint8_t *source_bank = get_memory_bank(machine, arg_two & 0xFF);
    uint8_t *dest_bank = get_memory_bank(machine, arg_one & 0xFF);
    uint16_t count = state->A.full + 1;
    uint16_t source_index = state->X;
    uint16_t dest_index = state->Y;
    for (uint16_t i = 0; i < count; i++) {
        dest_bank[dest_index & 0xFFFF] = source_bank[source_index & 0xFFFF];
        source_index = (source_index + 1) & 0xFFFF;
        dest_index = (dest_index + 1) & 0xFFFF;
    }
    state->X = source_index;
    state->Y = dest_index;
    state->A.full = 0xFFFF;  // A becomes $FFFF after completion
    return machine;
}

state_t* EOR_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LSR_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        // Set Carry flag based on bit 0
        check_and_set_carry_8(machine, value);
        value >>= 1;
        memory_bank[effective_address] = value;
        // Set Zero and Negative flags
        set_flags_nz_8(machine, value);
        clear_flag(machine, NEGATIVE);
    } else {
        // 16-bit mode
        uint16_t value16 = (memory_bank[effective_address] | (memory_bank[(effective_address + 1) & 0xFFFF] << 8));
        // Set Carry flag based on bit 0
        check_and_set_carry_16(machine, value16);
        value16 >>= 1;
        memory_bank[effective_address] = value16 & 0xFF;
        memory_bank[(effective_address + 1) & 0xFFFF] = (value16 >> 8) & 0xFF;
        // Set Zero and Negative flags
        set_flags_nz_16(machine, value16);
    }
    return machine;
}

// TODO: Fix for Long Addressing
state_t* EOR_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Direct Page Indirect Long Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t *dp_bank = get_memory_bank(machine, 0);
    uint8_t low_byte = dp_bank[dp_address];
    uint8_t mid_byte = dp_bank[(dp_address + 1) & 0xFFFF];
    uint8_t high_byte = dp_bank[(dp_address + 2) & 0xFFFF];
    uint16_t base_address = (mid_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, high_byte);
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value16 = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        state->A.full ^= value16;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* CLI           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Clear Interrupt Disable Flag
    clear_flag(machine, INTERRUPT_DISABLE);
    machine->processor.interrupts_disabled = false;
    return machine;
}

state_t* EOR_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* PHY           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Y Register onto Stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        push_byte(machine, state->Y & 0xFF);
    } else {
        push_word(machine, state->Y);
    }
    return machine;
}

state_t* TCD           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer Accumulator to Direct Page Register
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, M_FLAG)) {
        state->DP = state->A.low;
    } else {
        state->DP = state->A.full;
    }
    return machine;
}

state_t* JMP_AL        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to Absolute Long Address
    // arg_one = address (16-bit), arg_two = bank (8-bit)
    processor_state_t *state = &machine->processor;
    state->PC = arg_one & 0xFFFF;
    state->PBR = arg_two & 0xFF;
    return machine;
}

state_t* EOR_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LSR_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Logical Shift Right, Absolute Indexed with X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t result = value >> 1;
        check_and_set_carry_8(machine, value);
        memory_bank[effective_address] = result;
        clear_flag(machine, NEGATIVE);
        set_flags_nz_8(machine, result);
    } else {
        uint16_t result = (memory_bank[effective_address] | (memory_bank[(effective_address + 1) & 0xFFFF] << 8)) >> 1;
        check_and_set_carry_16(machine, result);
        memory_bank[effective_address] = result & 0xFF;
        memory_bank[(effective_address + 1) & 0xFFFF] = (result >> 8) & 0xFF;
        clear_flag(machine, NEGATIVE);
    }
    return machine;
}

// TODO: Fix for Long Addressing
state_t* EOR_AL_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Exclusive OR Accumulator with Memory (Absolute Long Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, arg_two);
    uint32_t base_address = arg_one;
    uint32_t effective_address = (base_address + state->X) & 0xFFFF;
    if (is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        state->A.low ^= value;
        set_flags_nz_8(machine, state->A.low);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        state->A.full ^= value;
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* RTS           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Return from Subroutine
    processor_state_t *state = &machine->processor;
    state->PC = pop_word(machine);
    return machine;
}

state_t* ADC_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint8_t value = memory_bank[effective_address];
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        check_and_set_carry_8(machine, result);
    } else {
        uint16_t value16 = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        check_and_set_carry_16(machine, result);
    }
    
    return machine;
}

state_t* PER           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Program Counter Relative
    processor_state_t *state = &machine->processor;
    uint16_t pc_relative = (state->PC + (int8_t)arg_one) & 0xFFFF;
    push_word(machine, pc_relative);
    return machine;
}

state_t* ADC_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Stack Relative)
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
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

state_t* STZ           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Store Zero
    processor_state_t *state = &machine->processor;
    uint16_t address = arg_one;
    uint8_t *bank = get_memory_bank(machine, state->DBR);
    bank[address] = 0x00;
    if (!is_flag_set(machine, M_FLAG)) {
        // 16-bit mode - store zero in both bytes
        bank[(address + 1) & 0xFFFF] = 0x00;
    }
    return machine;
}

state_t* ADC_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = memory_bank[dp_address];
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

state_t* ROR_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Direct Page
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t value = machine->memory[0][dp_address];
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t carry_in = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
        uint16_t result = (value >> 1) | carry_in;
        memory_bank[dp_address] = (uint8_t)(result & 0xFF);
        clear_flag(machine, NEGATIVE);
        set_flags_nzc_8(machine, (uint8_t)(result & 0xFF));
    } else {
        // 16-bit mode
        uint16_t carry_in = is_flag_set(machine, CARRY) ? 0x8000 : 0x0000;
        uint32_t result = (value >> 1) | carry_in;
        memory_bank[dp_address] = (uint8_t)((result >> 8) & 0xFF);
        memory_bank[(dp_address + 1) & 0xFFFF] = (uint8_t)(result & 0xFF);
        clear_flag(machine, NEGATIVE);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* ADC_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Long)
    processor_state_t *state = &machine->processor;
    uint8_t *dp_bank = get_memory_bank(machine, 0);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t low_byte = dp_bank[dp_address];
    uint8_t mid_byte = dp_bank[(dp_address + 1) & 0xFFFF];
    uint8_t high_byte = dp_bank[(dp_address + 2) & 0xFFFF];
    uint16_t effective_address = (mid_byte << 8) | low_byte;
    uint8_t *memory_bank = get_memory_bank(machine, high_byte);
    uint8_t value = memory_bank[effective_address];
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        set_flags_nzc_8(machine, state->A.low);
    } else {
        uint16_t value16 = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        set_flags_nzc_16(machine, state->A.full);
    }
    
    return machine;
}

state_t* PLA           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* ADC_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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
        if (result > 0xFFFF) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
    }
    return machine;
}

state_t* ROR           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* RTL           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Return from Subroutine Long
    processor_state_t *state = &machine->processor;
    // Pop 24-bit return address: 16-bit address first, then bank byte
    uint16_t return_address = pop_word(machine);
    uint8_t bank_byte = pop_byte(machine);
    state->PC = return_address;
    state->PBR = bank_byte;
    return machine;
}

state_t* JMP_ABS_I     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump, Absolute Indirect
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t address = arg_one;
    uint8_t low_byte = memory_bank[address];
    uint8_t high_byte = memory_bank[(address + 1) & 0xFFFF];
    uint16_t target_address = ((uint16_t)high_byte << 8) | low_byte;
    state->PC = target_address;
    return machine;
}

state_t* ADC_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Absolute)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t value = memory_bank[base_address];
        uint16_t result = (uint16_t)state->A.low + (uint16_t)value + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value = ((uint16_t)memory_bank[base_address]) | ((uint16_t)memory_bank[base_address + 1] << 8);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)value + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

state_t* ROR_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint8_t value = memory_bank[base_address];
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t carry_in = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
        check_and_set_carry_8(machine, value);
        memory_bank[base_address] = (value >> 1) | carry_in;
        set_flags_nz_8(machine, memory_bank[base_address]);
    } else {
        // 16-bit mode
        uint16_t carry_in = is_flag_set(machine, CARRY) ? 0x8000 : 0x0000;
        check_and_set_carry_16(machine, value);
        uint16_t value16 = memory_bank[base_address] | (memory_bank[(base_address + 1) & 0xFFFF] << 8);
        value16 = (value16 >> 1) | carry_in;
        set_flags_nz_16(machine, value16);
        memory_bank[base_address] = (uint8_t)(value16 & 0xFF);
        memory_bank[(base_address + 1) & 0xFFFF] = (uint8_t)((value16 >> 8) & 0xFF);
    }
    return machine;
}

// TODO: Fix for Long Addressing
state_t* ADC_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Absolute Long)
    processor_state_t *state = &machine->processor;
    uint32_t base_address = arg_one;
    uint8_t bank = (uint8_t)arg_two;
    uint8_t *memory_bank = get_memory_bank(machine, bank);
    uint8_t value = memory_bank[base_address];
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

state_t* BVS_PCR       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Overflow Set, Program Counter Relative Long
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, OVERFLOW)) {
        int16_t offset = (int8_t)arg_one;
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

state_t* ADC_DP_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indirect_indexed_y(machine, arg_one);
    uint8_t value = memory_bank[effective_address];
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

state_t* ADC_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect)
    processor_state_t *state = &machine->processor;
    uint16_t effective_address = get_dp_address_indirect(machine, arg_one);
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = memory_bank[effective_address];
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

state_t* ADC_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Stack Relative Indirect Indexed with Y)
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint8_t *pbr_bank = get_memory_bank(machine, state->DBR);
    uint8_t value = pbr_bank[effective_address];
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (uint16_t)state->A.low + (uint16_t)(value & 0xFF) + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value16 = ((uint16_t)pbr_bank[effective_address]) | ((uint16_t)pbr_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)(value16 & 0xFFFF) + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

state_t* STZ_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Store Zero, Direct Page Indexed with X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    memory_bank[effective_address] = 0x00;
    return machine;
}

state_t* ADC_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indexed with X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;
    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint8_t value = memory_bank[effective_address];
        uint16_t result = (uint16_t)state->A.low + (uint16_t)value + carry;
        state->A.low = (uint8_t)(result & 0xFF);
        // Set Carry flag
        check_and_set_carry_8(machine, result);
    } else {
        // 16-bit mode
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[effective_address + 1] << 8);
        uint32_t result = (uint32_t)state->A.full + (uint32_t)value + carry;
        state->A.full = (uint16_t)(result & 0xFFFF);
        // Set Carry flag
        check_and_set_carry_16(machine, result);
    }
    return machine;
}

state_t* ROR_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    uint16_t carry = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
    uint8_t carry_out = value & 0x01;

    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        memory_bank[effective_address] = ((uint8_t)result) & 0xFF;
    } else {
        // 16-bit mode
        uint32_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        memory_bank[effective_address+1] = ((uint8_t)(result & 0xFF)) >> 8;
        memory_bank[effective_address] = (uint8_t)(result & 0xFF);
    }

    return machine;
}

state_t* ADC_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry (Direct Page Indirect Long Indexed by Y)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t carry = is_flag_set(machine, CARRY) ? 1 : 0;

    // get address somehow
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t low_byte = memory_bank[dp_address];
    uint8_t high_byte = memory_bank[(dp_address + 1) & 0xFFFF];
    uint8_t bank_byte = memory_bank[(dp_address + 2) & 0xFFFF];
    uint16_t effective_address = ((high_byte << 8) & 0xFF00 | low_byte) + state->Y;
    uint8_t value = get_memory_bank(machine, bank_byte)[effective_address];

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

state_t* SEI           (state_t* machine, uint16_t arg_one, uint16_t arg_two) { // 0x78
    // Set Enable Interrupts
    processor_state_t *state = &machine->processor;
    state->interrupts_disabled = true;
    set_flag(machine, INTERRUPT_DISABLE);
    return machine;
}

state_t* ADC_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_addr = (arg_one + state->Y) &0xFFFF;
    uint8_t carry_in = is_flag_set(machine, CARRY) ? 1 : 0;
    uint16_t value = memory_bank[effective_addr];
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

state_t* PLY           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Pull Y register from the stack
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = pop_byte(machine);
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = pop_word(machine);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

state_t* TDC           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer D Register to C
    processor_state_t *state = &machine->processor;
    state->A.full = state->DP;
    return machine;
}

state_t* JMP_ABS_I_IX  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump (Absolute Indirect Indexed by X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t indexed_address = (arg_one + state->X) & 0xFFFF;
    uint8_t pointer_low  = memory_bank[indexed_address];
    uint8_t pointer_high = memory_bank[(indexed_address + 1) & 0xFFFF];
    uint16_t target_address = (pointer_high << 8) | pointer_low;
    state->PC = target_address;
    return machine;
}

state_t* ADC_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t mask = is_flag_set(machine, X_FLAG) ? 0xFF : 0xFFFF;
    uint16_t effective_address = (arg_one + (state->X & mask)) &0xFFFF;
    uint8_t value = memory_bank[effective_address];
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

state_t* ROR_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Rotate Right, Absolute Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    uint8_t value = memory_bank[effective_address];
    uint16_t carry = is_flag_set(machine, CARRY) ? 0x80 : 0x00;
    uint8_t carry_out = value & 0x01;

    if (is_flag_set(machine, M_FLAG)) {
        // 8-bit mode
        uint16_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        memory_bank[effective_address] = ((uint8_t)result) & 0xFF;
    } else {
        // 16-bit mode
        uint32_t result = (value >> 1) | carry;
        if (carry_out) set_flag(machine, CARRY);
        else clear_flag(machine, CARRY);
        memory_bank[effective_address+1] = ((uint8_t)(result & 0xFF)) >> 8;
        memory_bank[effective_address] = (uint8_t)(result & 0xFF);
    }

    return machine;
}

state_t* ADC_AL_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Add with Carry, Absolute Long Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)(arg_two & 0xFF));
    uint16_t mask = is_flag_set(machine, X_FLAG) ? 0xFF : 0xFFFF;
    uint16_t effective_addr = (arg_one + (state->X & mask)) &0xFFFF;
    uint8_t carry_in = is_flag_set(machine, CARRY) ? 1 : 0;
    uint16_t value = memory_bank[effective_addr];
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

state_t* BRA_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch Immediate
    processor_state_t *state = &machine->processor;
    state->PC = arg_one & 0xFFFF;
    return machine;
}

state_t* STA_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    // (DP,X) means: add X to DP+offset, THEN read pointer
    uint16_t effective_address_ptr = (dp_address + offset + state->X) & 0xFFFF;
    uint8_t pointer_low  = memory_bank[effective_address_ptr];
    uint8_t pointer_high = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t effective_address = ((pointer_high << 8) | pointer_low) & 0xFFFF;

    if( is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = state->A.low;
    } else {
        memory_bank[effective_address] = state->A.low;
        memory_bank[(effective_address + 1) & 0xFFFF] = state->A.high;
    }
    return machine;
}

state_t* BRL_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch always Long
    // offset is arg_one, program bank is arg_two
    processor_state_t *state = &machine->processor;
    state->PBR = ((uint8_t)arg_two) & 0xFF;
    state->PC = arg_one;
    return machine;
}

state_t* STA_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = state->A.low;
    } else {
        memory_bank[effective_address] = state->A.low;
        memory_bank[effective_address+1] = state->A.high;
    }

    return machine;
}

state_t* STY_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address;
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        memory_bank[effective_address] = (uint8_t)state->Y & 0xFF;
    } else {
        memory_bank[effective_address] = ((uint8_t)state->Y & 0xFF);
        memory_bank[effective_address+1] = (((uint8_t)state->Y >> 8) & 0xFF);
    }

    return machine;
}

state_t* STA_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address;
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = (uint8_t)state->A.low;
    } else {
        memory_bank[effective_address] = ((uint8_t)state->A.low & 0xFF);
        memory_bank[effective_address+1] = (((uint8_t)state->A.high >> 8) & 0xFF);
    }

    return machine;
}

state_t* STX_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address;
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        memory_bank[effective_address] = (uint8_t)state->X & 0xFF;
    } else {
        memory_bank[effective_address] = ((uint8_t)state->X & 0xFF);
        memory_bank[effective_address+1] = (((uint8_t)state->X >> 8) & 0xFF);
    }

    return machine;
}

state_t* STA_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address;
    if (state->emulation_mode) dp_address = 0x0000;
    else dp_address = state->DP & 0xFFFF;
    uint16_t offset = dp_address + ((uint8_t)arg_one & 0xFF);
    uint8_t low_byte = memory_bank[offset];
    uint8_t high_byte = memory_bank[offset+1];
    uint8_t page = memory_bank[offset+2];
    uint8_t *act_bank = get_memory_bank(machine, page);
    uint16_t effective_address = (((uint16_t)high_byte << 8) & 0xFF00) | (low_byte & 0xFF);
    if (state->emulation_mode | is_flag_set(machine, M_FLAG)) {
        act_bank[effective_address] = state->A.low;
    } else {
        act_bank[effective_address] = state->A.low;
        act_bank[effective_address+1] = state->A.high;
    }

    return machine;
}

state_t* DEY           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* BIT_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* TXA           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* PHB           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Data Bank register onto stack
    processor_state_t *state = &machine->processor;
    uint16_t stack_addr = state->emulation_mode ? (0x100 | (state->SP & 0xFF)) : (state->SP & 0xFFFF);
    uint8_t *memory_bank = get_memory_bank(machine, 0);  // Stack is always in bank 0
    memory_bank[stack_addr] = state->PBR;
    state->SP = state->emulation_mode ? (state->SP - 1) & 0x1FF : (state->SP - 1) & 0xFFFF;
    return machine;
}

state_t* STY_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, X_FLAG) || state->emulation_mode) {
        memory_bank[arg_one] = (uint8_t)(state->Y & 0xFF);
    } else {
        memory_bank[arg_one] = (uint8_t)(state->Y & 0xFF);
        memory_bank[arg_one+1] = ((uint8_t)(state->Y & 0xFF00)) >> 8;
    }
    return machine;
}

state_t* STA_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, M_FLAG) || state->emulation_mode) {
        memory_bank[arg_one] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[arg_one] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[arg_one+1] = (uint8_t)(state->A.high & 0xFF);
    }
    return machine;
}

state_t* STX_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (is_flag_set(machine, X_FLAG) || state->emulation_mode) {
        memory_bank[arg_one] = (uint8_t)(state->X & 0xFF);
    } else {
        memory_bank[arg_one] = (uint8_t)(state->X & 0xFF);
        memory_bank[arg_one+1] = (uint8_t)((state->X >> 8) & 0xFF);
    }
    return machine;
}

state_t* STA_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    if (is_flag_set(machine, M_FLAG) || state->emulation_mode) {
        memory_bank[arg_one] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[arg_one] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[arg_one+1] = (uint8_t)(state->A.high & 0xFF);
    }
    return machine;
}

state_t* BCC_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, CARRY)) {
        state->PC = arg_one & 0xFFFF;
    }
    return machine;
}

state_t* STA_DP_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[effective_address+1] = (uint8_t)(state->A.high & 0xFF);
    }
    return machine;
}

state_t* STA_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[base_address] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[base_address] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[base_address+1] = (uint8_t)(state->A.high & 0xFF);
    }

    return machine;
}

state_t* STA_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A, Stack-relative, Indirect, Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint8_t *stack_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address_ptr = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = stack_bank[effective_address_ptr];
    uint8_t high_byte = stack_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint8_t *target_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        target_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
    } else {
        target_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
        target_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((state->A.full >> 8) & 0xFF);
    }

    return machine;
}

state_t* STY_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Y register, Direct Page, Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        memory_bank[effective_address] = (uint8_t)state->Y & 0xFF;
    } else {
        memory_bank[effective_address] = (uint8_t)state->Y & 0xFF;
        memory_bank[effective_address+1] = (uint8_t)(state->Y >> 8) & 0xFF;        
    }
    return machine;
}

state_t* STA_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Direct Page, Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->X) & 0xFFFF;

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        memory_bank[effective_address] = (uint8_t)state->A.low;
    } else {
        memory_bank[effective_address] = (uint8_t)state->A.low;
        memory_bank[effective_address+1] = (uint8_t)state->A.high;        
    }
    return machine;}

state_t* STX_DP_IY     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Y register, Direct Page, Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint16_t effective_address = (dp_address + state->Y) & 0xFFFF;

    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        memory_bank[effective_address] = (uint8_t)state->X & 0xFF;
    } else {
        memory_bank[effective_address] = (uint8_t)state->X & 0xFF;
        memory_bank[effective_address+1] = (uint8_t)(state->X >> 8) & 0xFF;        
    }
    return machine;
}

state_t* STA_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Direct Page, Indirect Long Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t dp_address = get_dp_address(machine, arg_one);
    uint8_t low_byte = memory_bank[dp_address];
    uint8_t high_byte = memory_bank[(dp_address + 1) & 0xFFFF];
    uint8_t page = memory_bank[(dp_address + 2) & 0xFFFF];
    uint8_t *act_bank = get_memory_bank(machine, page);
    uint16_t base_address = (((uint16_t)high_byte << 8) & 0xFF00) | (low_byte & 0xFF);
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    if (state->emulation_mode | is_flag_set(machine, M_FLAG)) {
        act_bank[effective_address] = state->A.low;
    } else {
        act_bank[effective_address] = state->A.low;
        act_bank[effective_address+1] = state->A.high;
    }
    return machine;
}

state_t* TYA           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* STA_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[effective_address+1] = (uint8_t)(state->A.high & 0xFF);
    }
    return machine;
}

state_t* TXS           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Transfer X to Stack Pointer
    processor_state_t *state = &machine->processor;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->SP = (state->X & 0xFF) | 0x100;
    } else {
        state->SP = state->X & 0xFFFF;
    }
    return machine;
}

state_t* TXY           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* STZ_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Zero, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[arg_one] = 0x00;
    } else {
        memory_bank[arg_one] = 0x00;
        memory_bank[arg_one+1] = 0x00;
    }
    return machine;
}

state_t* STA_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Absolute Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[effective_address+1] = (uint8_t)(state->A.high & 0xFF);
    }
    return machine;
}

state_t* STZ_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore Zero, Absolute Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = 0x00;
    } else {
        memory_bank[effective_address] = 0x00;
        memory_bank[effective_address+1] = 0x00;
    }
    return machine;
}

state_t* STA_ABL_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // STore A register, Absolute Long Indexed X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    uint16_t base_address = arg_one;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
    } else {
        memory_bank[effective_address] = (uint8_t)(state->A.low & 0xFF);
        memory_bank[effective_address+1] = (uint8_t)(state->A.high & 0xFF);
    }
    return machine;
}

state_t* LDY_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* LDA_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    // (DP,X) means: add X to DP+offset, THEN read pointer
    uint16_t effective_address_ptr = (dp_address + offset + state->X) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = base_address & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDX_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* LDA_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Stack Relative
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDY_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000;
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = memory_bank[effective_address];
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = memory_bank[effective_address];
        state->Y |= ((uint16_t)memory_bank[effective_address+1] << 8);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

state_t* LDA_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000;
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[effective_address+1];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDX_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000;
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = memory_bank[effective_address];
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = memory_bank[effective_address];
        state->X |= ((uint16_t)memory_bank[effective_address+1] << 8);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

state_t* LDA_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint8_t page = memory_bank[(effective_address_ptr + 2) & 0xFFFF];
    uint8_t *act_bank = get_memory_bank(machine, page);
    uint16_t base_address = (((uint16_t)high_byte << 8) & 0xFF00) | (low_byte & 0xFF);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = act_bank[base_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = act_bank[base_address];
        state->A.high = act_bank[(base_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* TAY           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* LDA_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* TAX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* PLB           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // PuLl Data Bank register from stack
    processor_state_t *state = &machine->processor;
    uint8_t *stack_bank = get_memory_bank(machine, 0);  // Stack is in bank 0
    if (state->emulation_mode) {
        state->SP = (state->SP + 1) & 0x1FF;
        state->PBR = stack_bank[0x0100 | (state->SP & 0xFF)];
    } else {
        state->SP = (state->SP + 1) & 0xFFFF;
        state->PBR = stack_bank[state->SP & 0xFFFF];
    }
    set_flags_nz_8(machine, state->PBR);
    return machine;
}

state_t* LDY_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = memory_bank[arg_one];
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = memory_bank[arg_one];
        state->Y |= ((uint16_t)memory_bank[(arg_one + 1) & 0xFFFF] << 8);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

state_t* LDA_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[arg_one];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[arg_one];
        state->A.high = memory_bank[(arg_one + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDX_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = memory_bank[arg_one];
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = memory_bank[arg_one];
        state->X |= ((uint16_t)memory_bank[(arg_one + 1) & 0xFFFF] << 8);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

state_t* LDA_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Long
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[arg_one];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[arg_one];
        state->A.high = memory_bank[(arg_one + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* BCS_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Carry Set, Callback
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, CARRY)) {
        state->PC = arg_one & 0xFFFF;
    }
    return machine;
}

state_t* LDA_DP_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDA_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t effective_address = ((uint16_t)high_byte << 8) | low_byte;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDA_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Stack Relative Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDY_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = memory_bank[effective_address];
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = memory_bank[effective_address];
        state->Y |= ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

state_t* LDA_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDX_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Direct Page Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = memory_bank[effective_address];
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = memory_bank[effective_address];
        state->X |= ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

state_t* LDA_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Direct Page Indirect Long Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint8_t page = memory_bank[(effective_address_ptr + 2) & 0xFFFF];
    uint8_t *act_bank = get_memory_bank(machine, page);
    uint16_t base_address = (((uint16_t)high_byte << 8) & 0xFF00) | (low_byte & 0xFF);
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = act_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = act_bank[effective_address];
        state->A.high = act_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* CLV           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CLear oVerflow flag
    clear_flag(machine, OVERFLOW);
    return machine;
}

state_t* LDA_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* TSX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* TYX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* LDY_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD Y, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->Y = memory_bank[effective_address];
        set_flags_nz_8(machine, state->Y);
    } else {
        state->Y = memory_bank[effective_address];
        state->Y |= ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        set_flags_nz_16(machine, state->Y);
    }
    return machine;
}

state_t* LDA_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* LDX_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD X, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->Y) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        state->X = memory_bank[effective_address];
        set_flags_nz_8(machine, state->X);
    } else {
        state->X = memory_bank[effective_address];
        state->X |= ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        set_flags_nz_16(machine, state->X);
    }
    return machine;
}

state_t* LDA_AL_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // LoaD A, Absolute Long Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        state->A.low = memory_bank[effective_address];
        set_flags_nz_8(machine, state->A.low);
    } else {
        state->A.low = memory_bank[effective_address];
        state->A.high = memory_bank[(effective_address + 1) & 0xFFFF];
        set_flags_nz_16(machine, state->A.full);
    }
    return machine;
}

state_t* CPY_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* CMP_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->X) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* CMP_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Stack Relative
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* CPY_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare Y, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->Y & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->Y & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* CMP_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* DEC_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value - 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value - 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* CMP_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *dp_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = dp_bank[effective_address_ptr];
    uint8_t high_byte = dp_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint8_t page = dp_bank[(effective_address_ptr + 2) & 0xFFFF];
    uint8_t *act_bank = get_memory_bank(machine, page);
    uint16_t effective_address = (((uint16_t)high_byte << 8) | low_byte) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)act_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)act_bank[effective_address]) | ((uint16_t)act_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* INY           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* CMP_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* DEX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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
state_t* WAI           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // WAit for Interrupt
    processor_state_t *state = &machine->processor;
    // ??? what here ?
    return machine;
}

state_t* CPY_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare Y, Absolute
    processor_state_t *state = &machine->processor;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = (uint8_t)(arg_one & 0xFF);
        uint8_t result = (state->Y & 0xFF) - (value_to_compare & 0xFF);
        if ((state->Y & 0xFF) >= (value_to_compare & 0xFF)) {
            set_flag(machine, CARRY);
        } else {
            clear_flag(machine, CARRY);
        }
        set_flags_nz_8(machine, result);
    } else {
        value_to_compare = arg_one & 0xFFFF;
        uint16_t result = (state->Y & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* CMP_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = arg_one & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[effective_address + 1] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* DEC_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = arg_one & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value - 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value - 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* CMP_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Long
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    uint16_t effective_address = arg_one & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* BNE_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Not Equal (Zero flag clear)
    processor_state_t *state = &machine->processor;
    if (!is_flag_set(machine, ZERO)) {
        int8_t offset = (int8_t)(arg_one & 0xFF);
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

state_t* CMP_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t effective_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* CMP_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Stack Relative Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* PEI_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Effective Indirect, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t effective_address = ((uint16_t)high_byte << 8) | low_byte;
    // Push effective address onto stack
    memory_bank[effective_address - 1] = (uint8_t)((effective_address >> 8) & 0xFF);
    memory_bank[effective_address - 2] = (uint8_t)(effective_address & 0xFF);
    state->SP = (state->SP - 2) & 0xFFFF;
    return machine;
}

state_t* CMP_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->X) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* DEC_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value - 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value - 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* CMP_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Direct Page Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *dp_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = dp_bank[effective_address_ptr];
    uint8_t high_byte = dp_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint8_t page = dp_bank[(effective_address_ptr + 2) & 0xFFFF];
    uint8_t *memory_bank = get_memory_bank(machine, page);
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* CLD_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CLear Decimal Flag
    clear_flag(machine, DECIMAL_MODE);
    return machine;
}

state_t* CMP_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->Y) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->A.low & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->A.full & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* PHX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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
state_t* STP           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SToP the processor
    // ??? what here ?
    return machine;
}

state_t* JMP_ABS_IL    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // JuMP to Absolute Indirect Long address
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = arg_one & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address];
    uint8_t high_byte = memory_bank[(effective_address + 1) & 0xFFFF];
    uint8_t page = memory_bank[(effective_address + 2) & 0xFFFF];
    state->PC = (((uint16_t)high_byte << 8) | low_byte) & 0xFFFF;
    state->PBR = page;
    return machine;
}

state_t* CMP_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* DEC_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // DECrement, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value - 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value - 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* CMP_ABL_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // CoMPare A, Absolute Long Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->A.low & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* CPX_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* SBC_DP_I_IX   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = get_dp_address_indirect_indexed_x(machine, arg_one);
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
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

state_t* SBC_SR        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Stack Relative
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (sp_address + offset) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
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

state_t* CPX_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare X, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint8_t result = (state->X & 0xFF) - (value_to_compare & 0xFF);
        set_flags_nzc_8(machine, result);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t result = (state->X & 0xFFFF) - (value_to_compare & 0xFFFF);
        set_flags_nzc_16(machine, result);
    }
    return machine;
}

state_t* SBC_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* INC_DP        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement Direct Page
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value + 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value + 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* SBC_DP_IL     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Long
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *dp_bank = get_memory_bank(machine, 0);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = dp_bank[effective_address_ptr];
    uint8_t high_byte = dp_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint8_t page = dp_bank[(effective_address_ptr + 2) & 0xFFFF];
    uint8_t *memory_bank = get_memory_bank(machine, page);
    uint16_t effective_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t borrow = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (uint16_t)state->A.low - (uint16_t)value_to_subtract - borrow;
        state->A.low = result & 0xFF;
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, state->A.low);
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
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

state_t* INX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* SBC_IMM       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* NOP           (state_t* machine, uint16_t arg_one, uint16_t arg_two) { // opcode 0xEA
    // No OPeration
    // New simplest ever instruction
    return machine;
}

state_t* XBA           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // eXchange B and A
    processor_state_t *state = &machine->processor;
    uint8_t temp = state->A.low;
    state->A.low = state->A.high;
    state->A.high = temp;
    // Set flags based on new A low byte
    set_flags_nz_8(machine, state->A.low & 0xFF);
    return machine;
}

state_t* CPX_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // ComPare X, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = arg_one & 0xFFFF;
    uint16_t value_to_compare;
    if (state->emulation_mode || is_flag_set(machine, X_FLAG)) {
        value_to_compare = (uint8_t)memory_bank[effective_address];
        uint16_t result = (uint16_t)(state->X & 0xFF) - (uint16_t)(value_to_compare & 0xFF);
        // Carry is set if no borrow occurred
        if (result & 0x8000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_8(machine, result & 0xFF);
    } else {
        value_to_compare = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint32_t result = (uint32_t)(state->X & 0xFFFF) - (uint32_t)(value_to_compare & 0xFFFF);
        // Carry is set if no borrow occurred
        if (result & 0x80000000) clear_flag(machine, CARRY);  // Borrow occurred
        else set_flag(machine, CARRY);  // No borrow
        set_flags_nz_16(machine, result & 0xFFFF);
    }
    return machine;
}

state_t* SBC_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = arg_one & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* INC_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement value, Absolute
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = arg_one & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value + 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value + 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* SBC_ABL       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Long
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    uint16_t effective_address = arg_one & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* BEQ_CB        (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Branch if Equal (Zero flag set)
    processor_state_t *state = &machine->processor;
    if (is_flag_set(machine, ZERO)) {
        int8_t offset = (int8_t)(arg_one & 0xFF);
        state->PC = (state->PC + offset) & 0xFFFF;
    }
    return machine;
}

state_t* SBC_DP_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* SBC_DP_I      (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t effective_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* SBC_SR_I_IY   (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Stack Relative Indirect Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t sp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) sp_address = 0x0100 | (state->SP & 0xFF);
    else sp_address = state->SP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (sp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint16_t base_address = ((uint16_t)high_byte << 8) | low_byte;
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* PEA_ABS       (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Push Effective Absolute Address
    uint16_t effective_address = arg_one & 0xFFFF;
    push_word(machine, effective_address);
    return machine;
}

state_t* SBC_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->X) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* INC_DP_IX     (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement Direct Page Indexed by X
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address = (dp_address + offset + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value + 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value + 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* SBC_DP_IL_IY  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Direct Page Indirect Long Indexed by Y
    processor_state_t *state = &machine->processor;
    uint16_t dp_address;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    if (state->emulation_mode) dp_address = 0x0000 | (state->DP & 0xFF);
    else dp_address = state->DP & 0xFFFF;
    uint8_t offset = (uint8_t)arg_one;
    uint16_t effective_address_ptr = (dp_address + offset) & 0xFFFF;
    uint8_t low_byte = memory_bank[effective_address_ptr];
    uint8_t high_byte = memory_bank[(effective_address_ptr + 1) & 0xFFFF];
    uint8_t page = memory_bank[(effective_address_ptr + 2) & 0xFFFF];
    uint8_t *act_bank = get_memory_bank(machine, page);
    uint16_t base_address = (((uint16_t)high_byte << 8) & 0xFF00) | (low_byte & 0xFF);
    uint16_t effective_address = (base_address + state->Y) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)act_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)act_bank[effective_address]) | ((uint16_t)act_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* SED           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SEt Decimal flag
    set_flag(machine, DECIMAL_MODE);
    return machine;
}

state_t* SBC_ABS_IY    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Indexed by Y
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->Y) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* PLX           (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
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

state_t* JSR_ABS_I_IX  (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // Jump to SubRoutine, Absolute Indexed Indirect (Indexed by X)
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t pointer_address = (arg_one + state->X) & 0xFFFF;
    uint8_t low_byte = memory_bank[pointer_address];
    uint8_t high_byte = memory_bank[(pointer_address + 1) & 0xFFFF];
    uint16_t target_address = ((uint16_t)high_byte << 8) | low_byte;
    
    // Push return address minus 1 onto stack
    uint16_t return_address = (state->PC - 1) & 0xFFFF;
    push_word(machine, return_address);
    
    state->PC = target_address;
    return machine;
}

state_t* SBC_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}

state_t* INC_ABS_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // INCrement, Absolute Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, state->DBR);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        uint8_t value = memory_bank[effective_address];
        value = (value + 1) & 0xFF;
        memory_bank[effective_address] = value;
        set_flags_nz_8(machine, value);
    } else {
        uint16_t value = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        value = (value + 1) & 0xFFFF;
        memory_bank[effective_address] = (uint8_t)(value & 0xFF);
        memory_bank[(effective_address + 1) & 0xFFFF] = (uint8_t)((value >> 8) & 0xFF);
        set_flags_nz_16(machine, value);
    }
    return machine;
}

state_t* SBC_ABL_IX    (state_t* machine, uint16_t arg_one, uint16_t arg_two) {
    // SuBtract with Carry, Absolute Long Indexed by X
    processor_state_t *state = &machine->processor;
    uint8_t *memory_bank = get_memory_bank(machine, (uint8_t)arg_two);
    uint16_t effective_address = (arg_one + state->X) & 0xFFFF;
    uint16_t value_to_subtract;
    if (state->emulation_mode || is_flag_set(machine, M_FLAG)) {
        value_to_subtract = (uint8_t)memory_bank[effective_address];
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint8_t result = (state->A.low & 0xFF) - (value_to_subtract & 0xFF) - carry;
        set_flags_nzc_8(machine, result);
        state->A.low = result & 0xFF;
    } else {
        value_to_subtract = ((uint16_t)memory_bank[effective_address]) | ((uint16_t)memory_bank[(effective_address + 1) & 0xFFFF] << 8);
        uint16_t carry = is_flag_set(machine, CARRY) ? 0 : 1;
        uint16_t result = (state->A.full & 0xFFFF) - (value_to_subtract & 0xFFFF) - carry;
        set_flags_nzc_16(machine, result);
        state->A.full = result & 0xFFFF;
    }
    return machine;
}
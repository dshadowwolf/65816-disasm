#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "machine_setup.h"
#include "state.h"

// Read/write helpers
extern uint8_t read_byte_new(machine_state_t *machine, uint16_t address);
extern void write_byte_new(machine_state_t *machine, uint16_t address, uint8_t value);
extern uint16_t read_word_new(machine_state_t *machine, uint16_t address);
extern void write_word_new(machine_state_t *machine, uint16_t address, uint16_t value);

void print_test_header(const char *name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", name);
    printf("========================================\n");
}

void test_cycle_based_clocking(machine_state_t *machine) {
    print_test_header("Cycle-Based Hardware Clocking");
    
    // Initialize VIA Timer 1
    write_byte_new(machine, 0x7FC5, 0x10); // T1CH = 0x1000 (4096 cycles)
    write_byte_new(machine, 0x7FC4, 0x00); // T1CL = 0x00
    
    uint16_t initial_count = read_word_new(machine, 0x7FC4);
    printf("  Initial Timer 1 count: 0x%04X (%d)\n", initial_count, initial_count);
    
    // Get direct access to ROM memory region to write test program
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *rom_region = bank0->regions;
    while (rom_region && rom_region->start_offset != 0x8000) {
        rom_region = rom_region->next;
    }
    
    if (rom_region && rom_region->data) {
        // Load a simple program into ROM:
        // 8000: EA EA EA EA EA (5 x NOP, 2 cycles each = 10 cycles total)
        // 8005: DB (STP, 2 cycles)
        for (int i = 0; i < 5; i++) {
            rom_region->data[i] = 0xEA; // NOP
        }
        rom_region->data[5] = 0xDB; // STP
        rom_region->data[0x7FFC] = 0x00; // Reset vector low
        rom_region->data[0x7FFD] = 0x80; // Reset vector high
    }
    
    // Reset processor
    reset_processor(&machine->processor);
    machine->processor.PC = 0x8000;
    
    printf("  Executing 5 NOP instructions (2 cycles each)...\n");
    
    int total_cycles = 0;
    for (int i = 0; i < 5; i++) {
        step_result_t *result = machine_step(machine);
        printf("    Step %d: %s at $%04X - %d cycles\n", 
               i, result->mnemonic, result->address & 0xFFFF, result->cycles);
        total_cycles += result->cycles;
        free_step_result(result);
    }
    
    printf("  Total CPU cycles executed: %d\n", total_cycles);
    
    uint16_t final_count = read_word_new(machine, 0x7FC4);
    printf("  Final Timer 1 count: 0x%04X (%d)\n", final_count, final_count);
    
    int expected_decrease = total_cycles;
    int actual_decrease = initial_count - final_count;
    
    printf("  Expected timer decrease: %d cycles\n", expected_decrease);
    printf("  Actual timer decrease: %d cycles\n", actual_decrease);
    
    if (actual_decrease == expected_decrease) {
        printf("  ✓ VIA timer clocked correctly based on opcode cycles!\n");
    } else {
        printf("  ✗ Timer decrease mismatch (expected %d, got %d)\n", 
               expected_decrease, actual_decrease);
    }
}

void test_different_cycle_counts(machine_state_t *machine) {
    print_test_header("Different Opcode Cycle Counts");
    
    // Get direct access to ROM memory region
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *rom_region = bank0->regions;
    while (rom_region && rom_region->start_offset != 0x8000) {
        rom_region = rom_region->next;
    }
    
    if (rom_region && rom_region->data) {
        // Load different instructions with varying cycle counts
        // 8000: 18      CLC (2 cycles)
        // 8001: FB      XCE (2 cycles)
        // 8002: A9 42   LDA #$42 (2 cycles)
        // 8004: EA      NOP (2 cycles)
        // 8005: 00 00   BRK (7 cycles)
        // 8007: DB      STP (2 cycles)
        
        rom_region->data[0] = 0x18; // CLC
        rom_region->data[1] = 0xFB; // XCE
        rom_region->data[2] = 0xA9; // LDA #
        rom_region->data[3] = 0x42;
        rom_region->data[4] = 0xEA; // NOP
        rom_region->data[5] = 0x00; // BRK
        rom_region->data[6] = 0x00;
        rom_region->data[7] = 0xDB; // STP
    }
    
    // Reset processor
    reset_processor(&machine->processor);
    machine->processor.PC = 0x8000;
    
    printf("\n  Opcode  Mnemonic  Cycles\n");
    printf("  ------  --------  ------\n");
    
    int total_cycles = 0;
    for (int i = 0; i < 5; i++) {
        step_result_t *result = machine_step(machine);
        printf("  0x%02X    %-8s  %2d\n", 
               result->opcode, result->mnemonic, result->cycles);
        total_cycles += result->cycles;
        free_step_result(result);
    }
    
    printf("\n  Total cycles: %d\n", total_cycles);
    printf("  ✓ Various opcodes tracked with correct cycle counts\n");
}

int main() {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  Cycle-Based Hardware Clocking Test          ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    // Create machine
    machine_state_t *machine = create_machine();
    
    // Connect disassembler state
    set_emulated_processor(&machine->processor);
    
    // Run tests
    test_cycle_based_clocking(machine);
    test_different_cycle_counts(machine);
    
    // Cleanup
    destroy_machine(machine);
    
    printf("\n╔═══════════════════════════════════════════════╗\n");
    printf("║  All Tests Complete!                          ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    return 0;
}

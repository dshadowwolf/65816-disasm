#include <stdio.h>
#include <stdlib.h>
#include "machine.h"
#include "machine_setup.h"
#include "processor.h"
#include "processor_helpers.h"

// Helper function to write to ROM region through the region system
static void write_rom_byte(machine_state_t *machine, uint16_t address, uint8_t value) {
    memory_region_t *region = find_current_memory_region(machine, address);
    if (region && region->data) {
        // Direct write to ROM data for setup (bypassing readonly check)
        region->data[address - region->start_offset] = value;
    }
}

// Test program demonstrating single-step execution with disassembly
int main() {
    printf("=== 65816 Single-Step Emulator Test ===\n\n");
    
    // Create and initialize machine
    machine_state_t *machine = create_machine();
    if (!machine) {
        fprintf(stderr, "Failed to create machine\n");
        return 1;
    }
    
    // Write a small test program to ROM (0x8000) using region-based system
    // Reset vectors point to 0x8000
    write_rom_byte(machine, 0xFFFC, 0x00);  // Reset vector low
    write_rom_byte(machine, 0xFFFD, 0x80);  // Reset vector high
    
    // Write test program at 0x8000
    write_rom_byte(machine, 0x8000, 0x18);  // CLC
    write_rom_byte(machine, 0x8001, 0xFB);  // XCE - switch to native mode
    write_rom_byte(machine, 0x8002, 0xC2);  // REP
    write_rom_byte(machine, 0x8003, 0x30);  // #$30 - clear M and X flags (16-bit mode)
    write_rom_byte(machine, 0x8004, 0xA9);  // LDA
    write_rom_byte(machine, 0x8005, 0x34);  // #$1234 (16-bit immediate)
    write_rom_byte(machine, 0x8006, 0x12);
    write_rom_byte(machine, 0x8007, 0xE2);  // SEP
    write_rom_byte(machine, 0x8008, 0x20);  // #$20 - set M flag (8-bit accumulator)
    write_rom_byte(machine, 0x8009, 0xA9);  // LDA
    write_rom_byte(machine, 0x800A, 0x42);  // #$42 (8-bit immediate)
    write_rom_byte(machine, 0x800B, 0xA2);  // LDX
    write_rom_byte(machine, 0x800C, 0xCD);  // #$ABCD (16-bit immediate, X still 16-bit)
    write_rom_byte(machine, 0x800D, 0xAB);
    write_rom_byte(machine, 0x800E, 0xEA);  // NOP
    write_rom_byte(machine, 0x800F, 0xDB);  // STP - stop processor
    
    // Reset processor to start at reset vector
    reset_processor(&machine->processor);
    
    // Manually load PC from reset vector (0xFFFC/0xFFFD in emulation mode)
    machine->processor.PC = read_byte_new(machine, 0xFFFC) | (read_byte_new(machine, 0xFFFD) << 8);
    
    printf("Initial state:\n");
    printf("  PC: $%04X  PBR: $%02X  A: $%04X  X: $%04X  Y: $%04X\n",
           machine->processor.PC, machine->processor.PBR,
           machine->processor.A.full, machine->processor.X, machine->processor.Y);
    printf("  P: $%02X  Emulation: %s\n\n",
           machine->processor.P, machine->processor.emulation_mode ? "Yes" : "No");
    
    // Single-step through instructions
    int step_count = 0;
    int max_steps = 20;
    
    printf("Stepping through program:\n");
    printf("%-4s  %-8s  %-6s  %-12s  %-s\n", 
           "Step", "Address", "OpCode", "Instruction", "State");
    printf("----  --------  ------  ------------  -----\n");
    
    while (step_count < max_steps) {
        step_result_t *result = machine_step(machine);
        if (!result) {
            fprintf(stderr, "Error during step %d\n", step_count);
            break;
        }
        
        // Display disassembly and state
        printf("%4d  %02X:%04X   %02X      %-4s %-8s  A=$%04X X=$%04X Y=$%04X P=$%02X",
               step_count,
               (result->address >> 16) & 0xFF,
               result->address & 0xFFFF,
               result->opcode,
               result->mnemonic,
               result->operand_str,
               machine->processor.A.full,
               machine->processor.X,
               machine->processor.Y,
               machine->processor.P);
        
        // Debug: show operand value and size
        if (result->instruction_size > 1) {
            printf(" [op=$%04X sz=%d]", result->operand, result->instruction_size);
        }
        printf("\n");
        
        // Check for halt or wait
        if (result->halted) {
            printf("\nProcessor halted (STP instruction)\n");
            free_step_result(result);
            break;
        }
        
        if (result->waiting) {
            printf("\nProcessor waiting (WAI instruction)\n");
            free_step_result(result);
            break;
        }
        
        free_step_result(result);
        step_count++;
    }
    
    printf("\nFinal state after %d steps:\n", step_count);
    printf("  PC: $%04X  PBR: $%02X  A: $%04X  X: $%04X  Y: $%04X\n",
           machine->processor.PC, machine->processor.PBR,
           machine->processor.A.full, machine->processor.X, machine->processor.Y);
    printf("  P: $%02X  Emulation: %s\n",
           machine->processor.P, machine->processor.emulation_mode ? "Yes" : "No");
    
    // Cleanup
    destroy_machine(machine);
    
    printf("\n=== Test Complete ===\n");
    return 0;
}

/*
 * Example: Using Emulated Processor State with Disassembler
 * 
 * This demonstrates how tbl.c now references the actual emulated
 * machine state instead of using a separate disassembler state.
 */

#include <stdio.h>
#include <stdint.h>
#include "machine_setup.h"
#include "machine.h"
#include "processor_helpers.h"
#include "state.h"

void print_flag_state(machine_state_t *machine) {
    printf("Processor State:\n");
    printf("  P register: 0x%02X\n", machine->processor.P);
    printf("  M flag (bit 5): %s (accumulator is %s)\n", 
           (machine->processor.P & M_FLAG) ? "SET" : "CLEAR",
           (machine->processor.P & M_FLAG) ? "8-bit" : "16-bit");
    printf("  X flag (bit 4): %s (index is %s)\n", 
           (machine->processor.P & X_FLAG) ? "SET" : "CLEAR",
           (machine->processor.P & X_FLAG) ? "8-bit" : "16-bit");
    printf("  Emulation mode: %s\n\n", 
           machine->processor.emulation_mode ? "YES" : "NO");
    
    // Show what the disassembler will see
    printf("Disassembler will use:\n");
    printf("  isMSet() = %s\n", isMSet() ? "true (8-bit accumulator)" : "false (16-bit accumulator)");
    printf("  isXSet() = %s\n\n", isXSet() ? "true (8-bit index)" : "false (16-bit index)");
}

int main(void) {
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  Emulated State Integration Example           ║\n");
    printf("╚════════════════════════════════════════════════╝\n\n");
    
    // Initialize machine
    machine_state_t machine;
    initialize_machine(&machine);
    
    printf("Step 1: Connect disassembler to emulated processor\n");
    printf("-----------------------------------------------\n");
    set_emulated_processor(&machine.processor);
    printf("Called set_emulated_processor(&machine.processor)\n\n");
    
    printf("Step 2: Initial state (native mode, flags clear)\n");
    printf("-----------------------------------------------\n");
    machine.processor.emulation_mode = false;
    machine.processor.P = 0x00;  // All flags clear
    print_flag_state(&machine);
    
    printf("Step 3: Execute REP #$30 (clear M and X flags - 16-bit mode)\n");
    printf("-----------------------------------------------\n");
    // REP clears bits, so clear M_FLAG and X_FLAG
    machine.processor.P &= ~(M_FLAG | X_FLAG);
    print_flag_state(&machine);
    
    printf("Step 4: Execute SEP #$20 (set M flag - 8-bit accumulator)\n");
    printf("-----------------------------------------------\n");
    machine.processor.P |= M_FLAG;
    print_flag_state(&machine);
    
    printf("Step 5: Execute SEP #$10 (set X flag - 8-bit index)\n");
    printf("-----------------------------------------------\n");
    machine.processor.P |= X_FLAG;
    print_flag_state(&machine);
    
    printf("Step 6: Execute REP #$20 (clear M flag - 16-bit accumulator)\n");
    printf("-----------------------------------------------\n");
    machine.processor.P &= ~M_FLAG;
    print_flag_state(&machine);
    
    printf("Step 7: Switch to emulation mode\n");
    printf("-----------------------------------------------\n");
    machine.processor.emulation_mode = true;
    printf("Note: In emulation mode, M and X flags are ignored.\n");
    printf("Accumulator is always 16-bit, index registers are always 8-bit.\n\n");
    print_flag_state(&machine);
    
    printf("Step 8: Disconnect emulated processor (back to legacy mode)\n");
    printf("-----------------------------------------------\n");
    set_emulated_processor(NULL);
    printf("Called set_emulated_processor(NULL)\n");
    printf("Now isMSet() and isXSet() use legacy internal state.\n\n");
    
    // Cleanup
    cleanup_machine_with_via(&machine);
    
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  Example Complete                              ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\nSummary:\n");
    printf("- tbl.c functions (m_set, x_set) now check actual processor state\n");
    printf("- Disassembler operand sizes match emulated processor mode\n");
    printf("- No need to manually sync separate disassembler state\n");
    printf("- Legacy mode still available for backward compatibility\n");
    
    return 0;
}

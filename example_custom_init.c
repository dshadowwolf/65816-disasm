#include <stdio.h>
#include "machine_setup.h"
#include "processor.h"

int main() {
    // Create initial state with custom register values
    initial_state_t init = {
        .A = 0x1234,                // Set accumulator to $1234
        .X = 0x5678,                // Set X register to $5678
        .Y = 0x9ABC,                // Set Y register to $9ABC
        .PC = 0x8000,               // Start at address $8000
        .SP = 0x01FF,               // Stack pointer at top of stack
        .DP = 0x0000,               // Direct page at $0000
        .P = 0x30,                  // Processor status (M=1, X=1: 8-bit mode)
        .PBR = 0x01,                // Program bank register = $01
        .DBR = 0x02,                // Data bank register = $02
        .emulation_mode = false,    // Native mode
        .interrupts_disabled = true // Interrupts disabled
    };
    
    // Create machine with custom initial state
    machine_state_t *machine = create_machine_with_state(&init);
    
    printf("Machine created with custom initial state:\n");
    printf("  A  = $%04X\n", machine->processor.A);
    printf("  X  = $%04X\n", machine->processor.X);
    printf("  Y  = $%04X\n", machine->processor.Y);
    printf("  PC = $%04X\n", machine->processor.PC);
    printf("  SP = $%04X\n", machine->processor.SP);
    printf("  DP = $%04X\n", machine->processor.DP);
    printf("  P  = $%02X\n", machine->processor.P);
    printf("  PBR = $%02X\n", machine->processor.PBR);
    printf("  DBR = $%02X\n", machine->processor.DBR);
    printf("  Full address = $%06X (PBR:PC)\n", 
           (machine->processor.PBR << 16) | machine->processor.PC);
    printf("  Emulation mode: %s\n", 
           machine->processor.emulation_mode ? "yes" : "no");
    printf("  Interrupts: %s\n", 
           machine->processor.interrupts_disabled ? "disabled" : "enabled");
    
    // For comparison, create a machine with default state
    machine_state_t *default_machine = create_machine_with_state(NULL);
    printf("\nMachine created with default state:\n");
    printf("  A  = $%04X\n", default_machine->processor.A);
    printf("  X  = $%04X\n", default_machine->processor.X);
    printf("  Y  = $%04X\n", default_machine->processor.Y);
    printf("  PC = $%04X\n", default_machine->processor.PC);
    printf("  SP = $%04X\n", default_machine->processor.SP);
    printf("  Full address = $%06X (PBR:PC)\n", 
           (default_machine->processor.PBR << 16) | default_machine->processor.PC);
    
    destroy_machine(machine);
    destroy_machine(default_machine);
    
    return 0;
}

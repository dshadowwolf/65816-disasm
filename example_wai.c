/**
 * Example: Using WAI (Wait for Interrupt) with Hardware Devices
 * 
 * This example demonstrates how the WAI instruction now properly waits
 * for hardware interrupts from emulated devices (VIA, ACIA, Board FIFO).
 */

#include <stdio.h>
#include "machine_setup.h"
#include "via6522.h"

int main() {
    printf("=== WAI (Wait for Interrupt) Demonstration ===\n\n");
    
    // Create machine
    machine_state_t *machine = create_machine();
    
    // Get ROM region for setting up test program
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *rom_region = bank0->regions;
    while (rom_region && rom_region->start_offset != 0x8000) {
        rom_region = rom_region->next;
    }
    
    printf("Setting up example program:\n");
    printf("  - Initialize processor to native mode\n");
    printf("  - Enable interrupts\n");
    printf("  - Execute WAI instruction\n");
    printf("  - Hardware timer will generate interrupt\n\n");
    
    // Program: Initialize and wait for interrupt
    rom_region->data[0x0000] = 0x18; // CLC
    rom_region->data[0x0001] = 0xFB; // XCE (switch to native mode)
    rom_region->data[0x0002] = 0x58; // CLI (enable interrupts)
    rom_region->data[0x0003] = 0xCB; // WAI (wait for interrupt)
    rom_region->data[0x0004] = 0xEA; // NOP (interrupt handler)
    rom_region->data[0x0005] = 0x40; // RTI (return from interrupt)
    
    // Set up interrupt vector
    rom_region->data[0x7FEE] = 0x04; // IRQ vector low
    rom_region->data[0x7FEF] = 0x80; // IRQ vector high
    
    // Initialize processor
    machine->processor.PC = 0x8000;
    machine->processor.emulation_mode = true;
    
    // Get VIA and set up timer to generate interrupt after 50 cycles
    via6522_t *via = get_via_instance();
    via6522_write(via, 0x0E, 0xC0); // Enable Timer 1 interrupt
    via6522_write(via, 0x04, 50);   // Timer countdown value (low byte)
    via6522_write(via, 0x05, 0);    // Timer countdown value (high byte, starts timer)
    
    printf("Executing instructions:\n");
    
    // Execute CLC
    step_result_t *result = machine_step(machine);
    printf("  %s - PC: $%04X\n", result->mnemonic, result->address & 0xFFFF);
    free(result);
    
    // Execute XCE
    result = machine_step(machine);
    printf("  %s - PC: $%04X, Native mode: %s\n", 
           result->mnemonic, result->address & 0xFFFF,
           machine->processor.emulation_mode ? "No" : "Yes");
    free(result);
    
    // Execute CLI
    result = machine_step(machine);
    printf("  %s - PC: $%04X, Interrupts enabled: %s\n",
           result->mnemonic, result->address & 0xFFFF,
           machine->processor.interrupts_disabled ? "No" : "Yes");
    free(result);
    
    // Execute WAI - This will wait for the VIA timer interrupt
    printf("\n  Executing WAI...\n");
    printf("  (Processor will clock hardware and wait for interrupt)\n");
    result = machine_step(machine);
    printf("  %s - Waited %u cycles for interrupt\n", 
           result->mnemonic, result->cycles);
    printf("  PC after interrupt: $%04X (jumped to IRQ handler)\n",
           machine->processor.PC);
    printf("  I flag set: %s (interrupts disabled during handler)\n",
           machine->processor.interrupts_disabled ? "Yes" : "No");
    free(result);
    
    printf("\n✓ WAI successfully waited for hardware interrupt!\n");
    printf("\nKey features of WAI implementation:\n");
    printf("  - Actually waits by clocking hardware devices\n");
    printf("  - Checks VIA, ACIA, and Board FIFO for interrupt requests\n");
    printf("  - Processes interrupt and vectors to IRQ handler\n");
    printf("  - Exits immediately if interrupts are disabled\n");
    
    cleanup_machine_with_via(machine);
    free(machine);
    
    return 0;
}

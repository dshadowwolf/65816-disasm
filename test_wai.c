#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "machine_setup.h"
#include "machine.h"
#include "via6522.h"
#include "acia6551.h"

// Test WAI (Wait for Interrupt) instruction
void test_wai_with_via_timer() {
    printf("Test: WAI with VIA Timer 1 interrupt\n");
    
    machine_state_t *machine = create_machine();
    assert(machine != NULL);
    
    // Get ROM region for direct access (bypass read-only)
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *rom_region = bank0->regions;
    while (rom_region && rom_region->start_offset != 0x8000) {
        rom_region = rom_region->next;
    }
    assert(rom_region != NULL);
    
    // Set up test program at 0x8000:
    // 0x8000: CLC (clear carry)
    // 0x8001: XCE (switch to native mode)
    // 0x8002: SEI (disable interrupts first, we'll enable after setup)
    // 0x8003: CLI (enable interrupts)
    // 0x8004: WAI (wait for interrupt)
    // 0x8005: NOP (should execute after interrupt)
    // 0x8006: BRK (stop)
    
    rom_region->data[0x0000] = 0x18; // CLC
    rom_region->data[0x0001] = 0xFB; // XCE
    rom_region->data[0x0002] = 0x78; // SEI
    rom_region->data[0x0003] = 0x58; // CLI
    rom_region->data[0x0004] = 0xCB; // WAI
    rom_region->data[0x0005] = 0xEA; // NOP
    rom_region->data[0x0006] = 0x00; // BRK
    
    // Set up interrupt vector at 0xFFEE (native mode IRQ)
    rom_region->data[0x7FEE] = 0x05; // Low byte of 0x8005
    rom_region->data[0x7FEF] = 0x80; // High byte of 0x8005
    
    // Initialize processor
    machine->processor.PC = 0x8000;
    machine->processor.PBR = 0x00;
    machine->processor.emulation_mode = true; // Start in emulation, will switch to native
    
    // Get VIA instance and configure Timer 1 to generate interrupt
    via6522_t *via = get_via_instance();
    
    // Enable Timer 1 interrupt in VIA
    via6522_write(via, 0x0E, 0x80 | 0x40); // IER: Set bit 7 (enable), bit 6 (Timer 1)
    
    // Set Timer 1 to count down from 100
    via6522_write(via, 0x04, 100); // T1C-L (low byte of counter)
    via6522_write(via, 0x05, 0);   // T1C-H (high byte, starts timer)
    
    printf("  Executing CLC...\n");
    step_result_t *result = machine_step(machine);
    assert(result->opcode == 0x18); // CLC
    free(result);
    
    printf("  Executing XCE (switch to native mode)...\n");
    result = machine_step(machine);
    assert(result->opcode == 0xFB); // XCE
    assert(!machine->processor.emulation_mode); // Should be in native mode now
    free(result);
    
    printf("  Executing SEI (disable interrupts)...\n");
    result = machine_step(machine);
    assert(result->opcode == 0x78); // SEI
    assert(machine->processor.interrupts_disabled);
    free(result);
    
    printf("  Executing CLI (enable interrupts)...\n");
    result = machine_step(machine);
    assert(result->opcode == 0x58); // CLI
    assert(!machine->processor.interrupts_disabled);
    free(result);
    
    printf("  Executing WAI (should wait for Timer 1 interrupt)...\n");
    uint16_t pc_before_wai = machine->processor.PC;
    result = machine_step(machine);
    assert(result->opcode == 0xCB); // WAI
    assert(result->waiting);
    
    // After WAI, PC should have jumped to interrupt vector
    printf("  PC before WAI: 0x%04X, PC after interrupt: 0x%04X\n", 
           pc_before_wai, machine->processor.PC);
    printf("  Cycles spent waiting: %u\n", result->cycles);
    
    // Verify interrupt was processed:
    // - PC should be at interrupt handler (0x8005)
    // - Interrupt disable flag should be set
    assert(machine->processor.PC == 0x8005);
    assert(machine->processor.interrupts_disabled);
    
    printf("  ✓ WAI correctly waited for interrupt and jumped to handler\n");
    free(result);
    
    cleanup_machine_with_via(machine);
    free(machine);
    printf("  PASS\n\n");
}

void test_wai_with_acia_interrupt() {
    printf("Test: WAI with ACIA receive interrupt\n");
    
    machine_state_t *machine = create_machine();
    assert(machine != NULL);
    
    // Get ROM region
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *rom_region = bank0->regions;
    while (rom_region && rom_region->start_offset != 0x8000) {
        rom_region = rom_region->next;
    }
    assert(rom_region != NULL);
    
    // Set up test program at 0x8000:
    rom_region->data[0x0000] = 0x18; // CLC
    rom_region->data[0x0001] = 0xFB; // XCE (native mode)
    rom_region->data[0x0002] = 0x58; // CLI (enable interrupts)
    rom_region->data[0x0003] = 0xCB; // WAI
    rom_region->data[0x0004] = 0xEA; // NOP
    
    // Set up interrupt vector at 0xFFEE (native mode IRQ)
    rom_region->data[0x7FEE] = 0x04; // Low byte of 0x8004
    rom_region->data[0x7FEF] = 0x80; // High byte of 0x8004
    
    // Initialize processor
    machine->processor.PC = 0x8000;
    machine->processor.PBR = 0x00;
    machine->processor.emulation_mode = true;
    
    // Get ACIA instance and configure for receive interrupt
    acia6551_t *acia = get_acia_instance();
    
    // Configure ACIA for receive interrupts
    // Command register: enable RX IRQ (bit 1)
    acia6551_write(acia, 0x02, 0x02); // CMD register, IRQ on RX
    
    // Simulate receiving a byte (this should trigger interrupt)
    acia6551_receive_byte(acia, 0x55);
    
    printf("  Executing CLC, XCE, CLI...\n");
    step_result_t *result = machine_step(machine);
    free(result);
    result = machine_step(machine);
    free(result);
    result = machine_step(machine);
    free(result);
    
    printf("  Executing WAI (should wait for ACIA RX interrupt)...\n");
    result = machine_step(machine);
    assert(result->opcode == 0xCB); // WAI
    assert(result->waiting);
    
    // Check that interrupt was triggered
    assert(acia6551_get_irq(acia));
    
    // After WAI, PC should have jumped to interrupt vector
    printf("  PC after interrupt: 0x%04X\n", machine->processor.PC);
    printf("  Cycles spent waiting: %u\n", result->cycles);
    
    assert(machine->processor.PC == 0x8004);
    assert(machine->processor.interrupts_disabled);
    
    printf("  ✓ WAI correctly waited for ACIA interrupt\n");
    free(result);
    
    cleanup_machine_with_via(machine);
    free(machine);
    printf("  PASS\n\n");
}

void test_wai_with_interrupts_disabled() {
    printf("Test: WAI with interrupts disabled (should not wait)\n");
    
    machine_state_t *machine = create_machine();
    assert(machine != NULL);
    
    // Get ROM region
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *rom_region = bank0->regions;
    while (rom_region && rom_region->start_offset != 0x8000) {
        rom_region = rom_region->next;
    }
    assert(rom_region != NULL);
    
    // Set up test program
    rom_region->data[0x0000] = 0x78; // SEI (disable interrupts)
    rom_region->data[0x0001] = 0xCB; // WAI (should exit immediately)
    rom_region->data[0x0002] = 0xEA; // NOP
    
    machine->processor.PC = 0x8000;
    machine->processor.PBR = 0x00;
    
    printf("  Executing SEI...\n");
    step_result_t *result = machine_step(machine);
    free(result);
    
    printf("  Executing WAI with interrupts disabled...\n");
    result = machine_step(machine);
    assert(result->opcode == 0xCB); // WAI
    assert(result->waiting);
    
    // WAI should exit immediately when interrupts are disabled
    // PC should just continue to next instruction
    printf("  PC after WAI: 0x%04X (should be 0x8002)\n", machine->processor.PC);
    printf("  Cycles: %u (should be just base 2 cycles)\n", result->cycles);
    
    assert(machine->processor.PC == 0x8002);
    assert(result->cycles == 2); // Just base WAI cycles, no waiting
    
    printf("  ✓ WAI correctly exited immediately with interrupts disabled\n");
    free(result);
    
    cleanup_machine_with_via(machine);
    free(machine);
    printf("  PASS\n\n");
}

int main() {
    printf("=== WAI (Wait for Interrupt) Tests ===\n\n");
    
    test_wai_with_via_timer();
    test_wai_with_acia_interrupt();
    test_wai_with_interrupts_disabled();
    
    printf("=== All WAI tests passed ===\n");
    return 0;
}

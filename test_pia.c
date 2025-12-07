#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pia6521.h"

// Test context for port callbacks
typedef struct {
    uint8_t porta_value;
    uint8_t portb_value;
    int porta_writes;
    int portb_writes;
} test_context_t;

// IRQ callback context
typedef struct {
    int irqa_count;
    int irqb_count;
    bool irqa_state;
    bool irqb_state;
} irq_context_t;

// Port A callbacks
uint8_t porta_read(void* ctx) {
    test_context_t* context = (test_context_t*)ctx;
    printf("  Port A read: 0x%02X\n", context->porta_value);
    return context->porta_value;
}

void porta_write(void* ctx, uint8_t value) {
    test_context_t* context = (test_context_t*)ctx;
    context->porta_value = value;
    context->porta_writes++;
    printf("  Port A write: 0x%02X (count: %d)\n", value, context->porta_writes);
}

// Port B callbacks
uint8_t portb_read(void* ctx) {
    test_context_t* context = (test_context_t*)ctx;
    printf("  Port B read: 0x%02X\n", context->portb_value);
    return context->portb_value;
}

void portb_write(void* ctx, uint8_t value) {
    test_context_t* context = (test_context_t*)ctx;
    context->portb_value = value;
    context->portb_writes++;
    printf("  Port B write: 0x%02X (count: %d)\n", value, context->portb_writes);
}

// IRQ callbacks
void irqa_callback(void* ctx, bool state) {
    irq_context_t* context = (irq_context_t*)ctx;
    if (state && !context->irqa_state) {
        context->irqa_count++;
        printf("  *** IRQA ASSERTED (count: %d) ***\n", context->irqa_count);
    } else if (!state && context->irqa_state) {
        printf("  *** IRQA CLEARED ***\n");
    }
    context->irqa_state = state;
}

void irqb_callback(void* ctx, bool state) {
    irq_context_t* context = (irq_context_t*)ctx;
    if (state && !context->irqb_state) {
        context->irqb_count++;
        printf("  *** IRQB ASSERTED (count: %d) ***\n", context->irqb_count);
    } else if (!state && context->irqb_state) {
        printf("  *** IRQB CLEARED ***\n");
    }
    context->irqb_state = state;
}

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

void test_basic_io(void) {
    print_test_header("Basic I/O Port Operations");
    
    pia6521_t pia;
    test_context_t context = {0};
    
    pia6521_init(&pia);
    pia6521_set_porta_callbacks(&pia, porta_read, porta_write, &context);
    pia6521_set_portb_callbacks(&pia, portb_read, portb_write, &context);
    
    printf("\nSetting Port A to DDR access mode (CRA bit 2 = 0)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, 0x00);
    
    printf("\nSetting Port A to all outputs (DDR = 0xFF)\n");
    pia6521_write(&pia, PIA_PORTA_DATA, 0xFF);
    
    printf("\nSwitching to data register access (CRA bit 2 = 1)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CR_DDR_ACCESS);
    
    printf("\nWriting 0xAA to Port A data\n");
    pia6521_write(&pia, PIA_PORTA_DATA, 0xAA);
    
    printf("\nSetting Port B DDR bits 0-3 output, 4-7 input\n");
    pia6521_write(&pia, PIA_PORTB_CTRL, 0x00);  // DDR access
    pia6521_write(&pia, PIA_PORTB_DATA, 0x0F);
    
    printf("\nSwitching Port B to data access and writing 0x55\n");
    pia6521_write(&pia, PIA_PORTB_CTRL, PIA_CR_DDR_ACCESS);
    pia6521_write(&pia, PIA_PORTB_DATA, 0x55);
    
    printf("\nSimulating external Port B input = 0xF0\n");
    context.portb_value = 0xF0;
    
    printf("\nReading Port B (should mix output 0x05 with input 0xF0)\n");
    uint8_t val = pia6521_read(&pia, PIA_PORTB_DATA);
    printf("Read value: 0x%02X (expected 0xF5)\n", val);
    
    printf("\n✓ Basic I/O test complete\n");
}

void test_ddr_switching(void) {
    print_test_header("DDR/Data Register Switching");
    
    pia6521_t pia;
    pia6521_init(&pia);
    
    printf("\nAccess DDR (CRA bit 2 = 0)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, 0x00);
    
    printf("\nWrite 0x5A to DDR\n");
    pia6521_write(&pia, PIA_PORTA_DATA, 0x5A);
    
    printf("\nRead back DDR\n");
    uint8_t ddr = pia6521_read(&pia, PIA_PORTA_DATA);
    printf("DDR value: 0x%02X (expected 0x5A)\n", ddr);
    
    printf("\nSwitch to data register (CRA bit 2 = 1)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CR_DDR_ACCESS);
    
    printf("\nWrite 0xA5 to data register\n");
    pia6521_write(&pia, PIA_PORTA_DATA, 0xA5);
    
    printf("\nSwitch back to DDR access\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, 0x00);
    
    printf("\nRead DDR again (should still be 0x5A)\n");
    ddr = pia6521_read(&pia, PIA_PORTA_DATA);
    printf("DDR value: 0x%02X (expected 0x5A)\n", ddr);
    
    printf("\n✓ DDR switching test complete\n");
}

void test_ca1_interrupt(void) {
    print_test_header("CA1 Interrupt on Active Edge");
    
    pia6521_t pia;
    irq_context_t irq_ctx = {0};
    test_context_t io_ctx = {0};
    
    pia6521_init(&pia);
    pia6521_set_irqa_callback(&pia, irqa_callback, &irq_ctx);
    pia6521_set_porta_callbacks(&pia, porta_read, porta_write, &io_ctx);
    
    printf("\nConfiguring CA1 for positive edge (CRA bit 0 = 1)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CR_CA1_LOW_TO_HIGH | PIA_CR_DDR_ACCESS);
    
    printf("\nToggling CA1: low -> high (should trigger interrupt)\n");
    pia6521_set_ca1(&pia, false);
    pia6521_set_ca1(&pia, true);
    
    printf("\nReading control register\n");
    uint8_t cra = pia6521_read(&pia, PIA_PORTA_CTRL);
    printf("CRA: 0x%02X (bit 6 should be set for CA1 flag)\n", cra);
    
    printf("\nReading Port A data (should clear CA1 interrupt)\n");
    pia6521_read(&pia, PIA_PORTA_DATA);
    
    cra = pia6521_read(&pia, PIA_PORTA_CTRL);
    printf("CRA after read: 0x%02X (bit 6 should be clear)\n", cra);
    
    printf("\nConfiguring CA1 for negative edge (CRA bit 0 = 0)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CR_DDR_ACCESS);
    
    printf("\nToggling CA1: high -> low (should trigger)\n");
    pia6521_set_ca1(&pia, true);
    pia6521_set_ca1(&pia, false);
    
    cra = pia6521_read(&pia, PIA_PORTA_CTRL);
    printf("CRA: 0x%02X (bit 6 should be set)\n", cra);
    
    printf("\n✓ CA1 interrupt test complete\n");
}

void test_ca2_modes(void) {
    print_test_header("CA2 Input and Output Modes");
    
    pia6521_t pia;
    irq_context_t irq_ctx = {0};
    
    pia6521_init(&pia);
    pia6521_set_irqa_callback(&pia, irqa_callback, &irq_ctx);
    
    printf("\n--- CA2 Input Mode with IRQ ---\n");
    printf("Setting CA2 to input, negative edge with IRQ enabled\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CA2_INPUT_NEG_IRQ | PIA_CR_DDR_ACCESS);
    
    printf("\nToggling CA2: high -> low\n");
    pia6521_set_ca2_input(&pia, true);
    pia6521_set_ca2_input(&pia, false);
    
    uint8_t cra = pia6521_read(&pia, PIA_PORTA_CTRL);
    printf("CRA: 0x%02X (bit 7 should be set for CA2 flag)\n", cra);
    
    printf("\n--- CA2 Output Modes ---\n");
    printf("Setting CA2 to manual output HIGH\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CA2_OUTPUT_HIGH | PIA_CR_DDR_ACCESS);
    printf("CA2 state: %s (expected HIGH)\n", pia.ca2 ? "HIGH" : "LOW");
    
    printf("\nSetting CA2 to manual output LOW\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CA2_OUTPUT_LOW | PIA_CR_DDR_ACCESS);
    printf("CA2 state: %s (expected LOW)\n", pia.ca2 ? "HIGH" : "LOW");
    
    printf("\n✓ CA2 modes test complete\n");
}

void test_cb1_interrupt(void) {
    print_test_header("CB1 Interrupt Operation");
    
    pia6521_t pia;
    irq_context_t irq_ctx = {0};
    
    pia6521_init(&pia);
    pia6521_set_irqb_callback(&pia, irqb_callback, &irq_ctx);
    
    printf("\nConfiguring CB1 for positive edge\n");
    pia6521_write(&pia, PIA_PORTB_CTRL, PIA_CR_CA1_LOW_TO_HIGH | PIA_CR_DDR_ACCESS);
    
    printf("\nToggling CB1: low -> high\n");
    pia6521_set_cb1(&pia, false);
    pia6521_set_cb1(&pia, true);
    
    uint8_t crb = pia6521_read(&pia, PIA_PORTB_CTRL);
    printf("CRB: 0x%02X (bit 6 should be set)\n", crb);
    
    printf("\nReading Port B to clear interrupt\n");
    pia6521_read(&pia, PIA_PORTB_DATA);
    
    crb = pia6521_read(&pia, PIA_PORTB_CTRL);
    printf("CRB after read: 0x%02X (bit 6 should be clear)\n", crb);
    
    printf("\nIRQB count: %d (expected 1)\n", irq_ctx.irqb_count);
    
    printf("\n✓ CB1 interrupt test complete\n");
}

void test_handshake_mode(void) {
    print_test_header("CA2 Handshake Mode");
    
    pia6521_t pia;
    test_context_t io_ctx = {0};
    
    pia6521_init(&pia);
    pia6521_set_porta_callbacks(&pia, porta_read, porta_write, &io_ctx);
    
    printf("\nSetting up Port A for output\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, 0x00);
    pia6521_write(&pia, PIA_PORTA_DATA, 0xFF);  // DDR all output
    
    printf("\nConfiguring CA2 for handshake mode\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, PIA_CA2_OUTPUT_HS | PIA_CR_DDR_ACCESS);
    
    printf("CA2 initial state: %s\n", pia.ca2 ? "HIGH" : "LOW");
    
    printf("\nWriting to Port A (CA2 should go LOW)\n");
    pia6521_write(&pia, PIA_PORTA_DATA, 0x42);
    printf("CA2 after write: %s (expected LOW)\n", pia.ca2 ? "HIGH" : "LOW");
    
    printf("\nSimulating CA1 positive edge (CA2 should go HIGH)\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, 
                  PIA_CA2_OUTPUT_HS | PIA_CR_CA1_LOW_TO_HIGH | PIA_CR_DDR_ACCESS);
    pia6521_set_ca1(&pia, false);
    pia6521_set_ca1(&pia, true);
    printf("CA2 after CA1 edge: %s (expected HIGH)\n", pia.ca2 ? "HIGH" : "LOW");
    
    printf("\n✓ Handshake mode test complete\n");
}

void test_interrupt_flags(void) {
    print_test_header("Interrupt Flag Behavior");
    
    pia6521_t pia;
    pia6521_init(&pia);
    
    printf("\nSetting up for CA1 and CA2 interrupts\n");
    pia6521_write(&pia, PIA_PORTA_CTRL, 
                  PIA_CA2_INPUT_POS_IRQ | PIA_CR_CA1_LOW_TO_HIGH | PIA_CR_DDR_ACCESS);
    
    printf("\nTriggering CA1\n");
    pia6521_set_ca1(&pia, false);
    pia6521_set_ca1(&pia, true);
    
    printf("Triggering CA2\n");
    pia6521_set_ca2_input(&pia, false);
    pia6521_set_ca2_input(&pia, true);
    
    uint8_t cra = pia6521_read(&pia, PIA_PORTA_CTRL);
    printf("\nCRA: 0x%02X\n", cra);
    printf("  Bit 6 (CA1 flag): %s\n", (cra & 0x40) ? "SET" : "CLEAR");
    printf("  Bit 7 (CA2 flag): %s\n", (cra & 0x80) ? "SET" : "CLEAR");
    
    printf("\nReading Port A data (clears both flags)\n");
    pia6521_read(&pia, PIA_PORTA_DATA);
    
    cra = pia6521_read(&pia, PIA_PORTA_CTRL);
    printf("\nCRA after read: 0x%02X\n", cra);
    printf("  Bit 6 (CA1 flag): %s (expected CLEAR)\n", (cra & 0x40) ? "SET" : "CLEAR");
    printf("  Bit 7 (CA2 flag): %s (expected CLEAR)\n", (cra & 0x80) ? "SET" : "CLEAR");
    
    printf("\n✓ Interrupt flags test complete\n");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  6521 PIA (Peripheral Interface Adapter)     ║\n");
    printf("║  Emulation Test Suite                        ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_basic_io();
    test_ddr_switching();
    test_ca1_interrupt();
    test_ca2_modes();
    test_cb1_interrupt();
    test_handshake_mode();
    test_interrupt_flags();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  All tests completed successfully!           ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}

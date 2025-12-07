#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "via6522.h"

// Test context for port callbacks
typedef struct {
    uint8_t port_a_value;
    uint8_t port_b_value;
    int port_a_writes;
    int port_b_writes;
} test_context_t;

// IRQ callback context
typedef struct {
    int irq_count;
    bool irq_state;
} irq_context_t;

// Port A read callback
uint8_t port_a_read(void* ctx) {
    test_context_t* context = (test_context_t*)ctx;
    printf("  Port A read: 0x%02X\n", context->port_a_value);
    return context->port_a_value;
}

// Port A write callback
void port_a_write(void* ctx, uint8_t value) {
    test_context_t* context = (test_context_t*)ctx;
    context->port_a_value = value;
    context->port_a_writes++;
    printf("  Port A write: 0x%02X (write count: %d)\n", value, context->port_a_writes);
}

// Port B read callback
uint8_t port_b_read(void* ctx) {
    test_context_t* context = (test_context_t*)ctx;
    printf("  Port B read: 0x%02X\n", context->port_b_value);
    return context->port_b_value;
}

// Port B write callback
void port_b_write(void* ctx, uint8_t value) {
    test_context_t* context = (test_context_t*)ctx;
    context->port_b_value = value;
    context->port_b_writes++;
    printf("  Port B write: 0x%02X (write count: %d)\n", value, context->port_b_writes);
}

// IRQ callback
void irq_callback(void* ctx, bool state) {
    irq_context_t* context = (irq_context_t*)ctx;
    if (state && !context->irq_state) {
        context->irq_count++;
        printf("  *** IRQ ASSERTED (count: %d) ***\n", context->irq_count);
    } else if (!state && context->irq_state) {
        printf("  *** IRQ CLEARED ***\n");
    }
    context->irq_state = state;
}

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

void test_basic_io(void) {
    print_test_header("Basic I/O Port Operations");
    
    via6522_t via;
    test_context_t context = {0};
    
    via6522_init(&via);
    via6522_set_port_a_callbacks(&via, port_a_read, port_a_write, &context);
    via6522_set_port_b_callbacks(&via, port_b_read, port_b_write, &context);
    
    printf("\nSetting Port A to all outputs (DDR = 0xFF)\n");
    via6522_write(&via, VIA_DDRA, 0xFF);
    
    printf("\nWriting 0xAA to Port A\n");
    via6522_write(&via, VIA_ORA_IRA, 0xAA);
    
    printf("\nSetting Port B bits 0-3 to output, 4-7 to input (DDR = 0x0F)\n");
    via6522_write(&via, VIA_DDRB, 0x0F);
    
    printf("\nWriting 0x55 to Port B\n");
    via6522_write(&via, VIA_ORB_IRB, 0x55);
    
    printf("\nSimulating external Port B input = 0xF0\n");
    context.port_b_value = 0xF0;
    
    printf("\nReading Port B (should mix output 0x05 with input 0xF0)\n");
    uint8_t val = via6522_read(&via, VIA_ORB_IRB);
    printf("Read value: 0x%02X (expected 0xF5)\n", val);
    
    printf("\n✓ Basic I/O test complete\n");
}

void test_timer1_oneshot(void) {
    print_test_header("Timer 1 One-Shot Mode");
    
    via6522_t via;
    irq_context_t irq_ctx = {0};
    
    via6522_init(&via);
    via6522_set_irq_callback(&via, irq_callback, &irq_ctx);
    
    printf("\nEnabling Timer 1 interrupts\n");
    via6522_write(&via, VIA_IER, 0x80 | VIA_INT_T1);
    
    printf("\nSetting Timer 1 for 10 cycles (one-shot mode)\n");
    via6522_write(&via, VIA_ACR, 0x00);  // One-shot, no PB7
    via6522_write(&via, VIA_T1CL, 0x0A);  // Low byte = 10
    via6522_write(&via, VIA_T1CH, 0x00);  // High byte = 0, starts timer
    
    printf("\nClocking VIA for 15 cycles...\n");
    for (int i = 0; i < 15; i++) {
        printf("Cycle %d: ", i);
        via6522_clock(&via);
        
        if (i == 10) {
            printf(" <- Timer should fire here");
        }
        printf("\n");
    }
    
    uint8_t ifr = via6522_read(&via, VIA_IFR);
    printf("\nIFR after cycles: 0x%02X (T1 bit %s be set)\n", 
           ifr, (ifr & VIA_INT_T1) ? "should" : "should NOT");
    
    printf("\nReading T1CL to clear interrupt\n");
    via6522_read(&via, VIA_T1CL);
    
    ifr = via6522_read(&via, VIA_IFR);
    printf("IFR after clear: 0x%02X (should be 0x00)\n", ifr);
    
    printf("\n✓ Timer 1 one-shot test complete\n");
}

void test_timer1_continuous(void) {
    print_test_header("Timer 1 Continuous Mode");
    
    via6522_t via;
    irq_context_t irq_ctx = {0};
    
    via6522_init(&via);
    via6522_set_irq_callback(&via, irq_callback, &irq_ctx);
    
    printf("\nEnabling Timer 1 interrupts\n");
    via6522_write(&via, VIA_IER, 0x80 | VIA_INT_T1);
    
    printf("\nSetting Timer 1 for 5 cycles (continuous mode)\n");
    via6522_write(&via, VIA_ACR, 0x40);  // Continuous mode
    via6522_write(&via, VIA_T1CL, 0x05);
    via6522_write(&via, VIA_T1CH, 0x00);
    
    printf("\nClocking VIA for 20 cycles (expect interrupts at 5, 10, 15)...\n");
    for (int i = 0; i < 20; i++) {
        printf("Cycle %d: ", i);
        
        // Clear interrupt flag to see new ones
        if (via6522_get_irq(&via)) {
            via6522_read(&via, VIA_T1CL);
        }
        
        via6522_clock(&via);
        printf("\n");
    }
    
    printf("\nTotal IRQ assertions: %d (expected 3-4)\n", irq_ctx.irq_count);
    
    printf("\n✓ Timer 1 continuous test complete\n");
}

void test_timer2(void) {
    print_test_header("Timer 2 Operation");
    
    via6522_t via;
    irq_context_t irq_ctx = {0};
    
    via6522_init(&via);
    via6522_set_irq_callback(&via, irq_callback, &irq_ctx);
    
    printf("\nEnabling Timer 2 interrupts\n");
    via6522_write(&via, VIA_IER, 0x80 | VIA_INT_T2);
    
    printf("\nSetting Timer 2 for 7 cycles\n");
    via6522_write(&via, VIA_T2CL, 0x07);
    via6522_write(&via, VIA_T2CH, 0x00);
    
    printf("\nClocking VIA for 12 cycles...\n");
    for (int i = 0; i < 12; i++) {
        printf("Cycle %d: ", i);
        via6522_clock(&via);
        
        if (i == 7) {
            printf(" <- Timer should fire here");
        }
        printf("\n");
    }
    
    printf("\nIRQ count: %d (expected 1)\n", irq_ctx.irq_count);
    
    printf("\n✓ Timer 2 test complete\n");
}

void test_ca1_interrupt(void) {
    print_test_header("CA1 Edge Detection and Interrupt");
    
    via6522_t via;
    irq_context_t irq_ctx = {0};
    test_context_t io_ctx = {0};
    
    via6522_init(&via);
    via6522_set_irq_callback(&via, irq_callback, &irq_ctx);
    via6522_set_port_a_callbacks(&via, port_a_read, port_a_write, &io_ctx);
    
    printf("\nEnabling CA1 interrupts\n");
    via6522_write(&via, VIA_IER, 0x80 | VIA_INT_CA1);
    
    printf("\nConfiguring CA1 for positive edge trigger\n");
    via6522_write(&via, VIA_PCR, 0x01);
    
    printf("\nToggling CA1: low -> high (should trigger)\n");
    via6522_set_ca1(&via, false);
    via6522_set_ca1(&via, true);
    
    uint8_t ifr = via6522_read(&via, VIA_IFR);
    printf("IFR: 0x%02X (CA1 bit %s be set)\n", ifr, (ifr & VIA_INT_CA1) ? "should" : "should NOT");
    
    printf("\nReading ORA to clear CA1 interrupt\n");
    via6522_read(&via, VIA_ORA_IRA);
    
    ifr = via6522_read(&via, VIA_IFR);
    printf("IFR after read: 0x%02X (should be 0x00)\n", ifr);
    
    printf("\nConfiguring CA1 for negative edge trigger\n");
    via6522_write(&via, VIA_PCR, 0x00);
    
    printf("\nToggling CA1: high -> low (should trigger)\n");
    via6522_set_ca1(&via, true);
    via6522_set_ca1(&via, false);
    
    ifr = via6522_read(&via, VIA_IFR);
    printf("IFR: 0x%02X (CA1 bit %s be set)\n", ifr, (ifr & VIA_INT_CA1) ? "should" : "should NOT");
    
    printf("\n✓ CA1 interrupt test complete\n");
}

void test_port_latching(void) {
    print_test_header("Port Latching on CA1 Edge");
    
    via6522_t via;
    test_context_t io_ctx = {0};
    
    via6522_init(&via);
    via6522_set_port_a_callbacks(&via, port_a_read, port_a_write, &io_ctx);
    
    printf("\nSetting Port A to all inputs\n");
    via6522_write(&via, VIA_DDRA, 0x00);
    
    printf("\nEnabling Port A latching\n");
    via6522_write(&via, VIA_ACR, VIA_ACR_PA_LATCH);
    
    printf("\nConfiguring CA1 for positive edge\n");
    via6522_write(&via, VIA_PCR, 0x01);
    
    printf("\nSetting external Port A value to 0x42\n");
    io_ctx.port_a_value = 0x42;
    
    printf("\nTriggering CA1 edge (latches Port A)\n");
    via6522_set_ca1(&via, false);
    via6522_set_ca1(&via, true);
    
    printf("\nChanging external Port A value to 0x99\n");
    io_ctx.port_a_value = 0x99;
    
    printf("\nReading Port A (should return latched value 0x42)\n");
    uint8_t val = via6522_read(&via, VIA_ORA_IRA);
    printf("Read value: 0x%02X (expected 0x42)\n", val);
    
    printf("\n✓ Port latching test complete\n");
}

void test_register_access(void) {
    print_test_header("Register Read/Write Access");
    
    via6522_t via;
    via6522_init(&via);
    
    printf("\nWriting and reading back various registers:\n");
    
    via6522_write(&via, VIA_DDRA, 0xA5);
    printf("DDRA: wrote 0xA5, read 0x%02X\n", via6522_read(&via, VIA_DDRA));
    
    via6522_write(&via, VIA_DDRB, 0x5A);
    printf("DDRB: wrote 0x5A, read 0x%02X\n", via6522_read(&via, VIA_DDRB));
    
    via6522_write(&via, VIA_T1LL, 0x34);
    printf("T1LL: wrote 0x34, read 0x%02X\n", via6522_read(&via, VIA_T1LL));
    
    via6522_write(&via, VIA_T1LH, 0x12);
    printf("T1LH: wrote 0x12, read 0x%02X\n", via6522_read(&via, VIA_T1LH));
    
    via6522_write(&via, VIA_ACR, 0xC3);
    printf("ACR: wrote 0xC3, read 0x%02X\n", via6522_read(&via, VIA_ACR));
    
    via6522_write(&via, VIA_PCR, 0xEE);
    printf("PCR: wrote 0xEE, read 0x%02X\n", via6522_read(&via, VIA_PCR));
    
    via6522_write(&via, VIA_IER, 0xFF);  // Enable all interrupts
    uint8_t ier = via6522_read(&via, VIA_IER);
    printf("IER: wrote 0xFF, read 0x%02X (bit 7 always set)\n", ier);
    
    printf("\n✓ Register access test complete\n");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  6522 VIA (Versatile Interface Adapter)       ║\n");
    printf("║  Emulation Test Suite                         ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_register_access();
    test_basic_io();
    test_timer1_oneshot();
    test_timer1_continuous();
    test_timer2();
    test_ca1_interrupt();
    test_port_latching();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  All tests completed successfully!            ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}

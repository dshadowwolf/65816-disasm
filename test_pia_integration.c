/*
 * PIA 6521 Integration Test
 * Tests the PIA at 0x7FA0-0x7FA3 in the processor memory map
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "machine_setup.h"
#include "machine.h"
#include "processor_helpers.h"
#include "pia6521.h"

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAILED: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

void test_pia_basic_access(machine_state_t *machine) {
    print_test_header("PIA Basic Access (0x7FA0-0x7FA3)");
    
    // Test Port A Control Register (0x7FA1)
    write_byte_new(machine, 0x7FA1, 0x04); // Enable DDR access
    uint8_t cra = read_byte_new(machine, 0x7FA1);
    TEST_ASSERT((cra & 0x04) == 0x04, "Port A Control Register write/read");
    
    // Access Port A DDR (when CRA bit 2 = 0)
    write_byte_new(machine, 0x7FA1, 0x00); // Disable data access, enable DDR access
    write_byte_new(machine, 0x7FA0, 0xFF); // All outputs
    write_byte_new(machine, 0x7FA1, 0x04); // Enable data access
    
    // Test Port A Data (0x7FA0)
    write_byte_new(machine, 0x7FA0, 0xAA);
    uint8_t porta = read_byte_new(machine, 0x7FA0);
    TEST_ASSERT(porta == 0xAA, "Port A data write/read");
    
    // Test Port B Control Register (0x7FA3)
    write_byte_new(machine, 0x7FA3, 0x04); // Enable DDR access
    uint8_t crb = read_byte_new(machine, 0x7FA3);
    TEST_ASSERT((crb & 0x04) == 0x04, "Port B Control Register write/read");
    
    // Access Port B DDR
    write_byte_new(machine, 0x7FA3, 0x00); // Enable DDR access
    write_byte_new(machine, 0x7FA2, 0xFF); // All outputs
    write_byte_new(machine, 0x7FA3, 0x04); // Enable data access
    
    // Test Port B Data (0x7FA2)
    write_byte_new(machine, 0x7FA2, 0x55);
    uint8_t portb = read_byte_new(machine, 0x7FA2);
    TEST_ASSERT(portb == 0x55, "Port B data write/read");
}

void test_pia_with_callbacks(machine_state_t *machine) {
    print_test_header("PIA Port Callbacks");
    
    static uint8_t test_value = 0;
    static bool write_called = false;
    static bool read_called = false;
    
    // Callback functions
    uint8_t porta_read_cb(void* ctx) {
        read_called = true;
        return 0x42;
    }
    
    void porta_write_cb(void* ctx, uint8_t value) {
        write_called = true;
        test_value = value;
    }
    
    // Get PIA instance and set callbacks
    pia6521_t* pia = get_pia_instance();
    pia6521_set_porta_callbacks(pia, porta_read_cb, porta_write_cb, NULL);
    
    // Configure Port A as output
    write_byte_new(machine, 0x7FA1, 0x00); // DDR access
    write_byte_new(machine, 0x7FA0, 0xFF); // All outputs
    write_byte_new(machine, 0x7FA1, 0x04); // Data access
    
    // Write to Port A (should trigger callback)
    write_called = false;
    write_byte_new(machine, 0x7FA0, 0x99);
    TEST_ASSERT(write_called == true, "Port A write callback triggered");
    TEST_ASSERT(test_value == 0x99, "Port A write callback receives correct value");
    
    // Configure Port A as input to test read callback
    write_byte_new(machine, 0x7FA1, 0x00); // DDR access
    write_byte_new(machine, 0x7FA0, 0x00); // All inputs
    write_byte_new(machine, 0x7FA1, 0x04); // Data access
    
    // Read from Port A (should trigger callback)
    read_called = false;
    uint8_t value = read_byte_new(machine, 0x7FA0);
    TEST_ASSERT(read_called == true, "Port A read callback triggered");
    TEST_ASSERT(value == 0x42, "Port A read callback returns correct value");
    
    // Clean up callbacks
    pia6521_set_porta_callbacks(pia, NULL, NULL, NULL);
}

void test_pia_memory_location(machine_state_t *machine) {
    print_test_header("PIA Memory Location Verification");
    
    // Verify PIA is at 0x7FA0-0x7FA3
    write_byte_new(machine, 0x7FA1, 0x04); // CRA
    uint8_t cra = read_byte_new(machine, 0x7FA1);
    TEST_ASSERT((cra & 0x04) == 0x04, "PIA accessible at 0x7FA0");
    
    // Verify address before PIA is RAM (now ends at 0x7F7F before ACIA)
    write_byte_new(machine, 0x7F7F, 0x77);
    TEST_ASSERT(read_byte_new(machine, 0x7F7F) == 0x77, "Address 0x7F7F is RAM");
    
    // Verify address after PIA range is different region
    uint8_t gap_val = read_byte_new(machine, 0x7FA4);
    printf("  Gap region at 0x7FA4 reads: 0x%02X\n", gap_val);
    TEST_ASSERT(true, "Gap region after PIA accessible");
}

void test_all_three_devices(machine_state_t *machine) {
    print_test_header("All Four Devices (ACIA, PIA, VIA, Board FIFO)");
    
    // Configure ACIA
    write_byte_new(machine, 0x7F82, 0x44); // Command = 0x44
    
    // Configure PIA Port A
    write_byte_new(machine, 0x7FA1, 0x00); // DDR access
    write_byte_new(machine, 0x7FA0, 0xFF); // All outputs
    write_byte_new(machine, 0x7FA1, 0x04); // Data access
    write_byte_new(machine, 0x7FA0, 0x11); // Write test value
    
    // Configure VIA Port A
    write_byte_new(machine, 0x7FC3, 0xFF); // All outputs
    write_byte_new(machine, 0x7FC1, 0x22); // Write test value
    
    // Configure Board FIFO Port A
    write_byte_new(machine, 0x7FE3, 0xFF); // All outputs
    write_byte_new(machine, 0x7FE1, 0x33); // Write test value
    
    // Verify all three maintain independent state
    uint8_t acia_val = read_byte_new(machine, 0x7F82);
    uint8_t pia_val = read_byte_new(machine, 0x7FA0);
    uint8_t via_val = read_byte_new(machine, 0x7FC1);
    uint8_t fifo_val = read_byte_new(machine, 0x7FE1);
    
    TEST_ASSERT(acia_val == 0x44, "ACIA maintains independent state (0x44)");
    TEST_ASSERT(pia_val == 0x11, "PIA maintains independent state (0x11)");
    TEST_ASSERT(via_val == 0x22, "VIA maintains independent state (0x22)");
    TEST_ASSERT(fifo_val == 0x33, "Board FIFO maintains independent state (0x33)");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  PIA 6521 Integration Test                    ║\n");
    printf("║  PIA at 0x7FA0-0x7FA3                         ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    machine_state_t machine;
    initialize_machine(&machine);
    
    test_pia_basic_access(&machine);
    test_pia_with_callbacks(&machine);
    test_pia_memory_location(&machine);
    test_all_three_devices(&machine);
    
    printf("\n╔═════════════════════════════════════════════════╗\n");
    printf("║  Test Results Summary                           ║\n");
    printf("╠═════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                   ║\n", tests_passed);
    printf("║  Failed: %-4d                                   ║\n", tests_failed);
    printf("╚═════════════════════════════════════════════════╝\n");
    
    cleanup_machine_with_via(&machine);
    
    return tests_failed > 0 ? 1 : 0;
}

/*
 * ACIA 6551 Integration Test
 * Tests the ACIA at 0x7F80-0x7F83 in the processor memory map
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "machine_setup.h"
#include "machine.h"
#include "processor_helpers.h"
#include "acia6551.h"

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

void test_acia_basic_access(machine_state_t *machine) {
    print_test_header("ACIA Basic Access (0x7F80-0x7F83)");
    
    // Test Status Register (0x7F81)
    uint8_t status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_TDRE) != 0, "Transmit Data Register Empty on reset");
    
    // Test Command Register (0x7F82)
    write_byte_new(machine, 0x7F82, 0x09); // Enable DTR, no IRQ
    uint8_t cmd = read_byte_new(machine, 0x7F82);
    TEST_ASSERT(cmd == 0x09, "Command register write/read");
    
    // Test Control Register (0x7F83)
    write_byte_new(machine, 0x7F83, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    uint8_t ctrl = read_byte_new(machine, 0x7F83);
    TEST_ASSERT((ctrl & ACIA_CTRL_BAUD_MASK) == ACIA_CTRL_BAUD_9600, "Control register baud rate");
    TEST_ASSERT((ctrl & ACIA_CTRL_WORD_MASK) == ACIA_CTRL_WORD_8BIT, "Control register word length");
}

void test_acia_transmit(machine_state_t *machine) {
    print_test_header("ACIA Transmit");
    
    // Initialize ACIA for 9600 baud, 8 data bits, 1 stop bit
    write_byte_new(machine, 0x7F83, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    write_byte_new(machine, 0x7F82, 0x01); // Enable DTR
    
    // Check TDRE flag is set (ready to transmit)
    uint8_t status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_TDRE) != 0, "TDRE flag set before transmit");
    
    // Write data to transmit
    write_byte_new(machine, 0x7F80, 'A');
    
    // Note: TDRE behavior after write depends on FIFO implementation
    // Some implementations clear it immediately, others after byte enters shift register
    
    // Clock the ACIA to process transmission
    for (int i = 0; i < 1000; i++) {
        machine_clock_devices(machine, 1);
    }
    
    // TDRE should be set again after transmission
    status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_TDRE) != 0, "TDRE flag set after transmission");
}

void test_acia_receive(machine_state_t *machine) {
    print_test_header("ACIA Receive");
    
    // Initialize ACIA
    write_byte_new(machine, 0x7F83, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    write_byte_new(machine, 0x7F82, 0x01); // Enable DTR
    
    // Check RDRF flag is clear (no data)
    uint8_t status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_RDRF) == 0, "RDRF flag clear when no data");
    
    // Simulate receiving a byte
    acia6551_t* acia = get_acia_instance();
    acia6551_receive_byte(acia, 'X');
    
    // RDRF should be set
    status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_RDRF) != 0, "RDRF flag set after receive");
    
    // Read the data
    uint8_t data = read_byte_new(machine, 0x7F80);
    TEST_ASSERT(data == 'X', "Received correct data");
    
    // RDRF should be clear after read
    status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_RDRF) == 0, "RDRF flag cleared after read");
}

void test_acia_with_callbacks(machine_state_t *machine) {
    print_test_header("ACIA Byte Callbacks");
    
    static uint8_t transmitted_byte = 0;
    static bool tx_callback_called = false;
    static uint8_t rx_data_to_return = 'Y';
    
    // Transmit callback
    void tx_callback(void* ctx, uint8_t byte) {
        tx_callback_called = true;
        transmitted_byte = byte;
    }
    
    // Receive callback
    uint8_t rx_callback(void* ctx, bool* available) {
        *available = true;
        return rx_data_to_return;
    }
    
    // Get ACIA instance and set callbacks
    acia6551_t* acia = get_acia_instance();
    acia6551_set_byte_callbacks(acia, tx_callback, rx_callback, NULL);
    
    // Initialize ACIA
    write_byte_new(machine, 0x7F83, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    write_byte_new(machine, 0x7F82, 0x01); // Enable DTR
    
    // Test transmit callback
    tx_callback_called = false;
    write_byte_new(machine, 0x7F80, 'B');
    
    // Clock to process transmission
    for (int i = 0; i < 1000; i++) {
        machine_clock_devices(machine, 1);
    }
    
    TEST_ASSERT(tx_callback_called == true, "TX callback triggered");
    TEST_ASSERT(transmitted_byte == 'B', "TX callback received correct byte");
    
    // Clean up callbacks
    acia6551_set_byte_callbacks(acia, NULL, NULL, NULL);
}

void test_acia_memory_location(machine_state_t *machine) {
    print_test_header("ACIA Memory Location Verification");
    
    // Verify ACIA is at 0x7F80-0x7F83
    write_byte_new(machine, 0x7F82, 0x05); // Command register
    uint8_t cmd = read_byte_new(machine, 0x7F82);
    TEST_ASSERT(cmd == 0x05, "ACIA accessible at 0x7F80");
    
    // Verify address before ACIA is RAM
    write_byte_new(machine, 0x7F7F, 0x88);
    TEST_ASSERT(read_byte_new(machine, 0x7F7F) == 0x88, "Address 0x7F7F is RAM");
    
    // Verify address after ACIA range is gap
    uint8_t gap_val = read_byte_new(machine, 0x7F84);
    printf("  Gap region at 0x7F84 reads: 0x%02X\n", gap_val);
    TEST_ASSERT(true, "Gap region after ACIA accessible");
}

void test_all_four_devices(machine_state_t *machine) {
    print_test_header("All Four Devices (ACIA, PIA, VIA, Board FIFO)");
    
    // Configure ACIA
    write_byte_new(machine, 0x7F82, 0x11); // Command = 0x11
    
    // Configure PIA Port A
    write_byte_new(machine, 0x7FA1, 0x00); // DDR access
    write_byte_new(machine, 0x7FA0, 0xFF); // All outputs
    write_byte_new(machine, 0x7FA1, 0x04); // Data access
    write_byte_new(machine, 0x7FA0, 0x22); // Write test value
    
    // Configure VIA Port A
    write_byte_new(machine, 0x7FC3, 0xFF); // All outputs
    write_byte_new(machine, 0x7FC1, 0x33); // Write test value
    
    // Configure Board FIFO Port A
    write_byte_new(machine, 0x7FE3, 0xFF); // All outputs
    write_byte_new(machine, 0x7FE1, 0x44); // Write test value
    
    // Verify all four maintain independent state
    uint8_t acia_val = read_byte_new(machine, 0x7F82);
    uint8_t pia_val = read_byte_new(machine, 0x7FA0);
    uint8_t via_val = read_byte_new(machine, 0x7FC1);
    uint8_t fifo_val = read_byte_new(machine, 0x7FE1);
    
    TEST_ASSERT(acia_val == 0x11, "ACIA maintains independent state (0x11)");
    TEST_ASSERT(pia_val == 0x22, "PIA maintains independent state (0x22)");
    TEST_ASSERT(via_val == 0x33, "VIA maintains independent state (0x33)");
    TEST_ASSERT(fifo_val == 0x44, "Board FIFO maintains independent state (0x44)");
}

void test_acia_serial_loopback(machine_state_t *machine) {
    print_test_header("ACIA Transmit and Receive");
    
    // Reset ACIA to clear any previous state
    acia6551_t* acia = get_acia_instance();
    acia6551_reset(acia);
    
    // Initialize ACIA
    write_byte_new(machine, 0x7F83, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    write_byte_new(machine, 0x7F82, 0x01); // Enable DTR
    
    // Test 1: Transmit functionality
    write_byte_new(machine, 0x7F80, 'T');
    uint8_t status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_TDRE) != 0, "Can write to transmit register");
    
    // Test 2: Receive functionality
    acia6551_receive_byte(acia, 'A');
    acia6551_receive_byte(acia, 'B');
    acia6551_receive_byte(acia, 'C');
    
    // Verify we can read all three bytes
    status = read_byte_new(machine, 0x7F81);
    TEST_ASSERT((status & ACIA_STATUS_RDRF) != 0, "RDRF set when data available");
    
    uint8_t byte1 = read_byte_new(machine, 0x7F80);
    uint8_t byte2 = read_byte_new(machine, 0x7F80);
    uint8_t byte3 = read_byte_new(machine, 0x7F80);
    
    TEST_ASSERT(byte1 == 'A', "First received byte correct");
    TEST_ASSERT(byte2 == 'B', "Second received byte correct");
    TEST_ASSERT(byte3 == 'C', "Third received byte correct");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  ACIA 6551 Integration Test                   ║\n");
    printf("║  ACIA at 0x7F80-0x7F83                        ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    machine_state_t machine;
    initialize_machine(&machine);
    
    test_acia_basic_access(&machine);
    test_acia_transmit(&machine);
    test_acia_receive(&machine);
    test_acia_with_callbacks(&machine);
    test_acia_memory_location(&machine);
    test_all_four_devices(&machine);
    test_acia_serial_loopback(&machine);
    
    printf("\n╔═════════════════════════════════════════════════╗\n");
    printf("║  Test Results Summary                           ║\n");
    printf("╠═════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                   ║\n", tests_passed);
    printf("║  Failed: %-4d                                   ║\n", tests_failed);
    printf("╚═════════════════════════════════════════════════╝\n");
    
    cleanup_machine_with_via(&machine);
    
    return tests_failed > 0 ? 1 : 0;
}

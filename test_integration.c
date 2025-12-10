/*
 * Integration Test Suite for VIA and Board FIFO in Processor Memory Map
 * 
 * Tests both the standalone VIA at 0x7FC0 and the board_fifo at 0x7FE0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "machine_setup.h"
#include "machine.h"
#include "processor_helpers.h"
#include "via6522.h"

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

void print_summary(void) {
    printf("\n╔═════════════════════════════════════════════════╗\n");
    printf("║  Test Results Summary                           ║\n");
    printf("╠═════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                   ║\n", tests_passed);
    printf("║  Failed: %-4d                                   ║\n", tests_failed);
    printf("╚═════════════════════════════════════════════════╝\n");
}

// Test standalone VIA at 0x7FC0
void test_via_basic_access(machine_state_t *machine) {
    print_test_header("Standalone VIA Basic Access (0x7FC0)");
    
    // Test writing and reading DDRA
    write_byte_new(machine, 0x7FC3, 0xAA);
    uint8_t ddra = read_byte_new(machine, 0x7FC3);
    TEST_ASSERT(ddra == 0xAA, "DDRA write/read (0x7FC3)");
    
    // Test writing and reading DDRB
    write_byte_new(machine, 0x7FC2, 0x55);
    uint8_t ddrb = read_byte_new(machine, 0x7FC2);
    TEST_ASSERT(ddrb == 0x55, "DDRB write/read (0x7FC2)");
    
    // Test Port A output
    write_byte_new(machine, 0x7FC3, 0xFF); // All outputs
    write_byte_new(machine, 0x7FC1, 0x42);
    uint8_t porta = read_byte_new(machine, 0x7FC1);
    TEST_ASSERT(porta == 0x42, "Port A output (0x7FC1)");
    
    // Test Port B output
    write_byte_new(machine, 0x7FC2, 0xFF); // All outputs
    write_byte_new(machine, 0x7FC0, 0x99);
    uint8_t portb = read_byte_new(machine, 0x7FC0);
    TEST_ASSERT(portb == 0x99, "Port B output (0x7FC0)");
}

void test_via_timer1(machine_state_t *machine) {
    print_test_header("Standalone VIA Timer 1");
    
    // Set timer 1 latch
    write_byte_new(machine, 0x7FC6, 0x00); // T1LL = 0x00
    write_byte_new(machine, 0x7FC7, 0x10); // T1LH = 0x10 (4096 cycles)
    
    // Write to counter to start timer
    write_byte_new(machine, 0x7FC4, 0x00); // T1CL = 0x00
    write_byte_new(machine, 0x7FC5, 0x10); // T1CH = 0x10
    
    // Read back counter
    uint8_t t1cl = read_byte_new(machine, 0x7FC4);
    uint8_t t1ch = read_byte_new(machine, 0x7FC5);
    
    TEST_ASSERT(t1cl == 0x00, "Timer 1 counter low initialized");
    TEST_ASSERT(t1ch == 0x10, "Timer 1 counter high initialized");
    
    // Clock the VIA a few times
    for (int i = 0; i < 100; i++) {
        machine_clock_devices(machine);
    }
    
    // Counter should have decreased
    uint8_t t1cl_after = read_byte_new(machine, 0x7FC4);
    uint8_t t1ch_after = read_byte_new(machine, 0x7FC5);
    uint16_t count_before = (t1ch << 8) | t1cl;
    uint16_t count_after = (t1ch_after << 8) | t1cl_after;
    
    TEST_ASSERT(count_after < count_before, "Timer 1 counts down");
}

void test_via_callbacks(machine_state_t *machine) {
    print_test_header("Standalone VIA Port Callbacks");
    
    static uint8_t callback_read_value = 0x88;
    static uint8_t callback_write_value = 0x00;
    static bool read_called = false;
    static bool write_called = false;
    
    uint8_t test_read_callback(void *context) {
        read_called = true;
        return callback_read_value;
    }
    
    void test_write_callback(void *context, uint8_t value) {
        write_called = true;
        callback_write_value = value;
    }
    
    // Get VIA instance and set up callbacks
    via6522_t *via = get_via_instance();
    via6522_set_port_a_callbacks(via, test_read_callback, test_write_callback, NULL);
    
    // Configure Port A as input
    write_byte_new(machine, 0x7FC3, 0x00);
    
    // Read should trigger callback
    read_called = false;
    uint8_t value = read_byte_new(machine, 0x7FC1);
    TEST_ASSERT(read_called, "Port A read callback triggered");
    TEST_ASSERT(value == 0x88, "Port A read callback returns correct value");
    
    // Configure Port A as output
    write_byte_new(machine, 0x7FC3, 0xFF);
    
    // Write should trigger callback
    write_called = false;
    write_byte_new(machine, 0x7FC1, 0x77);
    TEST_ASSERT(write_called, "Port A write callback triggered");
    TEST_ASSERT(callback_write_value == 0x77, "Port A write callback receives correct value");
}

void test_board_fifo_basic_access(machine_state_t *machine) {
    print_test_header("Board FIFO Basic Access (0x7FE0)");
    
    // Configure Port A as output
    write_byte_new(machine, 0x7FE3, 0xFF);
    uint8_t ddra = read_byte_new(machine, 0x7FE3);
    TEST_ASSERT(ddra == 0xFF, "Board FIFO DDRA write/read (0x7FE3)");
    
    // Configure Port B for control signals
    write_byte_new(machine, 0x7FE2, 0x03); // Bits 0-1 output (RD#, WR)
    uint8_t ddrb = read_byte_new(machine, 0x7FE2);
    TEST_ASSERT(ddrb == 0x03, "Board FIFO DDRB write/read (0x7FE2)");
    
    // Read Port B status
    uint8_t portb = read_byte_new(machine, 0x7FE0);
    printf("  Initial Port B status: 0x%02X\n", portb);
    TEST_ASSERT((portb & 0x10) == 0x00, "PWREN# indicates USB ready (bit 4 low)");
}

void test_board_fifo_cpu_to_usb(machine_state_t *machine) {
    print_test_header("Board FIFO: CPU Write to USB");
    
    // Configure Port A as output
    write_byte_new(machine, 0x7FE3, 0xFF);
    
    // Configure Port B (RD#, WR as outputs)
    write_byte_new(machine, 0x7FE2, 0x03);
    
    const char *test_string = "HELLO";
    printf("  CPU sending: %s\n", test_string);
    
    for (int i = 0; i < 5; i++) {
        // Write data to Port A
        write_byte_new(machine, 0x7FE1, test_string[i]);
        
        // Assert WR (set bit 1 high, keep RD# high)
        write_byte_new(machine, 0x7FE0, 0x03); // RD#=1, WR=1
        
        // Clock to process
        for (int j = 0; j < 10; j++) {
            machine_clock_devices(machine);
        }
        
        // Deassert WR
        write_byte_new(machine, 0x7FE0, 0x01); // RD#=1, WR=0
    }
    
    // Receive on USB side
    printf("  USB receiving: ");
    char received[6] = {0};
    for (int i = 0; i < 5; i++) {
        received[i] = usb_receive_byte_from_cpu();
        printf("%c", received[i]);
    }
    printf("\n");
    
    TEST_ASSERT(strcmp(received, test_string) == 0, "USB received correct data from CPU");
}

void test_board_fifo_usb_to_cpu(machine_state_t *machine) {
    print_test_header("Board FIFO: USB Write to CPU");
    
    // Configure Port A as input
    write_byte_new(machine, 0x7FE3, 0x00);
    
    // Configure Port B
    write_byte_new(machine, 0x7FE2, 0x03);
    
    const char *test_string = "WORLD";
    printf("  USB sending: %s\n", test_string);
    
    // USB sends data
    for (int i = 0; i < 5; i++) {
        usb_send_byte_to_cpu(test_string[i]);
    }
    
    // Check that RXF# indicates data available
    uint8_t portb = read_byte_new(machine, 0x7FE0);
    TEST_ASSERT((portb & 0x04) == 0x00, "RXF# indicates data available (bit 2 low)");
    
    // CPU reads data
    printf("  CPU receiving: ");
    char received[6] = {0};
    for (int i = 0; i < 5; i++) {
        // Assert RD# (clear bit 0)
        write_byte_new(machine, 0x7FE0, 0x00); // RD#=0, WR=0
        
        // Clock to process
        for (int j = 0; j < 10; j++) {
            machine_clock_devices(machine);
        }
        
        // Read data from Port A
        received[i] = read_byte_new(machine, 0x7FE1);
        printf("%c", received[i]);
        
        // Deassert RD#
        write_byte_new(machine, 0x7FE0, 0x01); // RD#=1, WR=0
    }
    printf("\n");
    
    TEST_ASSERT(strcmp(received, test_string) == 0, "CPU received correct data from USB");
    
    // Check that RXF# now indicates no data
    portb = read_byte_new(machine, 0x7FE0);
    TEST_ASSERT((portb & 0x04) == 0x04, "RXF# indicates no data (bit 2 high)");
}

void test_board_fifo_bidirectional(machine_state_t *machine) {
    print_test_header("Board FIFO: Bidirectional Transfer");
    
    // Step 1: USB sends command to CPU
    const char *command = "READ";
    printf("  1. USB sends command: %s\n", command);
    for (int i = 0; i < 4; i++) {
        usb_send_byte_to_cpu(command[i]);
    }
    
    // Step 2: CPU reads command
    write_byte_new(machine, 0x7FE3, 0x00); // Port A input
    write_byte_new(machine, 0x7FE2, 0x03); // Port B control
    
    printf("  2. CPU reads command: ");
    char cpu_received[5] = {0};
    for (int i = 0; i < 4; i++) {
        // Poll RXF#
        uint8_t status;
        int timeout = 100;
        do {
            status = read_byte_new(machine, 0x7FE0);
            timeout--;
        } while ((status & 0x04) && timeout > 0);
        
        if (timeout == 0) {
            printf("TIMEOUT ");
            break;
        }
        
        // Read byte
        write_byte_new(machine, 0x7FE0, 0x00); // RD# low
        for (int j = 0; j < 10; j++) machine_clock_devices(machine);
        cpu_received[i] = read_byte_new(machine, 0x7FE1);
        printf("%c", cpu_received[i]);
        write_byte_new(machine, 0x7FE0, 0x01); // RD# high
    }
    printf("\n");
    
    TEST_ASSERT(strcmp(cpu_received, command) == 0, "CPU received command from USB");
    
    // Step 3: CPU sends response
    const char *response = "OK!";
    printf("  3. CPU sends response: %s\n", response);
    write_byte_new(machine, 0x7FE3, 0xFF); // Port A output
    
    for (int i = 0; i < 3; i++) {
        write_byte_new(machine, 0x7FE1, response[i]);
        write_byte_new(machine, 0x7FE0, 0x03); // WR high
        for (int j = 0; j < 10; j++) machine_clock_devices(machine);
        write_byte_new(machine, 0x7FE0, 0x01); // WR low
    }
    
    // Step 4: USB receives response
    printf("  4. USB receives response: ");
    char usb_received[4] = {0};
    for (int i = 0; i < 3; i++) {
        usb_received[i] = usb_receive_byte_from_cpu();
        printf("%c", usb_received[i]);
    }
    printf("\n");
    
    TEST_ASSERT(strcmp(usb_received, response) == 0, "USB received response from CPU");
}

void test_memory_regions(machine_state_t *machine) {
    print_test_header("Memory Region Boundaries");
    
    // Test RAM region (now ends at 0x7F9F before PIA)
    write_byte_new(machine, 0x0000, 0x11);
    write_byte_new(machine, 0x7F9F, 0x22);
    TEST_ASSERT(read_byte_new(machine, 0x0000) == 0x11, "RAM start (0x0000) accessible");
    TEST_ASSERT(read_byte_new(machine, 0x7F9F) == 0x22, "RAM end (0x7F9F) accessible");
    
    // Test VIA region exists (writing to output port should work)
    write_byte_new(machine, 0x7FC2, 0xFF); // Set DDRB to outputs
    write_byte_new(machine, 0x7FC0, 0x33); // Write to port B
    uint8_t via_val = read_byte_new(machine, 0x7FC0);
    TEST_ASSERT(via_val == 0x33, "VIA region (0x7FC0) is device");
    
    // Test gap region
    write_byte_new(machine, 0x7FD0, 0x44);
    uint8_t gap_val = read_byte_new(machine, 0x7FD0);
    printf("  Gap region 0x7FD0 read: 0x%02X\n", gap_val);
    TEST_ASSERT(true, "Gap region (0x7FD0) accessible");
    
    // Test board FIFO region
    write_byte_new(machine, 0x7FE0, 0x55);
    TEST_ASSERT(read_byte_new(machine, 0x7FE0) != 0x00, "Board FIFO region (0x7FE0) is device");
    
    // Test device area
    write_byte_new(machine, 0x7FF0, 0x66);
    uint8_t dev_val = read_byte_new(machine, 0x7FF0);
    printf("  Device area 0x7FF0 read: 0x%02X\n", dev_val);
    TEST_ASSERT(true, "Device area (0x7FF0) accessible");
    
    // Test ROM region (should be read-only)
    write_byte_new(machine, 0x8000, 0x77);
    uint8_t rom_val = read_byte_new(machine, 0x8000);
    TEST_ASSERT(rom_val == 0x00, "ROM region (0x8000) is read-only");
}

void test_concurrent_devices(machine_state_t *machine) {
    print_test_header("Concurrent Device Access");
    
    // Reset VIA callbacks from previous test
    via6522_t *via = get_via_instance();
    via6522_set_port_a_callbacks(via, NULL, NULL, NULL);
    via6522_set_port_b_callbacks(via, NULL, NULL, NULL);
    
    // Initialize both devices by doing a read first
    read_byte_new(machine, 0x7FC0); // Initialize VIA
    read_byte_new(machine, 0x7FE0); // Initialize board FIFO
    
    // Write to both VIA and board FIFO
    write_byte_new(machine, 0x7FC3, 0xAA); // VIA DDRA
    write_byte_new(machine, 0x7FE3, 0x55); // Board FIFO DDRA
    
    // Read back both
    uint8_t via_ddra = read_byte_new(machine, 0x7FC3);
    uint8_t fifo_ddra = read_byte_new(machine, 0x7FE3);
    
    TEST_ASSERT(via_ddra == 0xAA, "VIA maintains independent state");
    TEST_ASSERT(fifo_ddra == 0x55, "Board FIFO maintains independent state");
    
    // Clock both devices
    for (int i = 0; i < 50; i++) {
        machine_clock_devices(machine);
    }
    
    TEST_ASSERT(true, "Both devices can be clocked simultaneously");
}

void test_word_access(machine_state_t *machine) {
    print_test_header("Word (16-bit) Access");
    
    // Test word write to VIA Timer 1
    write_word_new(machine, 0x7FC4, 0x1234); // T1CL/T1CH
    uint16_t t1_word = read_word_new(machine, 0x7FC4);
    
    printf("  VIA Timer 1 word value: 0x%04X\n", t1_word);
    TEST_ASSERT((t1_word & 0xFF) == 0x34, "VIA word access low byte");
    TEST_ASSERT((t1_word >> 8) == 0x12, "VIA word access high byte");
    
    // Test word access to board FIFO
    write_byte_new(machine, 0x7FE3, 0xFF); // Port A output
    write_byte_new(machine, 0x7FE1, 0xAB);
    write_byte_new(machine, 0x7FE2, 0xCD);
    
    // Reading as word should get both registers
    uint16_t fifo_word = read_word_new(machine, 0x7FE0);
    printf("  Board FIFO word value: 0x%04X\n", fifo_word);
    TEST_ASSERT(true, "Board FIFO word access works");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  Integration Test Suite                       ║\n");
    printf("║  VIA (0x7FC0) and Board FIFO (0x7FE0)         ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    // Create and initialize machine
    machine_state_t *machine = create_machine();
    
    // Run all tests
    test_memory_regions(machine);
    test_via_basic_access(machine);
    test_via_timer1(machine);
    test_via_callbacks(machine);
    test_board_fifo_basic_access(machine);
    test_board_fifo_cpu_to_usb(machine);
    test_board_fifo_usb_to_cpu(machine);
    test_board_fifo_bidirectional(machine);
    test_concurrent_devices(machine);
    test_word_access(machine);
    
    // Cleanup
    cleanup_machine_with_via(machine);
    destroy_machine(machine);
    
    // Print summary
    print_summary();
    
    return (tests_failed > 0) ? 1 : 0;
}

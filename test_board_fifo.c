#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "board_fifo.h"
#include "via6522.h"

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

void print_port_b_status(uint8_t portb) {
    printf("Port B: 0x%02X\n", portb);
    printf("  RD#:    %s\n", (portb & PORTB_RD_N) ? "HIGH (inactive)" : "LOW (active)");
    printf("  WR:     %s\n", (portb & PORTB_WR) ? "HIGH (active)" : "LOW (inactive)");
    printf("  RXF#:   %s\n", (portb & PORTB_RXF_N) ? "HIGH (no data)" : "LOW (data available)");
    printf("  TXE#:   %s\n", (portb & PORTB_TXE_N) ? "HIGH (no space)" : "LOW (space available)");
    printf("  PWREN#: %s\n", (portb & PORTB_PWREN_N) ? "HIGH (not configured)" : "LOW (USB ready)");
}

void test_initialization(void) {
    print_test_header("Board FIFO Initialization");
    
    fifo_t *fifo = init_board_fifo();
    if (!fifo) {
        printf("ERROR: Failed to initialize board FIFO\n");
        return;
    }
    
    printf("\nConfiguring VIA for FIFO operation:\n");
    printf("Setting Port A (data bus) to all outputs\n");
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);
    
    printf("Setting Port B control bits:\n");
    printf("  Bits 0-1 (RD#, WR) as outputs\n");
    printf("  Bits 2-4 (RXF#, TXE#, PWREN#) as inputs\n");
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);
    
    printf("\nReading initial Port B status:\n");
    uint8_t portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    print_port_b_status(portb);
    
    printf("\nFIFO status:\n");
    printf("RX FIFO count: %u\n", board_fifo_get_rx_count(fifo));
    printf("TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    free_board_fifo(fifo);
    printf("\n✓ Initialization test complete\n");
}

void test_cpu_write_to_usb(void) {
    print_test_header("CPU Write to USB via VIA");
    
    fifo_t *fifo = init_board_fifo();
    if (!fifo) return;
    
    // Configure VIA
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);  // Port A output
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);  // Port B: bits 0-1 output
    
    printf("\nCPU writing 'HELLO' to USB:\n");
    const char *msg = "HELLO";
    
    for (int i = 0; i < 5; i++) {
        printf("\nWriting '%c' (0x%02X):\n", msg[i], msg[i]);
        
        // 1. Write data to Port A
        printf("  1. Write data to Port A\n");
        board_fifo_write_via(fifo, VIA_ORA_IRA, msg[i]);
        
        // 2. Assert WR (set bit 1 high)
        printf("  2. Assert WR signal\n");
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N | PORTB_WR);
        
        // 3. Clock to process
        for (int j = 0; j < 10; j++) {
            board_fifo_clock(fifo);
        }
        
        // 4. Deassert WR
        printf("  3. Deassert WR signal\n");
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    }
    
    printf("\nVerifying data sent to USB:\n");
    uint8_t usb_buffer[256];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, usb_buffer, 256);
    printf("Received %u bytes from CPU\n", count);
    printf("Data: ");
    for (int i = 0; i < count; i++) {
        printf("%c", usb_buffer[i]);
    }
    printf("\n");
    
    free_board_fifo(fifo);
    printf("\n✓ CPU write test complete\n");
}

void test_usb_read_by_cpu(void) {
    print_test_header("CPU Read from USB via VIA");
    
    fifo_t *fifo = init_board_fifo();
    if (!fifo) return;
    
    // Configure VIA
    board_fifo_write_via(fifo, VIA_DDRA, 0x00);  // Port A input
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);  // Port B: bits 0-1 output
    
    printf("\nUSB sending 'WORLD' to CPU:\n");
    const char *msg = "WORLD";
    for (int i = 0; i < 5; i++) {
        board_fifo_usb_send_to_cpu(fifo, msg[i]);
        printf("  USB sent: '%c'\n", msg[i]);
    }
    
    printf("\nChecking Port B status (should show data available):\n");
    uint8_t portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    print_port_b_status(portb);
    
    printf("\nCPU reading data from USB:\n");
    for (int i = 0; i < 5; i++) {
        // 1. Assert RD# (clear bit 0)
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_WR & 0);  // RD# low
        
        // 2. Clock to process
        for (int j = 0; j < 10; j++) {
            board_fifo_clock(fifo);
        }
        
        // 3. Read data from Port A
        uint8_t data = board_fifo_read_via(fifo, VIA_ORA_IRA);
        printf("  Read: '%c' (0x%02X)\n", data, data);
        
        // 4. Deassert RD#
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    }
    
    printf("\nChecking Port B status (should show no data):\n");
    portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    print_port_b_status(portb);
    
    free_board_fifo(fifo);
    printf("\n✓ USB read test complete\n");
}

void test_bidirectional_transfer(void) {
    print_test_header("Bidirectional Data Transfer");
    
    fifo_t *fifo = init_board_fifo();
    if (!fifo) return;
    
    // Configure VIA - Port A bidirectional (will change direction as needed)
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);  // Port B: bits 0-1 output
    
    printf("\nStep 1: USB sends 'TEST' to CPU\n");
    const char *rx_msg = "TEST";
    for (int i = 0; i < 4; i++) {
        board_fifo_usb_send_to_cpu(fifo, rx_msg[i]);
    }
    printf("USB TX count: %u\n", board_fifo_get_rx_count(fifo));
    
    printf("\nStep 2: CPU sends 'ECHO' to USB\n");
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);  // Port A output for writing
    const char *tx_msg = "ECHO";
    for (int i = 0; i < 4; i++) {
        board_fifo_write_via(fifo, VIA_ORA_IRA, tx_msg[i]);
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N | PORTB_WR);
        for (int j = 0; j < 10; j++) board_fifo_clock(fifo);
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    }
    
    printf("\nStep 3: CPU reads data from USB\n");
    board_fifo_write_via(fifo, VIA_DDRA, 0x00);  // Port A input for reading
    printf("Reading: ");
    for (int i = 0; i < 4; i++) {
        board_fifo_write_via(fifo, VIA_ORB_IRB, 0);  // RD# low
        for (int j = 0; j < 10; j++) board_fifo_clock(fifo);
        uint8_t data = board_fifo_read_via(fifo, VIA_ORA_IRA);
        printf("%c", data);
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    }
    printf("\n");
    
    printf("\nStep 4: USB reads data from CPU\n");
    uint8_t usb_buffer[256];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, usb_buffer, 256);
    printf("USB received: ");
    for (int i = 0; i < count; i++) {
        printf("%c", usb_buffer[i]);
    }
    printf("\n");
    
    free_board_fifo(fifo);
    printf("\n✓ Bidirectional transfer test complete\n");
}

void test_status_polling(void) {
    print_test_header("Status Signal Polling");
    
    fifo_t *fifo = init_board_fifo();
    if (!fifo) return;
    
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);
    
    printf("\nInitial status (no data):\n");
    uint8_t portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    print_port_b_status(portb);
    
    printf("\nAdding data to RX FIFO...\n");
    board_fifo_usb_send_to_cpu(fifo, 0x42);
    
    printf("\nStatus after adding data:\n");
    portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    print_port_b_status(portb);
    
    printf("\nPolling loop example:\n");
    printf("while (!(PORTB & RXF#)) { /* RXF# low = data available */ }\n");
    if (!(portb & PORTB_RXF_N)) {
        printf("  -> Data available for reading!\n");
    }
    
    free_board_fifo(fifo);
    printf("\n✓ Status polling test complete\n");
}

void test_real_world_scenario(void) {
    print_test_header("Real-World Scenario: Command/Response");
    
    fifo_t *fifo = init_board_fifo();
    if (!fifo) return;
    
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);
    
    printf("\nScenario: PC sends command, CPU responds\n");
    printf("\n1. PC sends command 'READ' to CPU\n");
    const char *cmd = "READ";
    for (int i = 0; i < 4; i++) {
        board_fifo_usb_send_to_cpu(fifo, cmd[i]);
    }
    
    printf("\n2. CPU polls for data and reads command\n");
    board_fifo_write_via(fifo, VIA_DDRA, 0x00);  // Port A input
    char received_cmd[5] = {0};
    for (int i = 0; i < 4; i++) {
        // Poll RXF#
        uint8_t portb;
        do {
            portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
        } while (portb & PORTB_RXF_N);  // Wait while RXF# is high
        
        // Read byte
        board_fifo_write_via(fifo, VIA_ORB_IRB, 0);  // RD# low
        for (int j = 0; j < 10; j++) board_fifo_clock(fifo);
        received_cmd[i] = board_fifo_read_via(fifo, VIA_ORA_IRA);
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);  // RD# high
    }
    printf("CPU received command: %s\n", received_cmd);
    
    printf("\n3. CPU sends response 'OK!' to PC\n");
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);  // Port A output
    const char *response = "OK!";
    for (int i = 0; i < 3; i++) {
        // Write byte
        board_fifo_write_via(fifo, VIA_ORA_IRA, response[i]);
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N | PORTB_WR);
        for (int j = 0; j < 10; j++) board_fifo_clock(fifo);
        board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    }
    
    printf("\n4. PC reads response from CPU\n");
    uint8_t response_buf[256];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, response_buf, 256);
    printf("PC received response: ");
    for (int i = 0; i < count; i++) {
        printf("%c", response_buf[i]);
    }
    printf("\n");
    
    free_board_fifo(fifo);
    printf("\n✓ Real-world scenario test complete\n");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  Board FIFO Test Suite                        ║\n");
    printf("║  VIA 6522 connected to FT245 USB FIFO         ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_initialization();
    test_cpu_write_to_usb();
    test_usb_read_by_cpu();
    test_bidirectional_transfer();
    test_status_polling();
    test_real_world_scenario();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  All tests completed successfully!            ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}

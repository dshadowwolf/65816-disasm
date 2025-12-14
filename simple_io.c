/*
 * Simple IO Interface for Board FIFO - Implementation
 */

#include <stddef.h>
#include "simple_io.h"
#include "via6522.h"

// Check if data is available to read from USB
bool io_data_available(fifo_t *fifo) {
    uint8_t portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    return !(portb & PORTB_RXF_N);  // RXF# is active low
}

// Check if space is available to write to USB
bool io_space_available(fifo_t *fifo) {
    uint8_t portb = board_fifo_read_via(fifo, VIA_ORB_IRB);
    return !(portb & PORTB_TXE_N);  // TXE# is active low
}

// Read a byte from USB/FIFO (blocking)
uint8_t io_read_byte(fifo_t *fifo) {
    // Wait for data available
    while (!io_data_available(fifo)) {
        board_fifo_clock(fifo);
    }
    
    // Set Port A as input for reading
    board_fifo_write_via(fifo, VIA_DDRA, 0x00);
    
    // Assert RD# (clear bit 0)
    board_fifo_write_via(fifo, VIA_ORB_IRB, 0x00);
    
    // Clock to process read
    for (int i = 0; i < 5; i++) {
        board_fifo_clock(fifo);
    }
    
    // Read data from Port A
    uint8_t data = board_fifo_read_via(fifo, VIA_ORA_IRA);
    
    // Deassert RD# (set bit 0)
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    
    board_fifo_clock(fifo);
    
    // Set Port A back to output for writing
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);
    
    return data;
}

// Write a byte to USB/FIFO (blocking)
void io_write_byte(fifo_t *fifo, uint8_t data) {
    // Wait for space available
    while (!io_space_available(fifo)) {
        board_fifo_clock(fifo);
    }
    
    // Write data to Port A
    board_fifo_write_via(fifo, VIA_ORA_IRA, data);
    
    // Assert WR (set bit 1)
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N | PORTB_WR);
    
    // Clock to process write
    for (int i = 0; i < 5; i++) {
        board_fifo_clock(fifo);
    }
    
    // Deassert WR (clear bit 1)
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    
    board_fifo_clock(fifo);
}

// Write a string to USB/FIFO
void io_write_string(fifo_t *fifo, const char *str) {
    while (*str) {
        io_write_byte(fifo, *str++);
    }
}

// Initialize the IO system
fifo_t *io_init(void) {
    fifo_t *fifo = init_board_fifo();
    if (!fifo) {
        return NULL;
    }
    
    // Configure VIA for FIFO operation
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);  // Port A: all outputs (bidirectional via OE)
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);  // Port B: bits 0-1 output, 2-4 input
    
    // Initialize control signals
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);  // RD# high, WR low
    
    return fifo;
}

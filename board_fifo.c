#include "board_fifo.h"
#include "via6522.h"
#include "ft245.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// Port B bit assignments for FT245 control/status
#define PORTB_RD_N      0x01  // Bit 0: RD# output (active low)
#define PORTB_WR        0x02  // Bit 1: WR output (active high)
#define PORTB_RXF_N     0x04  // Bit 2: RXF# input (active low - data available)
#define PORTB_TXE_N     0x08  // Bit 3: TXE# input (active low - space available)
#define PORTB_PWREN_N   0x10  // Bit 4: PWREN# input (active low - USB configured)
// Bits 5-7: Reserved/unused

typedef struct fifo_s {
    ft245_t ft245;
    via6522_t via;
    uint8_t portb_outputs;  // Track current Port B output state
} fifo_t;

fifo_t *init_board_fifo(void) {
    fifo_t *fifo = (fifo_t *)malloc(sizeof(fifo_t));
    if (!fifo) {
        fprintf(stderr, "Failed to allocate memory for FIFO board\n");
        return NULL;
    }

    // Initialize FT245 and VIA
    ft245_init(&fifo->ft245);
    via6522_init(&fifo->via);
    
    // Initialize Port B outputs: RD# and WR both inactive (high)
    fifo->portb_outputs = PORTB_RD_N;  // RD# high (inactive), WR low (inactive)
    
    // Connect USB (simulate USB cable connected and enumerated)
    ft245_set_usb_connected(&fifo->ft245, true);
    ft245_set_usb_configured(&fifo->ft245, true);
    
    // Set up VIA Port A callbacks (data bus)
    via6522_set_port_a_callbacks(&fifo->via, 
                                  board_fifo_via_port_a_read,
                                  board_fifo_via_port_a_write,
                                  fifo);
    
    // Set up VIA Port B callbacks (control signals)
    via6522_set_port_b_callbacks(&fifo->via,
                                  board_fifo_via_port_b_read,
                                  board_fifo_via_port_b_write,
                                  fifo);

    return fifo;
}

void free_board_fifo(fifo_t *fifo) {
    if (fifo) {
        free(fifo);
    }
}

// Port A callbacks - FT245 Data Bus
// Reading Port A reads the current FT245 data bus value
uint8_t board_fifo_via_port_a_read(void* context) {
    fifo_t *fifo = (fifo_t *)context;
    // When RD# is asserted (low), perform actual read
    if (!(fifo->portb_outputs & PORTB_RD_N)) {
        return ft245_read(&fifo->ft245);
    }
    // Otherwise return current data bus state
    return ft245_get_data_bus(&fifo->ft245);
}

// Writing to Port A sets data on the FT245 data bus
// Actual write happens when WR is asserted via Port B
void board_fifo_via_port_a_write(void* context, uint8_t value) {
    fifo_t *fifo = (fifo_t *)context;
    // Always update the FT245 data bus
    // If WR is already asserted, this will trigger a write
    ft245_write(&fifo->ft245, value);
}

// Port B callbacks - FT245 Control/Status Signals
// Reading Port B returns FT245 status signals
uint8_t board_fifo_via_port_b_read(void* context) {
    fifo_t *fifo = (fifo_t *)context;
    uint8_t value = 0;
    
    // Read FT245 status signals (inputs to VIA)
    if (ft245_get_rxf(&fifo->ft245)) {
        value |= PORTB_RXF_N;  // RXF# high (no data)
    }
    
    if (ft245_get_txe(&fifo->ft245)) {
        value |= PORTB_TXE_N;  // TXE# high (no space)
    }
    
    if (ft245_get_pwren(&fifo->ft245)) {
        value |= PORTB_PWREN_N;  // PWREN# high (not configured)
    }
    
    // Preserve output bits (RD#, WR)
    value |= (fifo->portb_outputs & (PORTB_RD_N | PORTB_WR));
    
    return value;
}

// Writing to Port B controls FT245 RD# and WR signals
void board_fifo_via_port_b_write(void* context, uint8_t value) {
    fifo_t *fifo = (fifo_t *)context;
    uint8_t old_outputs = fifo->portb_outputs;
    fifo->portb_outputs = value;
    
    // Update FT245 RD# signal (active low)
    bool rd_n = (value & PORTB_RD_N) ? true : false;
    bool old_rd_n = (old_outputs & PORTB_RD_N) ? true : false;
    
    if (rd_n != old_rd_n) {
        // RD# state changed
        ft245_set_rd(&fifo->ft245, !rd_n);  // FT245 expects active high
    }
    
    // Update FT245 WR signal (active high)
    bool wr = (value & PORTB_WR) ? true : false;
    bool old_wr = (old_outputs & PORTB_WR) ? true : false;
    
    if (wr != old_wr) {
        // WR state changed
        ft245_set_wr(&fifo->ft245, wr);
    }
}
void board_fifo_clock(fifo_t *fifo) {
    if (!fifo) return;
    
    // Clock both devices
    ft245_clock(&fifo->ft245);
    via6522_clock(&fifo->via);
}

// Helper functions for testing/external use

// USB side: Send data from PC/USB to CPU (appears in FT245 RX FIFO)
bool board_fifo_usb_send_to_cpu(fifo_t *fifo, uint8_t data) {
    if (!fifo) return false;
    return ft245_usb_receive(&fifo->ft245, data);
}

// USB side: Receive data from CPU to PC/USB (from FT245 TX FIFO)
bool board_fifo_usb_receive_from_cpu(fifo_t *fifo, uint8_t *data) {
    if (!fifo) return false;
    return ft245_usb_transmit(&fifo->ft245, data);
}

// USB side: Send buffer from PC/USB to CPU
uint16_t board_fifo_usb_send_buffer(fifo_t *fifo, const uint8_t *buffer, uint16_t length) {
    if (!fifo) return 0;
    return ft245_usb_receive_buffer(&fifo->ft245, buffer, length);
}

// USB side: Receive buffer from CPU to PC/USB
uint16_t board_fifo_usb_receive_buffer(fifo_t *fifo, uint8_t *buffer, uint16_t max_length) {
    if (!fifo) return 0;
    return ft245_usb_transmit_buffer(&fifo->ft245, buffer, max_length);
}

// Get VIA register address for CPU access
uint8_t board_fifo_read_via(fifo_t *fifo, uint8_t reg) {
    if (!fifo) return 0xFF;
    return via6522_read(&fifo->via, reg);
}

// Write to VIA register for CPU access
void board_fifo_write_via(fifo_t *fifo, uint8_t reg, uint8_t value) {
    if (!fifo) return;
    via6522_write(&fifo->via, reg, value);
}

// Get FIFO status
uint16_t board_fifo_get_rx_count(fifo_t *fifo) {
    if (!fifo) return 0;
    return ft245_get_rx_fifo_count(&fifo->ft245);
}

uint16_t board_fifo_get_tx_count(fifo_t *fifo) {
    if (!fifo) return 0;
    return ft245_get_tx_fifo_count(&fifo->ft245);
}

// Get VIA instance for interrupt checking
via6522_t* board_fifo_get_via(fifo_t *fifo) {
    if (!fifo) return NULL;
    return &fifo->via;
}

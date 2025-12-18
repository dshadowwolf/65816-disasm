#ifndef __BOARD_FIFO_H__
#define __BOARD_FIFO_H__

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct fifo_s fifo_t;
typedef struct via6522_s via6522_t;

// Port B bit assignments for FT245 control/status
#define PORTB_RD_N      0x01  // Bit 0: RD# output (active low)
#define PORTB_WR        0x02  // Bit 1: WR output (active high)
#define PORTB_RXF_N     0x04  // Bit 2: RXF# input (active low - data available)
#define PORTB_TXE_N     0x08  // Bit 3: TXE# input (active low - space available)
#define PORTB_PWREN_N   0x10  // Bit 4: PWREN# input (active low - USB configured)

// Initialize the board FIFO (VIA connected to FT245)
fifo_t *init_board_fifo(void);

// Free the board FIFO
void free_board_fifo(fifo_t *fifo);

// Clock the board (updates both VIA and FT245)
void board_fifo_clock(fifo_t *fifo);

// USB side operations (simulating PC/USB host)
// Send data from USB/PC to CPU (adds to FT245 RX FIFO)
bool board_fifo_usb_send_to_cpu(fifo_t *fifo, uint8_t data);

// Receive data from CPU to USB/PC (reads from FT245 TX FIFO)
bool board_fifo_usb_receive_from_cpu(fifo_t *fifo, uint8_t *data);

// Send buffer from USB/PC to CPU
uint16_t board_fifo_usb_send_buffer(fifo_t *fifo, const uint8_t *buffer, uint16_t length);

// Receive buffer from CPU to USB/PC
uint16_t board_fifo_usb_receive_buffer(fifo_t *fifo, uint8_t *buffer, uint16_t max_length);

// CPU side operations (VIA register access)
// Read from VIA register
uint8_t board_fifo_read_via(fifo_t *fifo, uint8_t reg);

// Write to VIA register
void board_fifo_write_via(fifo_t *fifo, uint8_t reg, uint8_t value);

// Status queries
uint16_t board_fifo_get_rx_count(fifo_t *fifo);  // Bytes available to read from USB
uint16_t board_fifo_get_tx_count(fifo_t *fifo);  // Bytes waiting to send to USB

// Get VIA instance (for interrupt checking)
via6522_t* board_fifo_get_via(fifo_t *fifo);

// Port callbacks (internal use)
uint8_t board_fifo_via_port_a_read(void* context);
void board_fifo_via_port_a_write(void* context, uint8_t value);
uint8_t board_fifo_via_port_b_read(void* context);
void board_fifo_via_port_b_write(void* context, uint8_t value);

#endif // __BOARD_FIFO_H__

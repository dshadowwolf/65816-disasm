#ifndef __FT245_H__
#define __FT245_H__

#include <stdint.h>
#include <stdbool.h>

// FT245 operates with 8-bit data bus and control signals
// Control signals (active low unless noted):
// - RXF# (Read): Data available to read from FTDI to CPU
// - TXE# (Write): Transmit buffer has space available
// - RD# (input): Read strobe from CPU
// - WR (input): Write strobe from CPU (active high in async mode)
// - PWREN# (output): USB configured and powered

// FIFO sizes
#define FT245_RX_FIFO_SIZE 512  // USB->CPU buffer
#define FT245_TX_FIFO_SIZE 512  // CPU->USB buffer

// Status flags
#define FT245_STATUS_RXF    0x01  // Data available to read (active low in hardware)
#define FT245_STATUS_TXE    0x02  // Transmit buffer empty (active low in hardware)
#define FT245_STATUS_PWREN  0x04  // USB powered and configured

typedef struct ft245_s {
    // Data bus
    uint8_t data_bus;          // Current value on 8-bit data bus
    
    // Control signals (internal state)
    bool rxf_n;                // RXF# - Low when data available to read
    bool txe_n;                // TXE# - Low when transmit buffer has space
    bool rd_n;                 // RD# - Read strobe (input from CPU)
    bool wr;                   // WR - Write strobe (input from CPU)
    bool pwren_n;              // PWREN# - USB configured (output)
    
    // Receive FIFO (USB -> CPU)
    uint8_t rx_fifo[FT245_RX_FIFO_SIZE];
    uint16_t rx_fifo_head;     // Write position
    uint16_t rx_fifo_tail;     // Read position
    uint16_t rx_fifo_count;    // Number of bytes in FIFO
    
    // Transmit FIFO (CPU -> USB)
    uint8_t tx_fifo[FT245_TX_FIFO_SIZE];
    uint16_t tx_fifo_head;     // Write position
    uint16_t tx_fifo_tail;     // Read position
    uint16_t tx_fifo_count;    // Number of bytes in FIFO
    
    // USB connection state
    bool usb_connected;        // USB cable connected
    bool usb_configured;       // USB enumeration complete
    
    // Timing
    uint8_t read_latency;      // Cycles of latency for read operations
    uint8_t write_latency;     // Cycles of latency for write operations
    uint8_t read_timer;        // Current read timer
    uint8_t write_timer;       // Current write timer
    
    // Callbacks for USB side (PC side)
    // Called when CPU writes data (for transmission to USB/PC)
    void (*usb_tx_callback)(void* context, uint8_t byte);
    
    // Called to check if USB has data to send to CPU
    uint8_t (*usb_rx_callback)(void* context, bool* available);
    
    void* usb_context;
    
    // Status change callback
    void (*status_callback)(void* context, bool rxf, bool txe);
    void* status_context;
} ft245_t;

// Initialize FT245
void ft245_init(ft245_t* ft245);

// Reset FT245 to power-on state
void ft245_reset(ft245_t* ft245);

// Read data from FT245 (CPU reads from USB)
uint8_t ft245_read(ft245_t* ft245);

// Write data to FT245 (CPU writes to USB)
void ft245_write(ft245_t* ft245, uint8_t data);

// Control signal inputs (from CPU)
void ft245_set_rd(ft245_t* ft245, bool state);  // RD# pin
void ft245_set_wr(ft245_t* ft245, bool state);  // WR pin

// Control signal outputs (to CPU)
bool ft245_get_rxf(ft245_t* ft245);  // Returns RXF# state (low = data available)
bool ft245_get_txe(ft245_t* ft245);  // Returns TXE# state (low = space available)
bool ft245_get_pwren(ft245_t* ft245); // Returns PWREN# state

// Get current data bus value
uint8_t ft245_get_data_bus(ft245_t* ft245);

// USB side operations (simulating PC/USB host)
// Add data to RX FIFO (from USB/PC to CPU)
bool ft245_usb_receive(ft245_t* ft245, uint8_t data);

// Get data from TX FIFO (from CPU to USB/PC)
bool ft245_usb_transmit(ft245_t* ft245, uint8_t* data);

// Add multiple bytes to RX FIFO
uint16_t ft245_usb_receive_buffer(ft245_t* ft245, const uint8_t* buffer, uint16_t length);

// Get multiple bytes from TX FIFO
uint16_t ft245_usb_transmit_buffer(ft245_t* ft245, uint8_t* buffer, uint16_t max_length);

// USB connection control
void ft245_set_usb_connected(ft245_t* ft245, bool connected);
void ft245_set_usb_configured(ft245_t* ft245, bool configured);

// Get FIFO status
uint16_t ft245_get_rx_fifo_count(ft245_t* ft245);
uint16_t ft245_get_tx_fifo_count(ft245_t* ft245);
uint16_t ft245_get_rx_fifo_free(ft245_t* ft245);
uint16_t ft245_get_tx_fifo_free(ft245_t* ft245);

// Clock the FT245 (for timing-accurate simulation)
void ft245_clock(ft245_t* ft245);

// Set callbacks
void ft245_set_usb_callbacks(ft245_t* ft245,
                              void (*tx_fn)(void*, uint8_t),
                              uint8_t (*rx_fn)(void*, bool*),
                              void* context);

void ft245_set_status_callback(ft245_t* ft245,
                                void (*status_fn)(void*, bool, bool),
                                void* context);

#endif // __FT245_H__

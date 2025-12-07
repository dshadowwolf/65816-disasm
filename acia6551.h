#ifndef __ACIA6551_H__
#define __ACIA6551_H__

#include <stdint.h>
#include <stdbool.h>

// 6551 ACIA Register offsets
#define ACIA_DATA       0x00  // Transmit/Receive Data Register
#define ACIA_STATUS     0x01  // Status Register (read)
#define ACIA_RESET      0x01  // Programmed Reset (write)
#define ACIA_COMMAND    0x02  // Command Register
#define ACIA_CONTROL    0x03  // Control Register

// Status Register bits (read from STATUS register)
#define ACIA_STATUS_PARITY_ERR  0x01  // Parity error
#define ACIA_STATUS_FRAMING_ERR 0x02  // Framing error
#define ACIA_STATUS_OVERRUN     0x04  // Receiver overrun
#define ACIA_STATUS_RDRF        0x08  // Receive Data Register Full
#define ACIA_STATUS_TDRE        0x10  // Transmit Data Register Empty
#define ACIA_STATUS_DCD         0x20  // Data Carrier Detect (inverted)
#define ACIA_STATUS_DSR         0x40  // Data Set Ready (inverted)
#define ACIA_STATUS_IRQ         0x80  // Interrupt Request

// Command Register bits
#define ACIA_CMD_DTR_MASK       0x01  // DTR control
#define ACIA_CMD_DTR_ENABLE     0x01  // DTR enabled (active low output)
#define ACIA_CMD_DTR_DISABLE    0x00  // DTR disabled (high output)

#define ACIA_CMD_IRQ_MASK       0x0E  // Interrupt request bits
#define ACIA_CMD_IRQ_RX_ENABLE  0x02  // IRQ on receive data register full
#define ACIA_CMD_IRQ_TX_ENABLE  0x04  // IRQ on transmit data register empty
#define ACIA_CMD_IRQ_DISABLED   0x00  // IRQ disabled
#define ACIA_CMD_IRQ_RX_BRK     0x0E  // IRQ on receive, break on transmit

#define ACIA_CMD_ECHO_MODE      0x10  // Receiver echo mode
#define ACIA_CMD_PARITY_MASK    0x60  // Parity control mask
#define ACIA_CMD_PARITY_ODD     0x00  // Odd parity (parity enable)
#define ACIA_CMD_PARITY_EVEN    0x20  // Even parity (parity enable)
#define ACIA_CMD_PARITY_NONE    0x40  // No parity (mark bit)
#define ACIA_CMD_PARITY_NONE2   0x60  // No parity (space bit)

// Control Register bits
#define ACIA_CTRL_BAUD_MASK     0x0F  // Baud rate selection
#define ACIA_CTRL_BAUD_16X_EXT  0x00  // External 16x clock
#define ACIA_CTRL_BAUD_50       0x01  // 50 baud
#define ACIA_CTRL_BAUD_75       0x02  // 75 baud
#define ACIA_CTRL_BAUD_110      0x03  // 109.92 baud
#define ACIA_CTRL_BAUD_135      0x04  // 134.58 baud
#define ACIA_CTRL_BAUD_150      0x05  // 150 baud
#define ACIA_CTRL_BAUD_300      0x06  // 300 baud
#define ACIA_CTRL_BAUD_600      0x07  // 600 baud
#define ACIA_CTRL_BAUD_1200     0x08  // 1200 baud
#define ACIA_CTRL_BAUD_1800     0x09  // 1800 baud
#define ACIA_CTRL_BAUD_2400     0x0A  // 2400 baud
#define ACIA_CTRL_BAUD_3600     0x0B  // 3600 baud
#define ACIA_CTRL_BAUD_4800     0x0C  // 4800 baud
#define ACIA_CTRL_BAUD_7200     0x0D  // 7200 baud
#define ACIA_CTRL_BAUD_9600     0x0E  // 9600 baud
#define ACIA_CTRL_BAUD_19200    0x0F  // 19200 baud

#define ACIA_CTRL_RECV_CLK      0x10  // Receiver clock source (0=ext, 1=baud gen)

#define ACIA_CTRL_WORD_MASK     0x60  // Word length mask
#define ACIA_CTRL_WORD_8BIT     0x00  // 8 bits
#define ACIA_CTRL_WORD_7BIT     0x20  // 7 bits
#define ACIA_CTRL_WORD_6BIT     0x40  // 6 bits
#define ACIA_CTRL_WORD_5BIT     0x60  // 5 bits

#define ACIA_CTRL_STOP_BITS     0x80  // Stop bits (0=1 stop, 1=1.5/2 stop)

// FIFO sizes
#define ACIA_RX_FIFO_SIZE 256
#define ACIA_TX_FIFO_SIZE 256

typedef struct acia6551_s {
    // Registers
    uint8_t data_rx;        // Receive data register
    uint8_t data_tx;        // Transmit data register
    uint8_t status;         // Status register
    uint8_t command;        // Command register
    uint8_t control;        // Control register
    
    // Receive FIFO
    uint8_t rx_fifo[ACIA_RX_FIFO_SIZE];
    uint16_t rx_fifo_head;  // Write position
    uint16_t rx_fifo_tail;  // Read position
    uint16_t rx_fifo_count; // Number of bytes in FIFO
    
    // Transmit FIFO
    uint8_t tx_fifo[ACIA_TX_FIFO_SIZE];
    uint16_t tx_fifo_head;
    uint16_t tx_fifo_tail;
    uint16_t tx_fifo_count;
    
    // Error flags
    bool parity_error;
    bool framing_error;
    bool overrun_error;
    
    // Control line states
    bool dtr;               // Data Terminal Ready (output)
    bool rts;               // Request To Send (output, not used in 6551)
    bool dcd;               // Data Carrier Detect (input)
    bool dsr;               // Data Set Ready (input)
    bool cts;               // Clear To Send (input, not used in 6551)
    
    // Timing
    uint32_t tx_clock_divider;  // Clock divider for transmit
    uint32_t rx_clock_divider;  // Clock divider for receive
    uint32_t tx_clock_counter;  // Current transmit clock count
    uint32_t rx_clock_counter;  // Current receive clock count
    
    // Shift registers
    uint16_t tx_shift_reg;      // Transmit shift register
    uint8_t tx_bits_remaining;  // Bits left to transmit
    uint16_t rx_shift_reg;      // Receive shift register
    uint8_t rx_bits_remaining;  // Bits left to receive
    
    // Serial I/O callbacks
    void (*tx_callback)(void* context, uint8_t bit);  // Called for each transmitted bit
    uint8_t (*rx_callback)(void* context);            // Called to get received bit
    void* serial_context;
    
    // Byte-level callbacks (higher level)
    void (*tx_byte_callback)(void* context, uint8_t byte);
    uint8_t (*rx_byte_callback)(void* context, bool* available);
    void* byte_context;
    
    // IRQ callback
    void (*irq_callback)(void* context, bool state);
    void* irq_context;
    
    // Control line callbacks
    void (*dtr_callback)(void* context, bool state);
    void* dtr_context;
} acia6551_t;

// Initialize ACIA
void acia6551_init(acia6551_t* acia);

// Reset ACIA to power-on state
void acia6551_reset(acia6551_t* acia);

// Register access
uint8_t acia6551_read(acia6551_t* acia, uint8_t reg);
void acia6551_write(acia6551_t* acia, uint8_t reg, uint8_t value);

// Clock the ACIA (call regularly to handle serial timing)
void acia6551_clock(acia6551_t* acia, uint32_t cycles);

// Control line inputs
void acia6551_set_dcd(acia6551_t* acia, bool state);
void acia6551_set_dsr(acia6551_t* acia, bool state);
void acia6551_set_cts(acia6551_t* acia, bool state);

// Get control line outputs
bool acia6551_get_dtr(acia6551_t* acia);
bool acia6551_get_rts(acia6551_t* acia);

// Get IRQ state
bool acia6551_get_irq(acia6551_t* acia);

// High-level byte interface (easier to use than bit-level)
void acia6551_receive_byte(acia6551_t* acia, uint8_t byte);
bool acia6551_transmit_byte_available(acia6551_t* acia, uint8_t* byte);

// Set callbacks
void acia6551_set_irq_callback(acia6551_t* acia,
                                void (*irq_fn)(void*, bool),
                                void* context);

void acia6551_set_dtr_callback(acia6551_t* acia,
                                void (*dtr_fn)(void*, bool),
                                void* context);

void acia6551_set_byte_callbacks(acia6551_t* acia,
                                  void (*tx_fn)(void*, uint8_t),
                                  uint8_t (*rx_fn)(void*, bool*),
                                  void* context);

// Get current baud rate (returns 0 if external clock)
uint32_t acia6551_get_baud_rate(acia6551_t* acia);

// Get word length in bits
uint8_t acia6551_get_word_length(acia6551_t* acia);

#endif // __ACIA6551_H__

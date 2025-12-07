#include "acia6551.h"
#include <string.h>

static void update_irq(acia6551_t* acia);
static void update_status(acia6551_t* acia);
static void start_transmit(acia6551_t* acia);
static uint32_t get_clock_divider(uint8_t baud_rate);

void acia6551_init(acia6551_t* acia) {
    memset(acia, 0, sizeof(acia6551_t));
    acia6551_reset(acia);
}

void acia6551_reset(acia6551_t* acia) {
    acia->data_rx = 0;
    acia->data_tx = 0;
    acia->status = ACIA_STATUS_TDRE;  // Transmit register empty on reset
    acia->command = 0;
    acia->control = 0;
    
    acia->rx_fifo_head = 0;
    acia->rx_fifo_tail = 0;
    acia->rx_fifo_count = 0;
    
    acia->tx_fifo_head = 0;
    acia->tx_fifo_tail = 0;
    acia->tx_fifo_count = 0;
    
    acia->parity_error = false;
    acia->framing_error = false;
    acia->overrun_error = false;
    
    acia->dtr = false;
    acia->rts = false;
    acia->dcd = true;  // Inverted: true = not connected
    acia->dsr = true;  // Inverted: true = not ready
    acia->cts = true;  // Not used but default high
    
    acia->tx_clock_divider = 1;
    acia->rx_clock_divider = 1;
    acia->tx_clock_counter = 0;
    acia->rx_clock_counter = 0;
    
    acia->tx_shift_reg = 0;
    acia->tx_bits_remaining = 0;
    acia->rx_shift_reg = 0;
    acia->rx_bits_remaining = 0;
}

uint8_t acia6551_read(acia6551_t* acia, uint8_t reg) {
    uint8_t value = 0;
    
    reg &= 0x03;  // Only 4 registers
    
    switch (reg) {
        case ACIA_DATA:
            // Read received data
            if (acia->rx_fifo_count > 0) {
                value = acia->rx_fifo[acia->rx_fifo_tail];
                acia->rx_fifo_tail = (acia->rx_fifo_tail + 1) % ACIA_RX_FIFO_SIZE;
                acia->rx_fifo_count--;
                
                // Update data register
                if (acia->rx_fifo_count > 0) {
                    acia->data_rx = acia->rx_fifo[acia->rx_fifo_tail];
                } else {
                    acia->data_rx = 0;
                }
            } else {
                value = acia->data_rx;
            }
            
            // Clear RDRF flag
            acia->status &= ~ACIA_STATUS_RDRF;
            
            // Clear error flags on data read
            acia->parity_error = false;
            acia->framing_error = false;
            acia->overrun_error = false;
            
            update_status(acia);
            update_irq(acia);
            break;
            
        case ACIA_STATUS:
            update_status(acia);
            value = acia->status;
            break;
            
        case ACIA_COMMAND:
            value = acia->command;
            break;
            
        case ACIA_CONTROL:
            value = acia->control;
            break;
    }
    
    return value;
}

void acia6551_write(acia6551_t* acia, uint8_t reg, uint8_t value) {
    reg &= 0x03;
    
    switch (reg) {
        case ACIA_DATA:
            // Write transmit data
            acia->data_tx = value;
            
            // Add to transmit FIFO
            if (acia->tx_fifo_count < ACIA_TX_FIFO_SIZE) {
                acia->tx_fifo[acia->tx_fifo_head] = value;
                acia->tx_fifo_head = (acia->tx_fifo_head + 1) % ACIA_TX_FIFO_SIZE;
                acia->tx_fifo_count++;
            }
            
            // Clear TDRE flag
            acia->status &= ~ACIA_STATUS_TDRE;
            
            // Start transmission if not already transmitting
            if (acia->tx_bits_remaining == 0) {
                start_transmit(acia);
            }
            
            update_irq(acia);
            break;
            
        case ACIA_RESET:
            // Programmed reset
            acia6551_reset(acia);
            break;
            
        case ACIA_COMMAND: {
            acia->command = value;
            
            // Update DTR
            bool new_dtr = (value & ACIA_CMD_DTR_ENABLE) ? true : false;
            if (new_dtr != acia->dtr) {
                acia->dtr = new_dtr;
                if (acia->dtr_callback) {
                    acia->dtr_callback(acia->dtr_context, acia->dtr);
                }
            }
            
            update_irq(acia);
            break;
        }
            
        case ACIA_CONTROL:
            acia->control = value;
            
            // Update clock dividers based on baud rate
            uint8_t baud_sel = value & ACIA_CTRL_BAUD_MASK;
            acia->tx_clock_divider = get_clock_divider(baud_sel);
            
            if (value & ACIA_CTRL_RECV_CLK) {
                // Use baud rate generator for receive
                acia->rx_clock_divider = acia->tx_clock_divider;
            } else {
                // Use external clock for receive
                acia->rx_clock_divider = 16;  // 16x external clock
            }
            break;
    }
}

void acia6551_clock(acia6551_t* acia, uint32_t cycles) {
    // This is a simplified clock implementation
    // In a real implementation, you'd handle bit-level timing
    
    // For now, we'll just handle byte-level transmission
    for (uint32_t i = 0; i < cycles; i++) {
        // Transmit side
        if (acia->tx_bits_remaining > 0) {
            acia->tx_clock_counter++;
            if (acia->tx_clock_counter >= acia->tx_clock_divider) {
                acia->tx_clock_counter = 0;
                acia->tx_bits_remaining--;
                
                if (acia->tx_bits_remaining == 0) {
                    // Byte transmission complete
                    if (acia->tx_fifo_count > 0) {
                        // More data to send
                        start_transmit(acia);
                    } else {
                        // Transmit buffer empty
                        acia->status |= ACIA_STATUS_TDRE;
                        update_irq(acia);
                    }
                }
            }
        }
        
        // Receive side - check for incoming data via callback
        if (acia->rx_byte_callback) {
            bool available = false;
            uint8_t byte = acia->rx_byte_callback(acia->byte_context, &available);
            if (available) {
                acia6551_receive_byte(acia, byte);
            }
        }
    }
}

void acia6551_set_dcd(acia6551_t* acia, bool state) {
    acia->dcd = !state;  // Inverted logic
    update_status(acia);
}

void acia6551_set_dsr(acia6551_t* acia, bool state) {
    acia->dsr = !state;  // Inverted logic
    update_status(acia);
}

void acia6551_set_cts(acia6551_t* acia, bool state) {
    acia->cts = !state;
}

bool acia6551_get_dtr(acia6551_t* acia) {
    return acia->dtr;
}

bool acia6551_get_rts(acia6551_t* acia) {
    return acia->rts;
}

bool acia6551_get_irq(acia6551_t* acia) {
    return (acia->status & ACIA_STATUS_IRQ) != 0;
}

void acia6551_receive_byte(acia6551_t* acia, uint8_t byte) {
    // Add to receive FIFO
    if (acia->rx_fifo_count < ACIA_RX_FIFO_SIZE) {
        acia->rx_fifo[acia->rx_fifo_head] = byte;
        acia->rx_fifo_head = (acia->rx_fifo_head + 1) % ACIA_RX_FIFO_SIZE;
        acia->rx_fifo_count++;
        
        // Update data register
        acia->data_rx = acia->rx_fifo[acia->rx_fifo_tail];
        
        // Set RDRF flag
        acia->status |= ACIA_STATUS_RDRF;
        update_irq(acia);
    } else {
        // Buffer overflow
        acia->overrun_error = true;
        update_status(acia);
    }
}

bool acia6551_transmit_byte_available(acia6551_t* acia, uint8_t* byte) {
    if (acia->tx_fifo_count > 0) {
        *byte = acia->tx_fifo[acia->tx_fifo_tail];
        acia->tx_fifo_tail = (acia->tx_fifo_tail + 1) % ACIA_TX_FIFO_SIZE;
        acia->tx_fifo_count--;
        
        if (acia->tx_fifo_count == 0) {
            acia->status |= ACIA_STATUS_TDRE;
            update_irq(acia);
        }
        
        return true;
    }
    return false;
}

void acia6551_set_irq_callback(acia6551_t* acia,
                                void (*irq_fn)(void*, bool),
                                void* context) {
    acia->irq_callback = irq_fn;
    acia->irq_context = context;
}

void acia6551_set_dtr_callback(acia6551_t* acia,
                                void (*dtr_fn)(void*, bool),
                                void* context) {
    acia->dtr_callback = dtr_fn;
    acia->dtr_context = context;
}

void acia6551_set_byte_callbacks(acia6551_t* acia,
                                  void (*tx_fn)(void*, uint8_t),
                                  uint8_t (*rx_fn)(void*, bool*),
                                  void* context) {
    acia->tx_byte_callback = tx_fn;
    acia->rx_byte_callback = rx_fn;
    acia->byte_context = context;
}

uint32_t acia6551_get_baud_rate(acia6551_t* acia) {
    static const uint32_t baud_rates[] = {
        0,      // External clock
        50, 75, 110, 135, 150, 300, 600,
        1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200
    };
    
    uint8_t baud_sel = acia->control & ACIA_CTRL_BAUD_MASK;
    return baud_rates[baud_sel];
}

uint8_t acia6551_get_word_length(acia6551_t* acia) {
    switch (acia->control & ACIA_CTRL_WORD_MASK) {
        case ACIA_CTRL_WORD_8BIT: return 8;
        case ACIA_CTRL_WORD_7BIT: return 7;
        case ACIA_CTRL_WORD_6BIT: return 6;
        case ACIA_CTRL_WORD_5BIT: return 5;
        default: return 8;
    }
}

// Internal helper functions

static void update_irq(acia6551_t* acia) {
    bool irq_active = false;
    uint8_t irq_mode = acia->command & ACIA_CMD_IRQ_MASK;
    
    // Check for receive interrupt
    if ((irq_mode == ACIA_CMD_IRQ_RX_ENABLE || irq_mode == ACIA_CMD_IRQ_RX_BRK) &&
        (acia->status & ACIA_STATUS_RDRF)) {
        irq_active = true;
    }
    
    // Check for transmit interrupt
    if ((irq_mode == ACIA_CMD_IRQ_TX_ENABLE) &&
        (acia->status & ACIA_STATUS_TDRE)) {
        irq_active = true;
    }
    
    // Update IRQ bit in status
    if (irq_active) {
        acia->status |= ACIA_STATUS_IRQ;
    } else {
        acia->status &= ~ACIA_STATUS_IRQ;
    }
    
    // Call IRQ callback
    if (acia->irq_callback) {
        acia->irq_callback(acia->irq_context, irq_active);
    }
}

static void update_status(acia6551_t* acia) {
    // Update error flags
    if (acia->parity_error) {
        acia->status |= ACIA_STATUS_PARITY_ERR;
    } else {
        acia->status &= ~ACIA_STATUS_PARITY_ERR;
    }
    
    if (acia->framing_error) {
        acia->status |= ACIA_STATUS_FRAMING_ERR;
    } else {
        acia->status &= ~ACIA_STATUS_FRAMING_ERR;
    }
    
    if (acia->overrun_error) {
        acia->status |= ACIA_STATUS_OVERRUN;
    } else {
        acia->status &= ~ACIA_STATUS_OVERRUN;
    }
    
    // Update control line status (inverted logic)
    if (acia->dcd) {
        acia->status |= ACIA_STATUS_DCD;
    } else {
        acia->status &= ~ACIA_STATUS_DCD;
    }
    
    if (acia->dsr) {
        acia->status |= ACIA_STATUS_DSR;
    } else {
        acia->status &= ~ACIA_STATUS_DSR;
    }
}

static void start_transmit(acia6551_t* acia) {
    if (acia->tx_fifo_count > 0) {
        uint8_t byte = acia->tx_fifo[acia->tx_fifo_tail];
        acia->tx_fifo_tail = (acia->tx_fifo_tail + 1) % ACIA_TX_FIFO_SIZE;
        acia->tx_fifo_count--;
        
        // Call byte-level callback if available
        if (acia->tx_byte_callback) {
            acia->tx_byte_callback(acia->byte_context, byte);
        }
        
        // Set up for bit-level transmission
        uint8_t word_length = acia6551_get_word_length(acia);
        acia->tx_shift_reg = byte;
        acia->tx_bits_remaining = word_length + 2;  // data + start + stop
        acia->tx_clock_counter = 0;
        
        // If FIFO is now empty, set TDRE
        if (acia->tx_fifo_count == 0) {
            acia->status |= ACIA_STATUS_TDRE;
        }
    }
}

static uint32_t get_clock_divider(uint8_t baud_rate) {
    // Simplified clock divider calculation
    // In a real implementation, this would be based on the master clock frequency
    // For now, return arbitrary values that represent relative timing
    static const uint32_t dividers[] = {
        16,      // External 16x
        38400,   // 50 baud
        25600,   // 75 baud
        17455,   // 110 baud
        14245,   // 135 baud
        12800,   // 150 baud
        6400,    // 300 baud
        3200,    // 600 baud
        1600,    // 1200 baud
        1067,    // 1800 baud
        800,     // 2400 baud
        533,     // 3600 baud
        400,     // 4800 baud
        267,     // 7200 baud
        200,     // 9600 baud
        100      // 19200 baud
    };
    
    if (baud_rate > 15) baud_rate = 0;
    return dividers[baud_rate];
}

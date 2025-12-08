#include "ft245.h"
#include <string.h>

static void update_status_signals(ft245_t* ft245);

void ft245_init(ft245_t* ft245) {
    memset(ft245, 0, sizeof(ft245_t));
    ft245_reset(ft245);
}

void ft245_reset(ft245_t* ft245) {
    ft245->data_bus = 0xFF;  // Bus idle state
    
    // Control signals (inactive state)
    ft245->rxf_n = true;     // No data available
    ft245->txe_n = false;    // Transmit buffer has space
    ft245->rd_n = true;      // Not reading
    ft245->wr = false;       // Not writing
    ft245->pwren_n = true;   // Not powered/configured
    
    // Clear FIFOs
    ft245->rx_fifo_head = 0;
    ft245->rx_fifo_tail = 0;
    ft245->rx_fifo_count = 0;
    
    ft245->tx_fifo_head = 0;
    ft245->tx_fifo_tail = 0;
    ft245->tx_fifo_count = 0;
    
    // USB state
    ft245->usb_connected = false;
    ft245->usb_configured = false;
    
    // Timing (typical values for FT245R)
    ft245->read_latency = 2;   // ~50ns @ 40MHz = ~2 cycles
    ft245->write_latency = 2;
    ft245->read_timer = 0;
    ft245->write_timer = 0;
    
    update_status_signals(ft245);
}

uint8_t ft245_read(ft245_t* ft245) {
    uint8_t data = 0xFF;
    
    // Check if RD# is asserted (low) and data is available
    if (!ft245->rd_n && ft245->rx_fifo_count > 0) {
        if (ft245->read_timer >= ft245->read_latency) {
            // Read data from RX FIFO
            data = ft245->rx_fifo[ft245->rx_fifo_tail];
            ft245->rx_fifo_tail = (ft245->rx_fifo_tail + 1) % FT245_RX_FIFO_SIZE;
            ft245->rx_fifo_count--;
            
            ft245->data_bus = data;
            ft245->read_timer = 0;
            
            update_status_signals(ft245);
        } else {
            // Return current data bus value during latency period
            data = ft245->data_bus;
        }
    } else {
        data = ft245->data_bus;
    }
    
    return data;
}

void ft245_write(ft245_t* ft245, uint8_t data) {
    // Just store data on bus - actual write triggered by WR signal
    ft245->data_bus = data;
}

void ft245_set_rd(ft245_t* ft245, bool state) {
    bool old_state = ft245->rd_n;
    ft245->rd_n = !state;  // Active low
    
    // On falling edge, start read operation
    if (old_state && !ft245->rd_n) {
        ft245->read_timer = 0;
    }
}

void ft245_set_wr(ft245_t* ft245, bool state) {
    bool old_state = ft245->wr;
    ft245->wr = state;  // Active high in async mode
    
    // On rising edge, perform write operation if data is on bus
    if (!old_state && ft245->wr) {
        // Check if buffer has space
        if (ft245->tx_fifo_count < FT245_TX_FIFO_SIZE) {
            // Write data from data bus to TX FIFO
            ft245->tx_fifo[ft245->tx_fifo_head] = ft245->data_bus;
            ft245->tx_fifo_head = (ft245->tx_fifo_head + 1) % FT245_TX_FIFO_SIZE;
            ft245->tx_fifo_count++;
            
            // Call USB transmit callback if available
            if (ft245->usb_tx_callback) {
                ft245->usb_tx_callback(ft245->usb_context, ft245->data_bus);
            }
            
            update_status_signals(ft245);
        }
        ft245->write_timer = 0;
    }
}

bool ft245_get_rxf(ft245_t* ft245) {
    return ft245->rxf_n;  // Active low - false means data available
}

bool ft245_get_txe(ft245_t* ft245) {
    return ft245->txe_n;  // Active low - false means space available
}

bool ft245_get_pwren(ft245_t* ft245) {
    return ft245->pwren_n;  // Active low - false means USB configured
}

uint8_t ft245_get_data_bus(ft245_t* ft245) {
    return ft245->data_bus;
}

bool ft245_usb_receive(ft245_t* ft245, uint8_t data) {
    if (ft245->rx_fifo_count < FT245_RX_FIFO_SIZE) {
        ft245->rx_fifo[ft245->rx_fifo_head] = data;
        ft245->rx_fifo_head = (ft245->rx_fifo_head + 1) % FT245_RX_FIFO_SIZE;
        ft245->rx_fifo_count++;
        
        update_status_signals(ft245);
        return true;
    }
    return false;  // FIFO full
}

bool ft245_usb_transmit(ft245_t* ft245, uint8_t* data) {
    if (ft245->tx_fifo_count > 0) {
        *data = ft245->tx_fifo[ft245->tx_fifo_tail];
        ft245->tx_fifo_tail = (ft245->tx_fifo_tail + 1) % FT245_TX_FIFO_SIZE;
        ft245->tx_fifo_count--;
        
        update_status_signals(ft245);
        return true;
    }
    return false;  // FIFO empty
}

uint16_t ft245_usb_receive_buffer(ft245_t* ft245, const uint8_t* buffer, uint16_t length) {
    uint16_t count = 0;
    
    for (uint16_t i = 0; i < length; i++) {
        if (ft245_usb_receive(ft245, buffer[i])) {
            count++;
        } else {
            break;  // FIFO full
        }
    }
    
    return count;
}

uint16_t ft245_usb_transmit_buffer(ft245_t* ft245, uint8_t* buffer, uint16_t max_length) {
    uint16_t count = 0;
    
    for (uint16_t i = 0; i < max_length; i++) {
        if (ft245_usb_transmit(ft245, &buffer[i])) {
            count++;
        } else {
            break;  // FIFO empty
        }
    }
    
    return count;
}

void ft245_set_usb_connected(ft245_t* ft245, bool connected) {
    ft245->usb_connected = connected;
    
    if (!connected) {
        // USB disconnected - reset state
        ft245->usb_configured = false;
        ft245->rx_fifo_head = 0;
        ft245->rx_fifo_tail = 0;
        ft245->rx_fifo_count = 0;
        ft245->tx_fifo_head = 0;
        ft245->tx_fifo_tail = 0;
        ft245->tx_fifo_count = 0;
    }
    
    update_status_signals(ft245);
}

void ft245_set_usb_configured(ft245_t* ft245, bool configured) {
    if (ft245->usb_connected) {
        ft245->usb_configured = configured;
        update_status_signals(ft245);
    }
}

uint16_t ft245_get_rx_fifo_count(ft245_t* ft245) {
    return ft245->rx_fifo_count;
}

uint16_t ft245_get_tx_fifo_count(ft245_t* ft245) {
    return ft245->tx_fifo_count;
}

uint16_t ft245_get_rx_fifo_free(ft245_t* ft245) {
    return FT245_RX_FIFO_SIZE - ft245->rx_fifo_count;
}

uint16_t ft245_get_tx_fifo_free(ft245_t* ft245) {
    return FT245_TX_FIFO_SIZE - ft245->tx_fifo_count;
}

void ft245_clock(ft245_t* ft245) {
    // Update read timer
    if (!ft245->rd_n && ft245->read_timer < ft245->read_latency) {
        ft245->read_timer++;
        
        if (ft245->read_timer >= ft245->read_latency && ft245->rx_fifo_count > 0) {
            // Latency complete, update data bus
            uint8_t data = ft245->rx_fifo[ft245->rx_fifo_tail];
            ft245->data_bus = data;
        }
    }
    
    // Check for USB receive callback
    if (ft245->usb_rx_callback) {
        bool available = false;
        uint8_t byte = ft245->usb_rx_callback(ft245->usb_context, &available);
        if (available) {
            ft245_usb_receive(ft245, byte);
        }
    }
}

void ft245_set_usb_callbacks(ft245_t* ft245,
                              void (*tx_fn)(void*, uint8_t),
                              uint8_t (*rx_fn)(void*, bool*),
                              void* context) {
    ft245->usb_tx_callback = tx_fn;
    ft245->usb_rx_callback = rx_fn;
    ft245->usb_context = context;
}

void ft245_set_status_callback(ft245_t* ft245,
                                void (*status_fn)(void*, bool, bool),
                                void* context) {
    ft245->status_callback = status_fn;
    ft245->status_context = context;
}

// Internal helper functions

static void update_status_signals(ft245_t* ft245) {
    bool old_rxf = ft245->rxf_n;
    bool old_txe = ft245->txe_n;
    
    // Update RXF# - low when data available to read
    ft245->rxf_n = (ft245->rx_fifo_count == 0);
    
    // Update TXE# - low when transmit buffer has space
    ft245->txe_n = (ft245->tx_fifo_count >= FT245_TX_FIFO_SIZE);
    
    // Update PWREN# - low when USB configured
    ft245->pwren_n = !(ft245->usb_connected && ft245->usb_configured);
    
    // Call status callback if signals changed
    if ((old_rxf != ft245->rxf_n || old_txe != ft245->txe_n) && ft245->status_callback) {
        ft245->status_callback(ft245->status_context, ft245->rxf_n, ft245->txe_n);
    }
}

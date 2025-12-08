#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ft245.h"

// Test context
typedef struct {
    uint8_t usb_tx_buffer[256];
    uint16_t usb_tx_count;
    uint8_t usb_rx_buffer[256];
    uint16_t usb_rx_read_pos;
    uint16_t usb_rx_write_pos;
    int status_changes;
} test_context_t;

// USB transmit callback (CPU -> USB)
void usb_tx_callback(void* ctx, uint8_t byte) {
    test_context_t* context = (test_context_t*)ctx;
    if (context->usb_tx_count < 256) {
        context->usb_tx_buffer[context->usb_tx_count++] = byte;
        printf("  USB TX: 0x%02X ('%c')\n", 
               byte, (byte >= 32 && byte < 127) ? byte : '.');
    }
}

// USB receive callback (USB -> CPU)
uint8_t usb_rx_callback(void* ctx, bool* available) {
    test_context_t* context = (test_context_t*)ctx;
    if (context->usb_rx_read_pos < context->usb_rx_write_pos) {
        *available = true;
        return context->usb_rx_buffer[context->usb_rx_read_pos++];
    }
    *available = false;
    return 0;
}

// Status change callback
void status_callback(void* ctx, bool rxf_n, bool txe_n) {
    test_context_t* context = (test_context_t*)ctx;
    context->status_changes++;
    printf("  Status change: RXF#=%s TXE#=%s (changes: %d)\n",
           rxf_n ? "HIGH" : "LOW",
           txe_n ? "HIGH" : "LOW",
           context->status_changes);
}

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

void test_initialization(void) {
    print_test_header("Initialization and Reset");
    
    ft245_t ft245;
    ft245_init(&ft245);
    
    printf("\nAfter initialization:\n");
    printf("RXF# (no data): %s\n", ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    printf("TXE# (space available): %s\n", ft245_get_txe(&ft245) ? "HIGH" : "LOW");
    printf("PWREN# (not configured): %s\n", ft245_get_pwren(&ft245) ? "HIGH" : "LOW");
    printf("RX FIFO count: %u\n", ft245_get_rx_fifo_count(&ft245));
    printf("TX FIFO count: %u\n", ft245_get_tx_fifo_count(&ft245));
    
    printf("\n✓ Initialization test complete\n");
}

void test_usb_connection(void) {
    print_test_header("USB Connection and Configuration");
    
    ft245_t ft245;
    ft245_init(&ft245);
    
    printf("\nInitial state - USB disconnected:\n");
    printf("PWREN#: %s (should be HIGH)\n", ft245_get_pwren(&ft245) ? "HIGH" : "LOW");
    
    printf("\nConnecting USB...\n");
    ft245_set_usb_connected(&ft245, true);
    printf("PWREN#: %s (still HIGH, not configured)\n", 
           ft245_get_pwren(&ft245) ? "HIGH" : "LOW");
    
    printf("\nConfiguring USB (enumeration complete)...\n");
    ft245_set_usb_configured(&ft245, true);
    printf("PWREN#: %s (should be LOW when configured)\n", 
           ft245_get_pwren(&ft245) ? "HIGH" : "LOW");
    
    printf("\nDisconnecting USB...\n");
    ft245_set_usb_connected(&ft245, false);
    printf("PWREN#: %s (should be HIGH)\n", ft245_get_pwren(&ft245) ? "HIGH" : "LOW");
    
    printf("\n✓ USB connection test complete\n");
}

void test_cpu_write_to_usb(void) {
    print_test_header("CPU Write to USB (TX Path)");
    
    ft245_t ft245;
    test_context_t context = {0};
    
    ft245_init(&ft245);
    ft245_set_usb_callbacks(&ft245, usb_tx_callback, usb_rx_callback, &context);
    ft245_set_usb_configured(&ft245, true);
    
    printf("\nInitial TXE# state: %s (should be LOW - space available)\n",
           ft245_get_txe(&ft245) ? "HIGH" : "LOW");
    
    printf("\nCPU writes 'HELLO' to FT245:\n");
    const char* msg = "HELLO";
    for (int i = 0; i < 5; i++) {
        printf("Writing '%c' (0x%02X)...\n", msg[i], msg[i]);
        
        // Assert WR
        ft245_set_wr(&ft245, true);
        ft245_write(&ft245, msg[i]);
        
        // Clock for latency
        for (int j = 0; j < 5; j++) {
            ft245_clock(&ft245);
        }
        
        // Deassert WR
        ft245_set_wr(&ft245, false);
    }
    
    printf("\nVerifying transmitted data:\n");
    printf("Expected: HELLO\n");
    printf("Received: ");
    for (int i = 0; i < context.usb_tx_count; i++) {
        printf("%c", context.usb_tx_buffer[i]);
    }
    printf("\n");
    
    printf("TX FIFO count: %u\n", ft245_get_tx_fifo_count(&ft245));
    
    printf("\n✓ CPU write test complete\n");
}

void test_usb_read_from_cpu(void) {
    print_test_header("USB Read from CPU (RX Path)");
    
    ft245_t ft245;
    test_context_t context = {0};
    
    ft245_init(&ft245);
    ft245_set_usb_callbacks(&ft245, usb_tx_callback, usb_rx_callback, &context);
    ft245_set_status_callback(&ft245, status_callback, &context);
    ft245_set_usb_configured(&ft245, true);
    
    printf("\nInitial RXF# state: %s (should be HIGH - no data)\n",
           ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    
    printf("\nUSB sends 'WORLD' to CPU:\n");
    const char* msg = "WORLD";
    for (int i = 0; i < 5; i++) {
        printf("USB sending '%c' (0x%02X)...\n", msg[i], msg[i]);
        ft245_usb_receive(&ft245, msg[i]);
    }
    
    printf("\nRXF# state: %s (should be LOW - data available)\n",
           ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    printf("RX FIFO count: %u\n", ft245_get_rx_fifo_count(&ft245));
    
    printf("\nCPU reads data:\n");
    for (int i = 0; i < 5; i++) {
        // Assert RD#
        ft245_set_rd(&ft245, true);
        
        // Clock for latency
        for (int j = 0; j < 5; j++) {
            ft245_clock(&ft245);
        }
        
        uint8_t data = ft245_read(&ft245);
        printf("Read: 0x%02X ('%c')\n", data, data);
        
        // Deassert RD#
        ft245_set_rd(&ft245, false);
    }
    
    printf("\nRXF# state: %s (should be HIGH - no more data)\n",
           ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    printf("RX FIFO count: %u\n", ft245_get_rx_fifo_count(&ft245));
    
    printf("\n✓ USB read test complete\n");
}

void test_bidirectional_transfer(void) {
    print_test_header("Bidirectional Data Transfer");
    
    ft245_t ft245;
    test_context_t context = {0};
    
    ft245_init(&ft245);
    ft245_set_usb_callbacks(&ft245, usb_tx_callback, usb_rx_callback, &context);
    ft245_set_usb_configured(&ft245, true);
    
    printf("\nUSB sends data to CPU:\n");
    const char* rx_msg = "TEST";
    for (int i = 0; i < 4; i++) {
        ft245_usb_receive(&ft245, rx_msg[i]);
    }
    
    printf("\nCPU sends data to USB:\n");
    const char* tx_msg = "ECHO";
    for (int i = 0; i < 4; i++) {
        ft245_set_wr(&ft245, true);
        ft245_write(&ft245, tx_msg[i]);
        for (int j = 0; j < 5; j++) ft245_clock(&ft245);
        ft245_set_wr(&ft245, false);
    }
    
    printf("\nReading data from USB (at CPU):\n");
    for (int i = 0; i < 4; i++) {
        ft245_set_rd(&ft245, true);
        for (int j = 0; j < 5; j++) ft245_clock(&ft245);
        uint8_t data = ft245_read(&ft245);
        printf("CPU received: '%c'\n", data);
        ft245_set_rd(&ft245, false);
    }
    
    printf("\nVerifying USB received data:\n");
    printf("Expected: ECHO\n");
    printf("Received: ");
    for (int i = 0; i < context.usb_tx_count; i++) {
        printf("%c", context.usb_tx_buffer[i]);
    }
    printf("\n");
    
    printf("\n✓ Bidirectional transfer test complete\n");
}

void test_fifo_status(void) {
    print_test_header("FIFO Status and Capacity");
    
    ft245_t ft245;
    ft245_init(&ft245);
    ft245_set_usb_configured(&ft245, true);
    
    printf("\nInitial FIFO state:\n");
    printf("RX FIFO: %u used, %u free\n", 
           ft245_get_rx_fifo_count(&ft245),
           ft245_get_rx_fifo_free(&ft245));
    printf("TX FIFO: %u used, %u free\n", 
           ft245_get_tx_fifo_count(&ft245),
           ft245_get_tx_fifo_free(&ft245));
    
    printf("\nFilling RX FIFO with 100 bytes:\n");
    for (int i = 0; i < 100; i++) {
        ft245_usb_receive(&ft245, i & 0xFF);
    }
    printf("RX FIFO: %u used, %u free\n", 
           ft245_get_rx_fifo_count(&ft245),
           ft245_get_rx_fifo_free(&ft245));
    
    printf("\nFilling TX FIFO with 50 bytes:\n");
    for (int i = 0; i < 50; i++) {
        ft245_set_wr(&ft245, true);
        ft245_write(&ft245, i & 0xFF);
        for (int j = 0; j < 5; j++) ft245_clock(&ft245);
        ft245_set_wr(&ft245, false);
    }
    printf("TX FIFO: %u used, %u free\n", 
           ft245_get_tx_fifo_count(&ft245),
           ft245_get_tx_fifo_free(&ft245));
    
    printf("\n✓ FIFO status test complete\n");
}

void test_buffer_operations(void) {
    print_test_header("Buffer Operations");
    
    ft245_t ft245;
    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];
    
    ft245_init(&ft245);
    ft245_set_usb_configured(&ft245, true);
    
    printf("\nSending 128 bytes via USB using buffer operation:\n");
    uint8_t test_data[128];
    for (int i = 0; i < 128; i++) {
        test_data[i] = i;
    }
    
    uint16_t sent = ft245_usb_receive_buffer(&ft245, test_data, 128);
    printf("Sent %u bytes to RX FIFO\n", sent);
    printf("RX FIFO count: %u\n", ft245_get_rx_fifo_count(&ft245));
    
    printf("\nFilling TX FIFO:\n");
    for (int i = 0; i < 64; i++) {
        ft245_set_wr(&ft245, true);
        ft245_write(&ft245, 0xA0 + i);
        for (int j = 0; j < 5; j++) ft245_clock(&ft245);
        ft245_set_wr(&ft245, false);
    }
    
    printf("\nReading from TX FIFO using buffer operation:\n");
    uint16_t received = ft245_usb_transmit_buffer(&ft245, tx_buffer, 64);
    printf("Received %u bytes from TX FIFO\n", received);
    printf("First byte: 0x%02X (expected 0xA0)\n", tx_buffer[0]);
    printf("Last byte: 0x%02X (expected 0xDF)\n", tx_buffer[received-1]);
    
    printf("\n✓ Buffer operations test complete\n");
}

void test_control_signals(void) {
    print_test_header("Control Signal Behavior");
    
    ft245_t ft245;
    ft245_init(&ft245);
    ft245_set_usb_configured(&ft245, true);
    
    printf("\nTesting RXF# signal:\n");
    printf("Empty FIFO - RXF#: %s (should be HIGH)\n", 
           ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    
    ft245_usb_receive(&ft245, 0x42);
    printf("After adding byte - RXF#: %s (should be LOW)\n", 
           ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    
    ft245_set_rd(&ft245, true);
    for (int i = 0; i < 5; i++) ft245_clock(&ft245);
    ft245_read(&ft245);
    ft245_set_rd(&ft245, false);
    printf("After reading byte - RXF#: %s (should be HIGH)\n", 
           ft245_get_rxf(&ft245) ? "HIGH" : "LOW");
    
    printf("\nTesting TXE# signal:\n");
    printf("Empty TX FIFO - TXE#: %s (should be LOW - space available)\n",
           ft245_get_txe(&ft245) ? "HIGH" : "LOW");
    
    // Fill TX FIFO completely
    printf("Filling TX FIFO to capacity...\n");
    int written = 0;
    for (int i = 0; i < FT245_TX_FIFO_SIZE + 10; i++) {
        if (ft245_get_tx_fifo_free(&ft245) > 0) {
            ft245_set_wr(&ft245, true);
            ft245_write(&ft245, i & 0xFF);
            for (int j = 0; j < 5; j++) ft245_clock(&ft245);
            ft245_set_wr(&ft245, false);
            written++;
        }
    }
    printf("Wrote %d bytes\n", written);
    printf("Full TX FIFO - TXE#: %s (should be HIGH - no space)\n",
           ft245_get_txe(&ft245) ? "HIGH" : "LOW");
    
    printf("\n✓ Control signal test complete\n");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  FT245 USB FIFO Emulation Test Suite          ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_initialization();
    test_usb_connection();
    test_cpu_write_to_usb();
    test_usb_read_from_cpu();
    test_bidirectional_transfer();
    test_fifo_status();
    test_buffer_operations();
    test_control_signals();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  All tests completed successfully!            ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}

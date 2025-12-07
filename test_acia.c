#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "acia6551.h"

// Test context for callbacks
typedef struct {
    uint8_t tx_buffer[256];
    uint16_t tx_count;
    uint8_t rx_buffer[256];
    uint16_t rx_read_pos;
    uint16_t rx_write_pos;
    int irq_count;
    bool irq_state;
    bool dtr_state;
    int dtr_changes;
} test_context_t;

// Transmit callback - stores transmitted bytes
void tx_byte_callback(void* ctx, uint8_t byte) {
    test_context_t* context = (test_context_t*)ctx;
    if (context->tx_count < 256) {
        context->tx_buffer[context->tx_count++] = byte;
        printf("  TX: 0x%02X ('%c') [count: %d]\n", 
               byte, (byte >= 32 && byte < 127) ? byte : '.', context->tx_count);
    }
}

// Receive callback - provides bytes from buffer
uint8_t rx_byte_callback(void* ctx, bool* available) {
    test_context_t* context = (test_context_t*)ctx;
    if (context->rx_read_pos < context->rx_write_pos) {
        *available = true;
        return context->rx_buffer[context->rx_read_pos++];
    }
    *available = false;
    return 0;
}

// IRQ callback
void irq_callback(void* ctx, bool state) {
    test_context_t* context = (test_context_t*)ctx;
    if (state && !context->irq_state) {
        context->irq_count++;
        printf("  *** IRQ ASSERTED (count: %d) ***\n", context->irq_count);
    } else if (!state && context->irq_state) {
        printf("  *** IRQ CLEARED ***\n");
    }
    context->irq_state = state;
}

// DTR callback
void dtr_callback(void* ctx, bool state) {
    test_context_t* context = (test_context_t*)ctx;
    context->dtr_state = state;
    context->dtr_changes++;
    printf("  DTR: %s (changes: %d)\n", state ? "ACTIVE" : "INACTIVE", context->dtr_changes);
}

void print_test_header(const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

void test_register_access(void) {
    print_test_header("Register Read/Write Access");
    
    acia6551_t acia;
    acia6551_init(&acia);
    
    printf("\nWriting command register: 0x4B\n");
    acia6551_write(&acia, ACIA_COMMAND, 0x4B);
    uint8_t cmd = acia6551_read(&acia, ACIA_COMMAND);
    printf("Read back: 0x%02X (expected 0x4B)\n", cmd);
    
    printf("\nWriting control register: 0x1F (19200, 8N1)\n");
    acia6551_write(&acia, ACIA_CONTROL, 0x1F);
    uint8_t ctrl = acia6551_read(&acia, ACIA_CONTROL);
    printf("Read back: 0x%02X (expected 0x1F)\n", ctrl);
    
    printf("\nReading status register\n");
    uint8_t status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  TDRE (bit 4): %s\n", (status & 0x10) ? "SET" : "CLEAR");
    
    printf("\n✓ Register access test complete\n");
}

void test_baud_rate_config(void) {
    print_test_header("Baud Rate Configuration");
    
    acia6551_t acia;
    acia6551_init(&acia);
    
    struct {
        uint8_t setting;
        uint32_t expected_baud;
        const char* name;
    } baud_tests[] = {
        {ACIA_CTRL_BAUD_300, 300, "300 baud"},
        {ACIA_CTRL_BAUD_1200, 1200, "1200 baud"},
        {ACIA_CTRL_BAUD_2400, 2400, "2400 baud"},
        {ACIA_CTRL_BAUD_9600, 9600, "9600 baud"},
        {ACIA_CTRL_BAUD_19200, 19200, "19200 baud"},
    };
    
    for (int i = 0; i < 5; i++) {
        printf("\nSetting %s\n", baud_tests[i].name);
        acia6551_write(&acia, ACIA_CONTROL, baud_tests[i].setting);
        uint32_t baud = acia6551_get_baud_rate(&acia);
        printf("Baud rate: %u (expected %u)\n", baud, baud_tests[i].expected_baud);
    }
    
    printf("\n✓ Baud rate configuration test complete\n");
}

void test_word_length_config(void) {
    print_test_header("Word Length Configuration");
    
    acia6551_t acia;
    acia6551_init(&acia);
    
    struct {
        uint8_t setting;
        uint8_t expected_bits;
        const char* name;
    } word_tests[] = {
        {ACIA_CTRL_WORD_8BIT, 8, "8 bits"},
        {ACIA_CTRL_WORD_7BIT, 7, "7 bits"},
        {ACIA_CTRL_WORD_6BIT, 6, "6 bits"},
        {ACIA_CTRL_WORD_5BIT, 5, "5 bits"},
    };
    
    for (int i = 0; i < 4; i++) {
        printf("\nSetting word length: %s\n", word_tests[i].name);
        acia6551_write(&acia, ACIA_CONTROL, word_tests[i].setting | ACIA_CTRL_BAUD_9600);
        uint8_t bits = acia6551_get_word_length(&acia);
        printf("Word length: %u (expected %u)\n", bits, word_tests[i].expected_bits);
    }
    
    printf("\n✓ Word length configuration test complete\n");
}

void test_transmit_data(void) {
    print_test_header("Data Transmission");
    
    acia6551_t acia;
    test_context_t context = {0};
    
    acia6551_init(&acia);
    acia6551_set_byte_callbacks(&acia, tx_byte_callback, rx_byte_callback, &context);
    
    printf("\nConfiguring ACIA: 9600 baud, 8N1\n");
    acia6551_write(&acia, ACIA_CONTROL, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    
    printf("\nTransmitting 'Hello'\n");
    const char* msg = "Hello";
    for (int i = 0; i < 5; i++) {
        acia6551_write(&acia, ACIA_DATA, msg[i]);
    }
    
    printf("\nClocking ACIA to process transmission\n");
    acia6551_clock(&acia, 1000);
    
    printf("\nVerifying transmitted data\n");
    printf("Expected: 'Hello'\n");
    printf("Received: ");
    for (int i = 0; i < context.tx_count && i < 5; i++) {
        printf("%c", context.tx_buffer[i]);
    }
    printf("\n");
    
    printf("\n✓ Transmit data test complete\n");
}

void test_receive_data(void) {
    print_test_header("Data Reception");
    
    acia6551_t acia;
    test_context_t context = {0};
    
    acia6551_init(&acia);
    acia6551_set_byte_callbacks(&acia, tx_byte_callback, rx_byte_callback, &context);
    acia6551_set_irq_callback(&acia, irq_callback, &context);
    
    printf("\nConfiguring ACIA: 9600 baud, 8N1\n");
    acia6551_write(&acia, ACIA_CONTROL, ACIA_CTRL_BAUD_9600 | ACIA_CTRL_WORD_8BIT);
    
    printf("\nEnabling receive interrupts\n");
    acia6551_write(&acia, ACIA_COMMAND, ACIA_CMD_IRQ_RX_ENABLE);
    
    printf("\nSimulating received bytes: 'A', 'B', 'C'\n");
    acia6551_receive_byte(&acia, 'A');
    acia6551_receive_byte(&acia, 'B');
    acia6551_receive_byte(&acia, 'C');
    
    printf("\nReading received data\n");
    for (int i = 0; i < 3; i++) {
        uint8_t status = acia6551_read(&acia, ACIA_STATUS);
        printf("Status: 0x%02X (RDRF: %s)\n", 
               status, (status & ACIA_STATUS_RDRF) ? "SET" : "CLEAR");
        
        if (status & ACIA_STATUS_RDRF) {
            uint8_t data = acia6551_read(&acia, ACIA_DATA);
            printf("  Read: 0x%02X ('%c')\n", data, data);
        }
    }
    
    printf("\n✓ Receive data test complete\n");
}

void test_status_flags(void) {
    print_test_header("Status Register Flags");
    
    acia6551_t acia;
    acia6551_init(&acia);
    
    printf("\nInitial status\n");
    uint8_t status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  TDRE (bit 4): %s (should be SET on reset)\n", 
           (status & ACIA_STATUS_TDRE) ? "SET" : "CLEAR");
    
    printf("\nWriting data (should clear TDRE)\n");
    acia6551_write(&acia, ACIA_DATA, 0x42);
    status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  TDRE (bit 4): %s\n", (status & ACIA_STATUS_TDRE) ? "SET" : "CLEAR");
    
    printf("\nReceiving byte (should set RDRF)\n");
    acia6551_receive_byte(&acia, 0x99);
    status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  RDRF (bit 3): %s\n", (status & ACIA_STATUS_RDRF) ? "SET" : "CLEAR");
    
    printf("\nReading data (should clear RDRF)\n");
    acia6551_read(&acia, ACIA_DATA);
    status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  RDRF (bit 3): %s\n", (status & ACIA_STATUS_RDRF) ? "SET" : "CLEAR");
    
    printf("\n✓ Status flags test complete\n");
}

void test_dtr_control(void) {
    print_test_header("DTR Control Line");
    
    acia6551_t acia;
    test_context_t context = {0};
    
    acia6551_init(&acia);
    acia6551_set_dtr_callback(&acia, dtr_callback, &context);
    
    printf("\nEnabling DTR (command bit 0 = 1)\n");
    acia6551_write(&acia, ACIA_COMMAND, ACIA_CMD_DTR_ENABLE);
    printf("DTR state: %s\n", acia6551_get_dtr(&acia) ? "ACTIVE" : "INACTIVE");
    
    printf("\nDisabling DTR (command bit 0 = 0)\n");
    acia6551_write(&acia, ACIA_COMMAND, ACIA_CMD_DTR_DISABLE);
    printf("DTR state: %s\n", acia6551_get_dtr(&acia) ? "ACTIVE" : "INACTIVE");
    
    printf("\nRe-enabling DTR\n");
    acia6551_write(&acia, ACIA_COMMAND, ACIA_CMD_DTR_ENABLE);
    printf("DTR state: %s\n", acia6551_get_dtr(&acia) ? "ACTIVE" : "INACTIVE");
    
    printf("\nTotal DTR changes: %d (expected 3)\n", context.dtr_changes);
    
    printf("\n✓ DTR control test complete\n");
}

void test_control_lines(void) {
    print_test_header("Control Line Inputs (DCD, DSR)");
    
    acia6551_t acia;
    acia6551_init(&acia);
    
    printf("\nInitial state (DCD and DSR inactive/inverted)\n");
    uint8_t status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  DCD (bit 5): %s\n", (status & ACIA_STATUS_DCD) ? "SET" : "CLEAR");
    printf("  DSR (bit 6): %s\n", (status & ACIA_STATUS_DSR) ? "SET" : "CLEAR");
    
    printf("\nActivating DCD (inverted logic)\n");
    acia6551_set_dcd(&acia, true);
    status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  DCD (bit 5): %s (should be CLEAR when active)\n", 
           (status & ACIA_STATUS_DCD) ? "SET" : "CLEAR");
    
    printf("\nActivating DSR (inverted logic)\n");
    acia6551_set_dsr(&acia, true);
    status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X\n", status);
    printf("  DSR (bit 6): %s (should be CLEAR when active)\n", 
           (status & ACIA_STATUS_DSR) ? "SET" : "CLEAR");
    
    printf("\n✓ Control line inputs test complete\n");
}

void test_interrupts(void) {
    print_test_header("Interrupt Generation");
    
    acia6551_t acia;
    test_context_t context = {0};
    
    acia6551_init(&acia);
    acia6551_set_irq_callback(&acia, irq_callback, &context);
    
    printf("\n--- Receive Interrupt Test ---\n");
    printf("Enabling receive interrupts\n");
    acia6551_write(&acia, ACIA_COMMAND, ACIA_CMD_IRQ_RX_ENABLE);
    
    printf("\nReceiving a byte\n");
    acia6551_receive_byte(&acia, 'X');
    
    printf("\nReading data to clear interrupt\n");
    acia6551_read(&acia, ACIA_DATA);
    
    printf("\n--- Transmit Interrupt Test ---\n");
    printf("Enabling transmit interrupts\n");
    acia6551_write(&acia, ACIA_COMMAND, ACIA_CMD_IRQ_TX_ENABLE);
    
    printf("\nTDRE should be set, triggering IRQ\n");
    
    printf("\nTotal IRQ assertions: %d\n", context.irq_count);
    
    printf("\n✓ Interrupt test complete\n");
}

void test_programmed_reset(void) {
    print_test_header("Programmed Reset");
    
    acia6551_t acia;
    acia6551_init(&acia);
    
    printf("\nSetting registers to non-default values\n");
    acia6551_write(&acia, ACIA_COMMAND, 0xFF);
    acia6551_write(&acia, ACIA_CONTROL, 0xFF);
    
    printf("Command: 0x%02X\n", acia6551_read(&acia, ACIA_COMMAND));
    printf("Control: 0x%02X\n", acia6551_read(&acia, ACIA_CONTROL));
    
    printf("\nPerforming programmed reset (write to status register)\n");
    acia6551_write(&acia, ACIA_RESET, 0x00);
    
    printf("\nReading registers after reset\n");
    printf("Command: 0x%02X (expected 0x00)\n", acia6551_read(&acia, ACIA_COMMAND));
    printf("Control: 0x%02X (expected 0x00)\n", acia6551_read(&acia, ACIA_CONTROL));
    uint8_t status = acia6551_read(&acia, ACIA_STATUS);
    printf("Status: 0x%02X (TDRE should be set)\n", status);
    
    printf("\n✓ Programmed reset test complete\n");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  6551 ACIA (Asynchronous Communications       ║\n");
    printf("║  Interface Adapter) Emulation Test Suite      ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    test_register_access();
    test_baud_rate_config();
    test_word_length_config();
    test_transmit_data();
    test_receive_data();
    test_status_flags();
    test_dtr_control();
    test_control_lines();
    test_interrupts();
    test_programmed_reset();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  All tests completed successfully!            ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}

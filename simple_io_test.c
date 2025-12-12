/*
 * Simple IO Test - Testing code with board_fifo IO
 * 
 * This demonstrates a simple echo program where:
 * - USB/PC sends data to CPU
 * - CPU reads it via VIA/FIFO
 * - CPU echoes it back to USB/PC
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "simple_io.h"

// Test 1: Simple echo program
void test_simple_echo(void) {
    printf("\n========================================\n");
    printf("TEST: Simple Echo Program\n");
    printf("========================================\n\n");
    
    fifo_t *fifo = io_init();
    if (!fifo) return;
    
    // PC/USB sends data to CPU
    const char *input = "Hello, World!";
    printf("USB sending to CPU: '%s'\n", input);
    board_fifo_usb_send_buffer(fifo, (const uint8_t *)input, strlen(input));
    
    // CPU echoes it back
    printf("CPU echoing back to USB...\n");
    char output[256];
    int i = 0;
    for (int j = 0; j < strlen(input); j++) {
        uint8_t byte = io_read_byte(fifo);
        output[i++] = byte;
        io_write_byte(fifo, byte);
    }
    output[i] = '\0';
    
    // PC/USB receives echoed data
    uint8_t received[256];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, received, 256);
    received[count] = '\0';
    
    printf("\nResults:\n");
    printf("  Input:    '%s'\n", input);
    printf("  Echoed:   '%s'\n", (char *)received);
    printf("  Match:    %s\n", (strcmp(input, (char *)received) == 0) ? "✓ YES" : "✗ NO");
    
    free_board_fifo(fifo);
    printf("\n✓ Echo test complete\n");
}

// Test 2: Send greeting from CPU
void test_cpu_greeting(void) {
    printf("\n========================================\n");
    printf("TEST: CPU Sends Greeting\n");
    printf("========================================\n\n");
    
    fifo_t *fifo = io_init();
    if (!fifo) return;
    
    // CPU sends a greeting to USB
    printf("CPU sending greeting to USB...\n");
    const char *greeting = "Hello from 65816 CPU!\n";
    io_write_string(fifo, greeting);
    
    // USB receives it
    uint8_t received[256];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, received, 256);
    received[count] = '\0';
    
    printf("\nUSB received %u bytes: '%s'\n", count, (char *)received);
    printf("Match: %s\n", (strcmp(greeting, (char *)received) == 0) ? "✓ YES" : "✗ NO");
    
    free_board_fifo(fifo);
    printf("\n✓ Greeting test complete\n");
}

// Test 3: CPU processes input and sends response
void test_cpu_processing(void) {
    printf("\n========================================\n");
    printf("TEST: CPU Processes Input\n");
    printf("========================================\n\n");
    
    fifo_t *fifo = io_init();
    if (!fifo) return;
    
    // USB sends a command to CPU
    const char *command = "ADD 5 7\n";
    printf("USB sending command: '%s'\n", command);
    board_fifo_usb_send_buffer(fifo, (const uint8_t *)command, strlen(command));
    
    // CPU reads command, processes it, sends response
    printf("CPU reading command...\n");
    char input_buffer[256];
    int i = 0;
    while (i < 255) {
        uint8_t byte = io_read_byte(fifo);
        input_buffer[i++] = byte;
        if (byte == '\n') break;
    }
    input_buffer[i] = '\0';
    printf("CPU received: '%s'\n", input_buffer);
    
    // CPU sends response
    printf("CPU processing and responding...\n");
    const char *response = "RESULT: 12\n";
    io_write_string(fifo, response);
    
    // USB receives response
    uint8_t received[256];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, received, 256);
    received[count] = '\0';
    
    printf("\nUSB received response: '%s'\n", (char *)received);
    
    free_board_fifo(fifo);
    printf("\n✓ Processing test complete\n");
}

// Test 4: Interactive demo
void test_interactive_io(void) {
    printf("\n========================================\n");
    printf("TEST: Interactive IO Demo\n");
    printf("========================================\n\n");
    
    fifo_t *fifo = io_init();
    if (!fifo) return;
    
    // Simulate a conversation
    const char *messages[] = {
        "PING\n",
        "STATUS\n",
        "DATA 42\n",
        "QUIT\n"
    };
    
    const char *responses[] = {
        "PONG\n",
        "OK: Ready\n",
        "ACK: Received 42\n",
        "BYE\n"
    };
    
    for (int msg = 0; msg < 4; msg++) {
        // USB sends message
        printf("\n[USB → CPU] '%s'", messages[msg]);
        board_fifo_usb_send_buffer(fifo, (const uint8_t *)messages[msg], strlen(messages[msg]));
        
        // CPU reads and processes
        char input[256];
        int i = 0;
        while (i < 255) {
            uint8_t byte = io_read_byte(fifo);
            input[i++] = byte;
            if (byte == '\n') break;
        }
        input[i] = '\0';
        
        // CPU responds
        io_write_string(fifo, responses[msg]);
        
        // USB receives response
        uint8_t received[256];
        uint16_t count = board_fifo_usb_receive_buffer(fifo, received, 256);
        received[count] = '\0';
        
        printf("[CPU → USB] '%s'", (char *)received);
    }
    
    free_board_fifo(fifo);
    printf("\n\n✓ Interactive demo complete\n");
}

int main(void) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Simple IO Test with Board FIFO        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    test_simple_echo();
    test_cpu_greeting();
    test_cpu_processing();
    test_interactive_io();
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║  All IO tests completed successfully   ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}

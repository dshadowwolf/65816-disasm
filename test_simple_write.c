#include <stdio.h>
#include <stdint.h>
#include "board_fifo.h"
#include "via6522.h"
#include "ft245.h"

int main(void) {
    printf("Simple write test\n\n");
    
    fifo_t *fifo = init_board_fifo();
    
    // Configure VIA
    printf("1. Configure Port A as output\n");
    board_fifo_write_via(fifo, VIA_DDRA, 0xFF);
    
    printf("2. Configure Port B bits 0-1 as output\n");
    board_fifo_write_via(fifo, VIA_DDRB, 0x03);
    
    printf("3. Check initial state\n");
    printf("   TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    printf("\n4. Write 'A' (0x41) to Port A\n");
    board_fifo_write_via(fifo, VIA_ORA_IRA, 0x41);
    printf("   TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    printf("\n5. Assert WR (set bit 1 high, keep RD# high)\n");
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N | PORTB_WR);
    printf("   TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    printf("\n6. Clock a few times\n");
    for (int i = 0; i < 5; i++) {
        board_fifo_clock(fifo);
    }
    printf("   TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    printf("\n7. Deassert WR\n");
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    printf("   TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    printf("\n8. Try to read from USB side\n");
    uint8_t buffer[10];
    uint16_t count = board_fifo_usb_receive_buffer(fifo, buffer, 10);
    printf("   Received %u bytes: ", count);
    for (int i = 0; i < count; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    
    // Try the other sequence: WR first, then data
    printf("\n\n=== Test 2: Assert WR first ===\n");
    printf("1. Assert WR\n");
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N | PORTB_WR);
    
    printf("2. Write 'B' (0x42) to Port A\n");
    board_fifo_write_via(fifo, VIA_ORA_IRA, 0x42);
    printf("   TX FIFO count: %u\n", board_fifo_get_tx_count(fifo));
    
    printf("3. Deassert WR\n");
    board_fifo_write_via(fifo, VIA_ORB_IRB, PORTB_RD_N);
    
    printf("4. Read from USB side\n");
    count = board_fifo_usb_receive_buffer(fifo, buffer, 10);
    printf("   Received %u bytes: ", count);
    for (int i = 0; i < count; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    
    free_board_fifo(fifo);
    return 0;
}

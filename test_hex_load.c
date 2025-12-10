#include <stdio.h>
#include <stdlib.h>
#include "machine.h"
#include "machine_setup.h"
#include "processor.h"
#include "processor_helpers.h"

int main() {
    printf("=== Hex File Loader Test ===\n\n");
    
    // Create test hex file
    FILE *fp = fopen("test_program.hex", "w");
    if (!fp) {
        fprintf(stderr, "Failed to create test file\n");
        return 1;
    }
    
    // Write a test program in hex format
    fprintf(fp, "# Test program for 65816 emulator\n");
    fprintf(fp, "# Format: address:byte byte byte...\n");
    fprintf(fp, "\n");
    fprintf(fp, "# Reset vectors\n");
    fprintf(fp, "FFFC:00 80\n");
    fprintf(fp, "\n");
    fprintf(fp, "# Program at 0x8000\n");
    fprintf(fp, "8000:18          # CLC\n");
    fprintf(fp, "8001:FB          # XCE - switch to native mode\n");
    fprintf(fp, "8002:C2 30       # REP #$30 - 16-bit mode\n");
    fprintf(fp, "8004:A9 34 12    # LDA #$1234\n");
    fprintf(fp, "8007:8D 00 20    # STA $2000 (write to RAM)\n");
    fprintf(fp, "800A:E2 20       # SEP #$20 - 8-bit accumulator\n");
    fprintf(fp, "800C:A9 42       # LDA #$42\n");
    fprintf(fp, "800E:8D 01 20    # STA $2001\n");
    fprintf(fp, "8011:A2 CD AB    # LDX #$ABCD (16-bit)\n");
    fprintf(fp, "8014:EA          # NOP\n");
    fprintf(fp, "8015:DB          # STP\n");
    fprintf(fp, "\n");
    fprintf(fp, "# Some data at 0x9000\n");
    fprintf(fp, "9000:DE AD BE EF CA FE BA BE\n");
    fclose(fp);
    
    printf("Created test_program.hex\n\n");
    
    // Create and initialize machine
    machine_state_t *machine = create_machine();
    if (!machine) {
        fprintf(stderr, "Failed to create machine\n");
        return 1;
    }
    
    // Load the hex file
    int result = load_hex_file(machine, "test_program.hex");
    if (result < 0) {
        fprintf(stderr, "Failed to load hex file\n");
        destroy_machine(machine);
        return 1;
    }
    
    printf("\nVerifying loaded data:\n");
    
    // Verify reset vector
    uint16_t reset_vector = read_byte_new(machine, 0xFFFC) | 
                           (read_byte_new(machine, 0xFFFD) << 8);
    printf("  Reset vector: $%04X\n", reset_vector);
    
    // Verify program bytes
    printf("  Program at 0x8000: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", read_byte_new(machine, 0x8000 + i));
    }
    printf("\n");
    
    // Verify data at 0x9000
    printf("  Data at 0x9000: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", read_byte_new(machine, 0x9000 + i));
    }
    printf("\n\n");
    
    // Reset processor and load PC from reset vector
    reset_processor(&machine->processor);
    machine->processor.PC = reset_vector;
    
    printf("Running program:\n");
    printf("Initial state: PC=$%04X A=$%04X X=$%04X Y=$%04X P=$%02X\n\n",
           machine->processor.PC, machine->processor.A.full,
           machine->processor.X, machine->processor.Y, machine->processor.P);
    
    // Execute program step by step
    int step = 0;
    while (step < 15) {
        step_result_t *r = machine_step(machine);
        if (!r) break;
        
        printf("%2d. %04X: %-4s %-10s  A=$%04X X=$%04X Y=$%04X P=$%02X\n",
               step, r->address & 0xFFFF, r->mnemonic, r->operand_str,
               machine->processor.A.full, machine->processor.X,
               machine->processor.Y, machine->processor.P);
        
        if (r->halted) {
            printf("\nProcessor halted\n");
            free_step_result(r);
            break;
        }
        
        free_step_result(r);
        step++;
    }
    
    printf("\nFinal state: PC=$%04X A=$%04X X=$%04X Y=$%04X P=$%02X\n",
           machine->processor.PC, machine->processor.A.full,
           machine->processor.X, machine->processor.Y, machine->processor.P);
    
    // Verify data written to RAM
    printf("\nData written to RAM at 0x2000: %02X %02X\n",
           read_byte_new(machine, 0x2000),
           read_byte_new(machine, 0x2001));
    
    // Cleanup
    destroy_machine(machine);
    
    printf("\n=== Test Complete ===\n");
    return 0;
}

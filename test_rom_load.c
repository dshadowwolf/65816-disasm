/*
 * Test ROM Loading Functionality
 * 
 * This program demonstrates loading a binary file into the ROM region (0x8000-0xFFFF)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "machine_setup.h"
#include "machine.h"
#include "processor_helpers.h"

void create_test_rom_file(const char* filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create test ROM file\n");
        return;
    }
    
    // Create a small test ROM with some recognizable data
    uint8_t rom_data[256];
    
    // Fill with a pattern
    for (int i = 0; i < 256; i++) {
        rom_data[i] = i & 0xFF;
    }
    
    // Add some special markers
    rom_data[0] = 0xAA;  // Start marker
    rom_data[1] = 0x55;
    rom_data[254] = 0xBE; // End marker  
    rom_data[255] = 0xEF;
    
    // Write the 65816 reset vector at the end (points to 0x8000)
    // The reset vector is at 0xFFFC-0xFFFD
    // Since we're only writing 256 bytes, we won't have the actual vectors
    // but in a real ROM, you'd set rom_data[0xFFFC-0x8000] and rom_data[0xFFFD-0x8000]
    
    fwrite(rom_data, 1, 256, fp);
    fclose(fp);
    
    printf("Created test ROM file: %s (256 bytes)\n", filename);
}

int main(void) {
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  ROM Loading Test                              ║\n");
    printf("╚════════════════════════════════════════════════╝\n\n");
    
    // Create test ROM file
    const char* rom_file = "test_rom.bin";
    create_test_rom_file(rom_file);
    
    // Initialize machine
    machine_state_t machine;
    initialize_machine(&machine);
    
    printf("\n--- Before Loading ROM ---\n");
    printf("ROM at 0x8000: 0x%02X (should be 0xFF - unprogrammed)\n", 
           read_byte_new(&machine, 0x8000));
    printf("ROM at 0x8001: 0x%02X (should be 0xFF - unprogrammed)\n", 
           read_byte_new(&machine, 0x8001));
    
    // Load ROM file
    printf("\n--- Loading ROM ---\n");
    int result = load_rom_from_file(&machine, rom_file);
    
    if (result != 0) {
        fprintf(stderr, "Failed to load ROM file\n");
        cleanup_machine_with_via(&machine);
        return 1;
    }
    
    // Verify ROM contents
    printf("\n--- After Loading ROM ---\n");
    printf("ROM at 0x8000: 0x%02X (should be 0xAA)\n", 
           read_byte_new(&machine, 0x8000));
    printf("ROM at 0x8001: 0x%02X (should be 0x55)\n", 
           read_byte_new(&machine, 0x8001));
    printf("ROM at 0x8002: 0x%02X (should be 0x02)\n", 
           read_byte_new(&machine, 0x8002));
    printf("ROM at 0x80FE: 0x%02X (should be 0xBE)\n", 
           read_byte_new(&machine, 0x80FE));
    printf("ROM at 0x80FF: 0x%02X (should be 0xEF)\n", 
           read_byte_new(&machine, 0x80FF));
    printf("ROM at 0x8100: 0x%02X (should be 0xFF - beyond loaded data)\n", 
           read_byte_new(&machine, 0x8100));
    
    // Test that ROM is read-only (writes should be ignored)
    printf("\n--- Testing ROM Write Protection ---\n");
    printf("Attempting to write 0x99 to ROM at 0x8000...\n");
    write_byte_new(&machine, 0x8000, 0x99);
    uint8_t value = read_byte_new(&machine, 0x8000);
    printf("ROM at 0x8000 after write: 0x%02X (should still be 0xAA)\n", value);
    
    if (value == 0xAA) {
        printf("✓ ROM is correctly write-protected\n");
    } else {
        printf("✗ WARNING: ROM write protection may not be working\n");
    }
    
    // Dump first 32 bytes of ROM
    printf("\n--- ROM Dump (0x8000-0x801F) ---\n");
    for (int i = 0; i < 32; i++) {
        if (i % 16 == 0) {
            printf("0x%04X: ", 0x8000 + i);
        }
        printf("%02X ", read_byte_new(&machine, 0x8000 + i));
        if (i % 16 == 15) {
            printf("\n");
        }
    }
    
    // Test loading a non-existent file
    printf("\n--- Testing Error Handling ---\n");
    result = load_rom_from_file(&machine, "nonexistent.bin");
    if (result != 0) {
        printf("✓ Correctly handled missing file\n");
    }
    
    // Cleanup
    cleanup_machine_with_via(&machine);
    remove(rom_file);
    
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║  ROM Loading Test Complete                     ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    return 0;
}

/*
 * SREC (Motorola S-record) Format Loader and Single-Step Disassembler
 * 
 * Loads Motorola S-record format files and performs single-step execution
 * with disassembly output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "machine_setup.h"
#include "processor_helpers.h"

// Maximum line length for SREC files
#define MAX_LINE_LENGTH 256

// Parse a hex digit
static int parse_hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Parse two hex digits into a byte
static int parse_hex_byte(const char *str) {
    int high = parse_hex_digit(str[0]);
    int low = parse_hex_digit(str[1]);
    if (high < 0 || low < 0) return -1;
    return (high << 4) | low;
}

// Load an SREC format file into memory
// Returns the entry point address, or -1 on error
static int32_t load_srec_file(const char *filename, machine_state_t *machine) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;
    int32_t entry_point = -1;
    int total_bytes = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // Trim trailing whitespace
        int len = strlen(line);
        while (len > 0 && isspace(line[len - 1])) {
            line[--len] = '\0';
        }

        // All S-records must start with 'S'
        if (line[0] != 'S' && line[0] != 's') {
            fprintf(stderr, "Warning: Line %d doesn't start with 'S', skipping\n", line_num);
            continue;
        }

        // Get record type
        char record_type = line[1];
        
        // Parse byte count (2 hex digits)
        int byte_count = parse_hex_byte(&line[2]);
        if (byte_count < 0) {
            fprintf(stderr, "Error: Invalid byte count at line %d\n", line_num);
            fclose(fp);
            return -1;
        }

        // Process based on record type
        switch (record_type) {
            case '0': // S0 - Header record (optional, ignore)
                break;

            case '1': { // S1 - Data record with 16-bit address
                if (byte_count < 3) {
                    fprintf(stderr, "Error: S1 record too short at line %d\n", line_num);
                    fclose(fp);
                    return -1;
                }

                // Parse address (4 hex digits = 2 bytes)
                int addr_high = parse_hex_byte(&line[4]);
                int addr_low = parse_hex_byte(&line[6]);
                if (addr_high < 0 || addr_low < 0) {
                    fprintf(stderr, "Error: Invalid address at line %d\n", line_num);
                    fclose(fp);
                    return -1;
                }
                uint32_t address = (addr_high << 8) | addr_low;

                // Data bytes = byte_count - 2 (address) - 1 (checksum)
                int data_bytes = byte_count - 3;
                
                // Load data into memory
                for (int i = 0; i < data_bytes; i++) {
                    int byte_val = parse_hex_byte(&line[8 + i * 2]);
                    if (byte_val < 0) {
                        fprintf(stderr, "Error: Invalid data byte at line %d\n", line_num);
                        fclose(fp);
                        return -1;
                    }
                    
                    // Write byte to memory via region system
                    memory_region_t *region = find_current_memory_region(machine, address + i);
                    if (region) {
                        unsigned int offset = (address + i) - region->start_offset;
                        region->data[offset] = (uint8_t)byte_val;
                        total_bytes++;
                    } else {
                        fprintf(stderr, "Warning: Address 0x%04X not in any memory region\n", (unsigned int)(address + i));
                    }
                }
                break;
            }

            case '2': { // S2 - Data record with 24-bit address
                if (byte_count < 4) {
                    fprintf(stderr, "Error: S2 record too short at line %d\n", line_num);
                    fclose(fp);
                    return -1;
                }

                // Parse address (6 hex digits = 3 bytes)
                int addr_high = parse_hex_byte(&line[4]);
                int addr_mid = parse_hex_byte(&line[6]);
                int addr_low = parse_hex_byte(&line[8]);
                if (addr_high < 0 || addr_mid < 0 || addr_low < 0) {
                    fprintf(stderr, "Error: Invalid address at line %d\n", line_num);
                    fclose(fp);
                    return -1;
                }
                uint32_t address = (addr_high << 16) | (addr_mid << 8) | addr_low;

                // Data bytes = byte_count - 3 (address) - 1 (checksum)
                int data_bytes = byte_count - 4;
                
                // Load data into memory
                for (int i = 0; i < data_bytes; i++) {
                    int byte_val = parse_hex_byte(&line[10 + i * 2]);
                    if (byte_val < 0) {
                        fprintf(stderr, "Error: Invalid data byte at line %d\n", line_num);
                        fclose(fp);
                        return -1;
                    }
                    
                    // Write byte to memory via region system
                    memory_region_t *region = find_current_memory_region(machine, address + i);
                    if (region) {
                        unsigned int offset = (address + i) - region->start_offset;
                        region->data[offset] = (uint8_t)byte_val;
                        total_bytes++;
                    }
                }
                break;
            }

            case '3': { // S3 - Data record with 32-bit address
                if (byte_count < 5) {
                    fprintf(stderr, "Error: S3 record too short at line %d\n", line_num);
                    fclose(fp);
                    return -1;
                }

                // Parse address (8 hex digits = 4 bytes)
                int addr_0 = parse_hex_byte(&line[4]);
                int addr_1 = parse_hex_byte(&line[6]);
                int addr_2 = parse_hex_byte(&line[8]);
                int addr_3 = parse_hex_byte(&line[10]);
                if (addr_0 < 0 || addr_1 < 0 || addr_2 < 0 || addr_3 < 0) {
                    fprintf(stderr, "Error: Invalid address at line %d\n", line_num);
                    fclose(fp);
                    return -1;
                }
                uint32_t address = (addr_0 << 24) | (addr_1 << 16) | (addr_2 << 8) | addr_3;

                // Data bytes = byte_count - 4 (address) - 1 (checksum)
                int data_bytes = byte_count - 5;
                
                // Load data into memory
                for (int i = 0; i < data_bytes; i++) {
                    int byte_val = parse_hex_byte(&line[12 + i * 2]);
                    if (byte_val < 0) {
                        fprintf(stderr, "Error: Invalid data byte at line %d\n", line_num);
                        fclose(fp);
                        return -1;
                    }
                    
                    // Write byte to memory via region system
                    memory_region_t *region = find_current_memory_region(machine, address + i);
                    if (region) {
                        unsigned int offset = (address + i) - region->start_offset;
                        region->data[offset] = (uint8_t)byte_val;
                        total_bytes++;
                    }
                }
                break;
            }

            case '5': { // S5 - Count of S1/S2/S3 records (optional, ignore)
                break;
            }

            case '7': { // S7 - Entry point with 32-bit address
                int addr_0 = parse_hex_byte(&line[4]);
                int addr_1 = parse_hex_byte(&line[6]);
                int addr_2 = parse_hex_byte(&line[8]);
                int addr_3 = parse_hex_byte(&line[10]);
                if (addr_0 >= 0 && addr_1 >= 0 && addr_2 >= 0 && addr_3 >= 0) {
                    entry_point = (addr_0 << 24) | (addr_1 << 16) | (addr_2 << 8) | addr_3;
                }
                break;
            }

            case '8': { // S8 - Entry point with 24-bit address
                int addr_high = parse_hex_byte(&line[4]);
                int addr_mid = parse_hex_byte(&line[6]);
                int addr_low = parse_hex_byte(&line[8]);
                if (addr_high >= 0 && addr_mid >= 0 && addr_low >= 0) {
                    entry_point = (addr_high << 16) | (addr_mid << 8) | addr_low;
                }
                break;
            }

            case '9': { // S9 - Entry point with 16-bit address
                int addr_high = parse_hex_byte(&line[4]);
                int addr_low = parse_hex_byte(&line[6]);
                if (addr_high >= 0 && addr_low >= 0) {
                    entry_point = (addr_high << 8) | addr_low;
                }
                break;
            }

            default:
                fprintf(stderr, "Warning: Unknown S-record type 'S%c' at line %d\n", 
                        record_type, line_num);
                break;
        }
    }

    fclose(fp);
    printf("Loaded %d bytes from SREC file '%s'\n", total_bytes, filename);
    return entry_point;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-p ADDR] <srec_file>\n", argv[0]);
        fprintf(stderr, "  -p ADDR  Set custom program counter start address (hex)\n");
        return 1;
    }

    const char *filename = NULL;
    uint16_t custom_pc = 0;
    bool use_custom_pc = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -p option requires an address argument\n");
                return 1;
            }
            i++;
            char *endptr;
            custom_pc = (uint16_t)strtoul(argv[i], &endptr, 16);
            if (*endptr != '\0') {
                fprintf(stderr, "Error: Invalid hex address '%s'\n", argv[i]);
                return 1;
            }
            use_custom_pc = true;
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }

    // Initialize machine
    machine_state_t *machine = create_machine();
    if (!machine) {
        fprintf(stderr, "Error: Failed to initialize machine\n");
        return 1;
    }

    // Load SREC file
    int32_t entry_point = load_srec_file(filename, machine);
    
    // Determine starting PC
    if (use_custom_pc) {
        machine->processor.PC = custom_pc;
        printf("\nUsing custom PC: $%04X\n", custom_pc);
    } else if (entry_point >= 0) {
        machine->processor.PC = (uint16_t)entry_point;
        printf("\nUsing entry point from SREC: $%04X\n", (uint16_t)entry_point);
    } else {
        // Try to get reset vector from memory
        memory_region_t *region = find_current_memory_region(machine, 0xFFFC);
        if (region) {
            unsigned int offset = 0xFFFC - region->start_offset;
            uint16_t reset_vector = region->data[offset] | (region->data[offset + 1] << 8);
            if (reset_vector != 0) {
                machine->processor.PC = reset_vector;
                printf("\nUsing reset vector: $%04X\n", reset_vector);
            } else {
                fprintf(stderr, "Error: No entry point found and no reset vector set\n");
                fprintf(stderr, "Use -p option to specify starting address\n");
                destroy_machine(machine);
                return 1;
            }
        } else {
            fprintf(stderr, "Error: No entry point found and reset vector not accessible\n");
            fprintf(stderr, "Use -p option to specify starting address\n");
            destroy_machine(machine);
            return 1;
        }
    }

    printf("Starting execution at PC=$%04X\n\n", machine->processor.PC);
    
    // Print initial state
    printf("Initial state: PC=$%04X A=$%04X X=$%04X Y=$%04X SP=$%04X P=%c%c%c%c%c%c%c%c %s\n\n",
           machine->processor.PC, machine->processor.A.full, machine->processor.X, 
           machine->processor.Y, machine->processor.SP,
           (machine->processor.P & 0x80) ? 'N' : '-',
           (machine->processor.P & 0x40) ? 'V' : '-',
           (machine->processor.P & 0x20) ? 'M' : '-',
           (machine->processor.P & 0x10) ? 'X' : '-',
           (machine->processor.P & 0x08) ? 'D' : '-',
           (machine->processor.P & 0x04) ? 'I' : '-',
           (machine->processor.P & 0x02) ? 'Z' : '-',
           (machine->processor.P & 0x01) ? 'C' : '-',
           machine->processor.emulation_mode ? "[E]" : "[N]");

    // Single-step through program
    int step_count = 0;
    const int max_steps = 10000; // Safety limit
    uint16_t last_pc = 0xFFFF;
    int same_pc_count = 0;
    
    while (step_count < max_steps) {
        step_result_t *result = machine_step(machine);
        
        // Print disassembly and state
        printf("%5d. %04X: %-16s A=$%04X X=$%04X Y=$%04X SP=$%04X P=%c%c%c%c%c%c%c%c %s",
               step_count,
               (uint16_t)result->address,
               result->mnemonic,
               machine->processor.A.full,
               machine->processor.X,
               machine->processor.Y,
               machine->processor.SP,
               (machine->processor.P & 0x80) ? 'N' : '-',
               (machine->processor.P & 0x40) ? 'V' : '-',
               (machine->processor.P & 0x20) ? 'M' : '-',
               (machine->processor.P & 0x10) ? 'X' : '-',
               (machine->processor.P & 0x08) ? 'D' : '-',
               (machine->processor.P & 0x04) ? 'I' : '-',
               (machine->processor.P & 0x02) ? 'Z' : '-',
               (machine->processor.P & 0x01) ? 'C' : '-',
               machine->processor.emulation_mode ? "[E]" : "[N]");
        
        // Print operand if present
        if (result->operand_str[0] != '\0') {
            printf(" [%s]", result->operand_str);
        }
        printf("\n");

        uint8_t opcode = result->opcode;
        bool halted = result->halted;
        uint32_t result_addr = result->address;
        free_step_result(result);

        step_count++;

        // Check for halt conditions
        if (opcode == 0xDB || halted) { // STP instruction
            printf("\nProgram stopped (STP instruction)\n");
            break;
        }
        if (opcode == 0x00 && machine->processor.PC == (uint16_t)(result_addr + 1)) {
            // BRK that doesn't change PC (infinite loop)
            printf("\nProgram halted (BRK loop detected)\n");
            break;
        }
        
        // Detect infinite loops - same PC executed many times
        if (machine->processor.PC == last_pc) {
            same_pc_count++;
            if (same_pc_count >= 10) {
                printf("\nProgram stuck in loop at PC=$%04X\n", machine->processor.PC);
                break;
            }
        } else {
            same_pc_count = 0;
            last_pc = machine->processor.PC;
        }
    }

    if (step_count >= max_steps) {
        printf("\nReached maximum step count (%d)\n", max_steps);
    }

    // Print final state
    printf("\nFinal state: PC=$%04X A=$%04X X=$%04X Y=$%04X SP=$%04X P=%c%c%c%c%c%c%c%c %s\n",
           machine->processor.PC, machine->processor.A.full, machine->processor.X, 
           machine->processor.Y, machine->processor.SP,
           (machine->processor.P & 0x80) ? 'N' : '-',
           (machine->processor.P & 0x40) ? 'V' : '-',
           (machine->processor.P & 0x20) ? 'M' : '-',
           (machine->processor.P & 0x10) ? 'X' : '-',
           (machine->processor.P & 0x08) ? 'D' : '-',
           (machine->processor.P & 0x04) ? 'I' : '-',
           (machine->processor.P & 0x02) ? 'Z' : '-',
           (machine->processor.P & 0x01) ? 'C' : '-',
           machine->processor.emulation_mode ? "[E]" : "[N]");

    printf("\nTotal steps executed: %d\n", step_count);

    // Dump stack contents
    printf("\nStack dump (page 1: $0100-$01FF):\n");
    printf("SP=$%04X points to next free location\n", machine->processor.SP);
    printf("Stack contents from $01FF (bottom) to SP (top):\n");
    
    for (int addr = 0x01FF; addr >= 0x0100; addr--) {
        uint8_t value = read_byte_new(machine, addr);
        
        // Mark current SP position
        if (addr == (machine->processor.SP & 0x1FF)) {
            printf("  $%04X: $%02X  <-- SP (next push goes here)\n", addr, value);
        } else if (addr > (machine->processor.SP & 0x1FF)) {
            // This is data on the stack
            printf("  $%04X: $%02X\n", addr, value);
        } else {
            // Below SP - unused stack space (only show first few)
            if (addr >= 0x01F0 || addr < 0x0108) {
                printf("  $%04X: $%02X  (unused)\n", addr, value);
            } else if (addr == 0x0107) {
                printf("  ... (unused stack space) ...\n");
            }
        }
    }
    printf("\n");

    destroy_machine(machine);
    return 0;
}

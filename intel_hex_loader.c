/*
 * Hex file loader
 * Supports:
 * 1. Standard Intel HEX format with checksums
 * 2. Simple address:bytes format (ADDR:XX XX XX...)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "machine_setup.h"
#include "processor_helpers.h"

/*
 * Load simple address:bytes format
 * Format: ADDR:XX XX XX ...
 * where ADDR is hex address and XX are hex bytes
 */
static int load_simple_hex_file(machine_state_t *machine, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    char line[1024];
    int total_bytes = 0;
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        // Skip blank lines and comments
        char *p = line;
        while (*p && isspace(*p)) p++;
        if (*p == '\0' || *p == '#' || *p == ';') continue;

        // Parse address before colon
        unsigned int address;
        if (sscanf(p, "%x:", &address) != 1) {
            fprintf(stderr, "Warning: Line %d has invalid format - skipping\n", line_num);
            continue;
        }

        // Find colon
        while (*p && *p != ':') p++;
        if (*p != ':') {
            fprintf(stderr, "Warning: Line %d missing colon - skipping\n", line_num);
            continue;
        }
        p++; // Skip colon

        // Parse hex bytes
        while (*p) {
            // Skip whitespace
            while (*p && isspace(*p)) p++;
            if (*p == '\0' || *p == '#' || *p == ';') break;

            // Parse hex byte
            unsigned int byte_val;
            if (sscanf(p, "%2x", &byte_val) != 1) {
                fprintf(stderr, "Warning: Invalid hex byte at line %d\n", line_num);
                break;
            }

            // Write to memory via region system
            memory_region_t *region = find_current_memory_region(machine, address);
            if (region) {
                unsigned int offset = address - region->start_offset;
                region->data[offset] = (unsigned char)byte_val;
                total_bytes++;
            } else {
                fprintf(stderr, "Warning: Address 0x%04X not in any memory region\n", address);
            }

            address++;

            // Move to next hex digit pair
            if (isxdigit(*p)) p++;
            if (isxdigit(*p)) p++;
        }
    }

    fclose(fp);
    printf("Loaded %d bytes from simple hex file '%s'\n", total_bytes, filename);
    return total_bytes;
}

/*
 * Parse Intel HEX format:
 * :LLAAAATT[DD...]CC
 * LL = byte count
 * AAAA = address
 * TT = record type (00=data, 01=EOF, 02=extended segment, 04=extended linear, 05=start linear)
 * DD = data bytes
 * CC = checksum
 */
static int load_intel_hex_file(machine_state_t *machine, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    char line[1024];
    int total_bytes = 0;
    int line_num = 0;
    unsigned int extended_address = 0; // For extended addressing modes

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        // Skip blank lines and comments
        char *p = line;
        while (*p && isspace(*p)) p++;
        if (*p == '\0' || *p == '#' || *p == ';') continue;

        // Intel HEX lines start with ':'
        if (*p != ':') {
            fprintf(stderr, "Warning: Line %d doesn't start with ':' - skipping\n", line_num);
            continue;
        }
        p++;

        // Parse byte count (2 hex digits)
        unsigned int byte_count, address, record_type;
        if (sscanf(p, "%02X", &byte_count) != 1) {
            fprintf(stderr, "Error: Invalid byte count at line %d\n", line_num);
            continue;
        }
        p += 2;

        // Parse address (4 hex digits)
        if (sscanf(p, "%04X", &address) != 1) {
            fprintf(stderr, "Error: Invalid address at line %d\n", line_num);
            continue;
        }
        p += 4;

        // Parse record type (2 hex digits)
        if (sscanf(p, "%02X", &record_type) != 1) {
            fprintf(stderr, "Error: Invalid record type at line %d\n", line_num);
            continue;
        }
        p += 2;

        // Calculate checksum as we go
        unsigned int checksum = byte_count + (address >> 8) + (address & 0xFF) + record_type;

        // Handle different record types
        switch (record_type) {
            case 0x00: { // Data record
                unsigned int full_address = extended_address + address;
                
                for (unsigned int i = 0; i < byte_count; i++) {
                    unsigned int byte_val;
                    if (sscanf(p, "%02X", &byte_val) != 1) {
                        fprintf(stderr, "Error: Invalid data byte at line %d, position %d\n", line_num, i);
                        break;
                    }
                    p += 2;
                    checksum += byte_val;

                    // Write byte to memory via region system
                    memory_region_t *region = find_current_memory_region(machine, full_address);
                    if (region) {
                        unsigned int offset = full_address - region->start_offset;
                        region->data[offset] = (unsigned char)byte_val;
                        total_bytes++;
                    } else {
                        fprintf(stderr, "Warning: Address 0x%04X not in any memory region\n", full_address);
                    }
                    full_address++;
                }
                break;
            }
            
            case 0x01: // End of file
                // Verify checksum and finish
                goto done;
                
            case 0x02: { // Extended Segment Address (segment base = data * 16)
                unsigned int segment;
                if (sscanf(p, "%04X", &segment) != 1) {
                    fprintf(stderr, "Error: Invalid extended segment at line %d\n", line_num);
                    break;
                }
                p += 4;
                checksum += (segment >> 8) + (segment & 0xFF);
                extended_address = segment << 4;
                break;
            }
            
            case 0x04: { // Extended Linear Address (upper 16 bits of address)
                unsigned int upper_addr;
                if (sscanf(p, "%04X", &upper_addr) != 1) {
                    fprintf(stderr, "Error: Invalid extended linear address at line %d\n", line_num);
                    break;
                }
                p += 4;
                checksum += (upper_addr >> 8) + (upper_addr & 0xFF);
                extended_address = upper_addr << 16;
                break;
            }
            
            case 0x05: { // Start Linear Address (entry point for 32-bit systems)
                unsigned int entry;
                if (sscanf(p, "%08X", &entry) != 1) {
                    fprintf(stderr, "Warning: Invalid start address at line %d\n", line_num);
                }
                // For 65816, we don't use this, but skip the data bytes
                p += 8;
                break;
            }
            
            default:
                fprintf(stderr, "Warning: Unknown record type 0x%02X at line %d\n", record_type, line_num);
                break;
        }

        // Verify checksum
        unsigned int file_checksum;
        if (sscanf(p, "%02X", &file_checksum) == 1) {
            checksum = (~checksum + 1) & 0xFF; // Two's complement
            if (checksum != file_checksum) {
                fprintf(stderr, "Warning: Checksum mismatch at line %d (expected 0x%02X, got 0x%02X)\n", 
                       line_num, checksum, file_checksum);
            }
        }
    }

done:
    fclose(fp);
    printf("Loaded %d bytes from Intel HEX file '%s'\n", total_bytes, filename);
    return total_bytes;
}

/*
 * Detect file format and load accordingly
 * Returns number of bytes loaded, or -1 on error
 */
static int auto_load_hex_file(machine_state_t *machine, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    // Read first non-comment, non-blank line to detect format
    char line[1024];
    int is_intel_hex = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p && isspace(*p)) p++;
        
        // Skip blank lines and comments
        if (*p == '\0' || *p == '#' || *p == ';') continue;
        
        // Intel HEX lines start with ':'
        if (*p == ':') {
            is_intel_hex = 1;
        }
        break;
    }
    fclose(fp);

    // Load using appropriate format
    if (is_intel_hex) {
        printf("Detected Intel HEX format\n");
        return load_intel_hex_file(machine, filename);
    } else {
        printf("Detected simple address:bytes format\n");
        return load_simple_hex_file(machine, filename);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [options] <hex_file>\n", argv[0]);
        fprintf(stderr, "\nLoads a hex file and performs single-step execution with disassembly.\n");
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "  -i           Force Intel HEX format\n");
        fprintf(stderr, "  -s           Force simple address:bytes format\n");
        fprintf(stderr, "  -p ADDR      Set starting PC address (hex, e.g., -p 2000)\n");
        fprintf(stderr, "  (no option)  Auto-detect format, use reset vector\n");
        fprintf(stderr, "\nSupported formats:\n");
        fprintf(stderr, "  Intel HEX: :LLAAAATTDDDDCC (standard format with checksums)\n");
        fprintf(stderr, "  Simple:    ADDR:XX XX XX ... (address followed by hex bytes)\n");
        return 1;
    }

    const char *filename = NULL;
    enum { AUTO, INTEL_HEX, SIMPLE_HEX } format = AUTO;
    int custom_pc = -1;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            format = INTEL_HEX;
        } else if (strcmp(argv[i], "-s") == 0) {
            format = SIMPLE_HEX;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -p requires an address argument\n");
                return 1;
            }
            i++;
            if (sscanf(argv[i], "%x", &custom_pc) != 1) {
                fprintf(stderr, "Error: Invalid address '%s'\n", argv[i]);
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return 1;
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
    initialize_machine(machine);

    // Load the hex file
    int bytes_loaded;
    if (format == INTEL_HEX) {
        printf("Using Intel HEX format\n");
        bytes_loaded = load_intel_hex_file(machine, filename);
    } else if (format == SIMPLE_HEX) {
        printf("Using simple address:bytes format\n");
        bytes_loaded = load_simple_hex_file(machine, filename);
    } else {
        bytes_loaded = auto_load_hex_file(machine, filename);
    }
    
    if (bytes_loaded < 0) {
        destroy_machine(machine);
        return 1;
    }

    if (bytes_loaded == 0) {
        fprintf(stderr, "Warning: No bytes loaded from file\n");
        destroy_machine(machine);
        return 1;
    }

    // Read reset vector to set PC
    unsigned int reset_vector;
    if (custom_pc >= 0) {
        reset_vector = custom_pc;
        printf("\nUsing custom PC: $%04X\n", reset_vector);
    } else {
        unsigned char reset_lo = read_byte_new(machine, 0xFFFC);
        unsigned char reset_hi = read_byte_new(machine, 0xFFFD);
        reset_vector = reset_lo | (reset_hi << 8);
        printf("\nReset vector: $%04X\n", reset_vector);
    }
    
    machine->processor.PC = reset_vector;
    printf("Starting execution at PC=$%04X\n\n", machine->processor.PC);

    // Print initial state
    printf("Initial state: PC=$%04X A=$%04X X=$%04X Y=$%04X P=%c%c%c%c%c%c%c%c\n\n",
           machine->processor.PC, machine->processor.A.full, machine->processor.X, 
           machine->processor.Y,
           (machine->processor.P & 0x80) ? '1' : '0',
           (machine->processor.P & 0x40) ? '1' : '0',
           (machine->processor.P & 0x20) ? '1' : '0',
           (machine->processor.P & 0x10) ? '1' : '0',
           (machine->processor.P & 0x08) ? '1' : '0',
           (machine->processor.P & 0x04) ? '1' : '0',
           (machine->processor.P & 0x02) ? '1' : '0',
           (machine->processor.P & 0x01) ? '1' : '0');

    // Single-step through program
    int step_count = 0;
    int max_steps = 10000; // Safety limit
    uint16_t last_pc = 0xFFFF;
    int same_pc_count = 0;

    while (step_count < max_steps) {
        step_result_t *result = machine_step(machine);
        if (!result) {
            fprintf(stderr, "Error: machine_step failed\n");
            break;
        }

        // Print disassembly and state
        printf("%5d. %04X: %-16s A=$%04X X=$%04X Y=$%04X P=%c%c%c%c%c%c%c%c",
               step_count,
               result->address,
               result->mnemonic,
               machine->processor.A.full,
               machine->processor.X,
               machine->processor.Y,
               (machine->processor.P & 0x80) ? '1' : '0',
               (machine->processor.P & 0x40) ? '1' : '0',
               (machine->processor.P & 0x20) ? '1' : '0',
               (machine->processor.P & 0x10) ? '1' : '0',
               (machine->processor.P & 0x08) ? '1' : '0',
               (machine->processor.P & 0x04) ? '1' : '0',
               (machine->processor.P & 0x02) ? '1' : '0',
               (machine->processor.P & 0x01) ? '1' : '0');

        if (result->operand_str[0] != '\0') {
            printf(" [%s]", result->operand_str);
        }
        printf("\n");

        uint8_t opcode = result->opcode;
        bool halted = result->halted;
        bool waiting = result->waiting;
        uint32_t result_addr = result->address;
        free_step_result(result);

        step_count++;

        // Check for halt conditions
        if (opcode == 0xDB || halted) { // STP instruction
            printf("\nProgram stopped (STP instruction)\n");
            break;
        }
        if (waiting) {
            printf("\nProcessor waiting (WAI instruction)\n");
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
        printf("\nReached maximum step limit (%d steps)\n", max_steps);
    }

    printf("\nFinal state: PC=$%04X A=$%04X X=$%04X Y=$%04X P=%c%c%c%c%c%c%c%c\n",
           machine->processor.PC, machine->processor.A.full, machine->processor.X, 
           machine->processor.Y,
           (machine->processor.P & 0x80) ? '1' : '0',
           (machine->processor.P & 0x40) ? '1' : '0',
           (machine->processor.P & 0x20) ? '1' : '0',
           (machine->processor.P & 0x10) ? '1' : '0',
           (machine->processor.P & 0x08) ? '1' : '0',
           (machine->processor.P & 0x04) ? '1' : '0',
           (machine->processor.P & 0x02) ? '1' : '0',
           (machine->processor.P & 0x01) ? '1' : '0');

    printf("\nTotal steps executed: %d\n", step_count);

    destroy_machine(machine);
    return 0;
}

#include "machine_setup.h"
#include "machine.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "via6522.h"
#include "board_fifo.h"
#include "pia6521.h"
#include "acia6551.h"

// Global ACIA instance (at 0x7F80)
static acia6551_t g_acia;
static bool g_acia_initialized = false;

// Global PIA instance (at 0x7FA0)
static pia6521_t g_pia;
static bool g_pia_initialized = false;

// Global VIA instance (standalone at 0x7FC0)
static via6522_t g_via;
static bool g_via_initialized = false;

// Global board FIFO instance (VIA+FT245 at 0x7FE0)
static fifo_t *g_board_fifo = NULL;

void initialize_processor(processor_state_t *state) {
    state->A.full = 0;
    state->X = 0;
    state->Y = 0;
    state->PC = 0;
    state->SP = 0x1FF;            // Stack Pointer starts at 0x1FF in emulation mode
    state->DP = 0;
    state->P = 0x34;              // Default status register value -- this is kinda arbitrary
    state->PBR = 0;               // Start in bank 0
    state->DBR = 0;               // Start in bank 0
    state->emulation_mode = true; // Start in emulation mode
}

void reset_processor(processor_state_t *state) {
    state->A.full = 0;
    state->X = 0;
    state->Y = 0;
    state->PC = 0x0000;           // Reset Program Counter to 0
    state->SP = 0x1FF;             // Stack Pointer reset value
    state->DP = 0;
    state->P = 0x34;              // Reset status register value
    state->PBR = 0;               // Start in bank 0
    state->DBR = 0;               // Start in bank 0
    state->emulation_mode = true; // Reset to emulation mode
}

uint8_t read_byte_from_region_nodev(memory_region_t *region, uint16_t address) {
    uint16_t addr = address - region->start_offset;
    if (region->flags & MEM_READONLY || region->flags & MEM_READWRITE) {
        return region->data[addr];
    }
    return 0; // Cannot read from this region
}

uint16_t read_word_from_region_nodev(memory_region_t *region, uint16_t address) {
    if (region->flags & MEM_READONLY || region->flags & MEM_READWRITE) {
        uint8_t low_byte = read_byte_from_region_nodev(region, address);
        uint8_t high_byte = read_byte_from_region_nodev(region, (address + 1) & 0xFFFF);
        return (high_byte << 8) | low_byte;
    }
    return 0; // Cannot read from this region
}

void write_byte_to_region_nodev(memory_region_t *region, uint16_t address, uint8_t value) {
    uint16_t addr = address - region->start_offset;
    if (region->flags & MEM_READWRITE) {
        region->data[address - region->start_offset] = value;
    }
    // If region is read-only or device, do nothing
}

void write_word_to_region_nodev(memory_region_t *memory, uint16_t address, uint16_t value) {
    write_byte_to_region_nodev(memory, address, value & 0xFF);               // Low byte
    write_byte_to_region_nodev(memory, (address + 1) & 0xFFFF, (value >> 8) & 0xFF); // High byte
}

uint8_t read_byte_from_region_dev(memory_region_t *region, uint16_t address) {
    // ACIA is mapped at 0x7F80-0x7F83 (4 registers)
    if (address >= 0x7F80 && address <= 0x7F83) {
        if (!g_acia_initialized) {
            acia6551_init(&g_acia);
            g_acia_initialized = true;
        }
        uint8_t reg = address & 0x03;  // Get register offset (0-3)
        return acia6551_read(&g_acia, reg);
    }
    
    // PIA is mapped at 0x7FA0-0x7FA3 (4 registers)
    if (address >= 0x7FA0 && address <= 0x7FA3) {
        if (!g_pia_initialized) {
            pia6521_init(&g_pia);
            g_pia_initialized = true;
        }
        uint8_t reg = address & 0x03;  // Get register offset (0-3)
        return pia6521_read(&g_pia, reg);
    }
    
    // Standalone VIA is mapped at 0x7FC0-0x7FCF (16 registers)
    if (address >= 0x7FC0 && address <= 0x7FCF) {
        if (!g_via_initialized) {
            via6522_init(&g_via);
            g_via_initialized = true;
        }
        uint8_t reg = address & 0x0F;  // Get register offset (0-15)
        return via6522_read(&g_via, reg);
    }
    
    // Board FIFO (VIA+FT245) is mapped at 0x7FE0-0x7FEF (16 registers)
    if (address >= 0x7FE0 && address <= 0x7FEF) {
        if (!g_board_fifo) {
            return 0xFF; // No device present
        }
        uint8_t reg = address & 0x0F;  // Get register offset (0-15)
        return board_fifo_read_via(g_board_fifo, reg);
    }
    
    // Default for other devices
    return 0xFF;
}

uint16_t read_word_from_region_dev(memory_region_t *region, uint16_t address) {
    // Read word as two bytes (little-endian)
    uint8_t low = read_byte_from_region_dev(region, address);
    uint8_t high = read_byte_from_region_dev(region, address + 1);
    return (high << 8) | low;
}

void write_byte_to_region_dev(memory_region_t *region, uint16_t address, uint8_t value) {
    // ACIA is mapped at 0x7F80-0x7F83 (4 registers)
    if (address >= 0x7F80 && address <= 0x7F83) {
        if (!g_acia_initialized) {
            acia6551_init(&g_acia);
            g_acia_initialized = true;
        }
        uint8_t reg = address & 0x03;  // Get register offset (0-3)
        acia6551_write(&g_acia, reg, value);
        return;
    }
    
    // PIA is mapped at 0x7FA0-0x7FA3 (4 registers)
    if (address >= 0x7FA0 && address <= 0x7FA3) {
        if (!g_pia_initialized) {
            pia6521_init(&g_pia);
            g_pia_initialized = true;
        }
        uint8_t reg = address & 0x03;  // Get register offset (0-3)
        pia6521_write(&g_pia, reg, value);
        return;
    }
    
    // Standalone VIA is mapped at 0x7FC0-0x7FCF (16 registers)
    if (address >= 0x7FC0 && address <= 0x7FCF) {
        if (!g_via_initialized) {
            via6522_init(&g_via);
            g_via_initialized = true;
        }
        uint8_t reg = address & 0x0F;  // Get register offset (0-15)
        via6522_write(&g_via, reg, value);
        return;
    }
    
    // Board FIFO (VIA+FT245) is mapped at 0x7FE0-0x7FEF (16 registers)
    if (address >= 0x7FE0 && address <= 0x7FEF) {
        if (!g_board_fifo) {
            return; // No device present
        }
        uint8_t reg = address & 0x0F;  // Get register offset (0-15)
        board_fifo_write_via(g_board_fifo, reg, value);
        return;
    }
    
    // Default for other devices - ignore write
}

void write_word_to_region_dev(memory_region_t *region, uint16_t address, uint16_t value) {
    // Write word as two bytes (little-endian)
    write_byte_to_region_dev(region, address, value & 0xFF);
    write_byte_to_region_dev(region, address + 1, (value >> 8) & 0xFF);
}

void initialize_machine(machine_state_t *machine) {
    initialize_processor(&machine->processor);
    for (int i = 0; i < 64; i++) {
        machine->memory[i] = NULL; // Initialize memory pointers to NULL
    }

    machine->memory[0] = (uint8_t*)malloc(65536 * sizeof(uint8_t)); // Allocate 64KB for bank 0

    g_board_fifo = init_board_fifo();
    
    for (int i = 0; i < 256; i++) {
        machine->memory_banks[i] = NULL; // Initialize memory banks to NULL
    }
    machine->memory_banks[0] = (memory_bank_t*)malloc(sizeof(memory_bank_t));

    /*
     * what follows is all experimental design work for memory regions and banks
     */
    memory_bank_t *bank0 = machine->memory_banks[0];
    memory_region_t *region0 = (memory_region_t*)malloc(sizeof(memory_region_t));
    memory_region_t *region1 = (memory_region_t*)malloc(sizeof(memory_region_t));
    memory_region_t *region2 = (memory_region_t*)malloc(sizeof(memory_region_t));

    region0->start_offset = 0x0000;
    region0->end_offset = 0x7F7F;
    region0->data = (uint8_t *)malloc(0x7F80 * sizeof(uint8_t));
    region0->read_byte = read_byte_from_region_nodev;
    region0->write_byte = write_byte_to_region_nodev;
    region0->read_word = read_word_from_region_nodev;
    region0->write_word = write_word_to_region_nodev;
    region0->flags = MEM_READWRITE;

    // Region: ACIA at 0x7F80-0x7F83 (4 bytes)
    memory_region_t *region_acia = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_acia->start_offset = 0x7F80;
    region_acia->end_offset = 0x7F83;
    region_acia->data = NULL; // No backing storage for devices
    region_acia->read_byte = read_byte_from_region_dev;
    region_acia->write_byte = write_byte_to_region_dev;
    region_acia->read_word = read_word_from_region_dev;
    region_acia->write_word = write_word_to_region_dev;
    region_acia->flags = MEM_DEVICE;

    // Region: Gap between ACIA and PIA (0x7F84-0x7F9F)
    memory_region_t *region_gap_acia_pia = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_gap_acia_pia->start_offset = 0x7F84;
    region_gap_acia_pia->end_offset = 0x7F9F;
    region_gap_acia_pia->data = NULL;
    region_gap_acia_pia->read_byte = read_byte_from_region_dev;
    region_gap_acia_pia->write_byte = write_byte_to_region_dev;
    region_gap_acia_pia->read_word = read_word_from_region_dev;
    region_gap_acia_pia->write_word = write_word_to_region_dev;
    region_gap_acia_pia->flags = MEM_DEVICE;

    // Region: PIA at 0x7FA0-0x7FA3 (4 bytes)
    memory_region_t *region_pia = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_pia->start_offset = 0x7FA0;
    region_pia->end_offset = 0x7FA3;
    region_pia->data = NULL; // No backing storage for devices
    region_pia->read_byte = read_byte_from_region_dev;
    region_pia->write_byte = write_byte_to_region_dev;
    region_pia->read_word = read_word_from_region_dev;
    region_pia->write_word = write_word_to_region_dev;
    region_pia->flags = MEM_DEVICE;

    // Region: Gap between PIA and VIA (0x7FA4-0x7FBF)
    memory_region_t *region_gap1 = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_gap1->start_offset = 0x7FA4;
    region_gap1->end_offset = 0x7FBF;
    region_gap1->data = NULL;
    region_gap1->read_byte = read_byte_from_region_dev;
    region_gap1->write_byte = write_byte_to_region_dev;
    region_gap1->read_word = read_word_from_region_dev;
    region_gap1->write_word = write_word_to_region_dev;
    region_gap1->flags = MEM_DEVICE;

    // Region: Standalone VIA at 0x7FC0-0x7FCF (16 bytes)
    memory_region_t *region_via = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_via->start_offset = 0x7FC0;
    region_via->end_offset = 0x7FCF;
    region_via->data = NULL; // No backing storage for devices
    region_via->read_byte = read_byte_from_region_dev;
    region_via->write_byte = write_byte_to_region_dev;
    region_via->read_word = read_word_from_region_dev;
    region_via->write_word = write_word_to_region_dev;
    region_via->flags = MEM_DEVICE;

    // Region: Device gap 0x7FD0-0x7FDF
    memory_region_t *region_gap = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_gap->start_offset = 0x7FD0;
    region_gap->end_offset = 0x7FDF;
    region_gap->data = NULL;
    region_gap->read_byte = read_byte_from_region_dev;
    region_gap->write_byte = write_byte_to_region_dev;
    region_gap->read_word = read_word_from_region_dev;
    region_gap->write_word = write_word_to_region_dev;
    region_gap->flags = MEM_DEVICE;

    // Region: Board FIFO (VIA+FT245) at 0x7FE0-0x7FEF (16 bytes)
    memory_region_t *region_board_fifo = (memory_region_t*)malloc(sizeof(memory_region_t));
    region_board_fifo->start_offset = 0x7FE0;
    region_board_fifo->end_offset = 0x7FEF;
    region_board_fifo->data = NULL; // No backing storage for devices
    region_board_fifo->read_byte = read_byte_from_region_dev;
    region_board_fifo->write_byte = write_byte_to_region_dev;
    region_board_fifo->read_word = read_word_from_region_dev;
    region_board_fifo->write_word = write_word_to_region_dev;
    region_board_fifo->flags = MEM_DEVICE;

    region1->start_offset = 0x7FF0;
    region1->end_offset = 0x7FFF;
    region1->data = (uint8_t *)malloc(16 * sizeof(uint8_t));
    region1->read_byte = read_byte_from_region_dev;  // Default read/write functions can be set later
    region1->write_byte = write_byte_to_region_dev;
    region1->read_word = read_word_from_region_dev;
    region1->write_word = write_word_to_region_dev;
    region1->flags = MEM_DEVICE;

    region2->start_offset = 0x8000;
    region2->end_offset = 0xFFFF;
    region2->data = (uint8_t *)malloc(32768 * sizeof(uint8_t));
    region2->read_byte = read_byte_from_region_nodev;  // Default read/write functions can be set later
    region2->write_byte = write_byte_to_region_nodev;
    region2->read_word = read_word_from_region_nodev;
    region2->write_word = write_word_to_region_nodev;
    region2->flags = MEM_READONLY;
    
    // Link all regions together
    region0->next = region_acia;
    region_acia->next = region_gap_acia_pia;
    region_gap_acia_pia->next = region_pia;
    region_pia->next = region_gap1;
    region_gap1->next = region_via;
    region_via->next = region_gap;
    region_gap->next = region_board_fifo;
    region_board_fifo->next = region1;
    region1->next = region2;
    region2->next = NULL;
    bank0->regions = region0;

}

// Clock devices (call this in your main emulation loop)
void machine_clock_devices(machine_state_t *machine) {
    // Clock ACIA at 0x7F80
    if (g_acia_initialized) {
        acia6551_clock(&g_acia, 1);
    }
    
    // PIA at 0x7FA0 doesn't need clocking (no timers)
    
    // Clock standalone VIA at 0x7FC0
    if (g_via_initialized) {
        via6522_clock(&g_via);
    }
    
    // Clock board FIFO (VIA+FT245) at 0x7FE0
    if (g_board_fifo) {
        board_fifo_clock(g_board_fifo);
    }
}

// Cleanup
void cleanup_machine_with_via(machine_state_t *machine) {
    if (g_board_fifo) {
        free_board_fifo(g_board_fifo);
        g_board_fifo = NULL;
    }
    
    // Free memory regions
    if (machine->memory_banks[0]) {
        memory_region_t *region = machine->memory_banks[0]->regions;
        while (region) {
            memory_region_t *next = region->next;
            if (region->data) {
                free(region->data);
            }
            free(region);
            region = next;
        }
        free(machine->memory_banks[0]);
        machine->memory_banks[0] = NULL;
    }
}

// Example: USB side operations (for testing/debugging)
void usb_send_byte_to_cpu(uint8_t data) {
    if (g_board_fifo) {
        board_fifo_usb_send_to_cpu(g_board_fifo, data);
    }
}

uint8_t usb_receive_byte_from_cpu(void) {
    uint8_t data = 0;
    if (g_board_fifo) {
        board_fifo_usb_receive_from_cpu(g_board_fifo, &data);
    }
    return data;
}

// Get standalone VIA instance for direct access (e.g., setting callbacks)
via6522_t* get_via_instance(void) {
    if (!g_via_initialized) {
        via6522_init(&g_via);
        g_via_initialized = true;
    }
    return &g_via;
}

// Get PIA instance for direct access (e.g., setting callbacks)
pia6521_t* get_pia_instance(void) {
    if (!g_pia_initialized) {
        pia6521_init(&g_pia);
        g_pia_initialized = true;
    }
    return &g_pia;
}

// Get ACIA instance for direct access (e.g., setting callbacks)
acia6551_t* get_acia_instance(void) {
    if (!g_acia_initialized) {
        acia6551_init(&g_acia);
        g_acia_initialized = true;
    }
    return &g_acia;
}

// Load a binary file into the ROM region (0x8000-0xFFFF)
// Returns: 0 on success, -1 on error
int load_rom_from_file(machine_state_t *machine, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open ROM file '%s'\n", filename);
        return -1;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // ROM region is 0x8000-0xFFFF (32KB)
    const size_t rom_size = 32768;
    
    if (file_size > rom_size) {
        fprintf(stderr, "Warning: ROM file '%s' is %ld bytes, truncating to %zu bytes\n",
                filename, file_size, rom_size);
        file_size = rom_size;
    }
    
    // Find the ROM region (region2 at 0x8000-0xFFFF)
    memory_bank_t *bank0 = machine->memory_banks[0];
    if (!bank0) {
        fprintf(stderr, "Error: Memory bank 0 not initialized\n");
        fclose(fp);
        return -1;
    }
    
    memory_region_t *rom_region = NULL;
    memory_region_t *region = bank0->regions;
    while (region) {
        if (region->start_offset == 0x8000 && region->end_offset == 0xFFFF) {
            rom_region = region;
            break;
        }
        region = region->next;
    }
    
    if (!rom_region || !rom_region->data) {
        fprintf(stderr, "Error: ROM region not found or not initialized\n");
        fclose(fp);
        return -1;
    }
    
    // Clear ROM to 0xFF (typical unprogrammed ROM state)
    memset(rom_region->data, 0xFF, rom_size);
    
    // Read file into ROM
    size_t bytes_read = fread(rom_region->data, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read complete ROM file (read %zu of %ld bytes)\n",
                bytes_read, file_size);
        return -1;
    }
    
    printf("Loaded %zu bytes from '%s' into ROM at 0x8000-0x%04X\n",
           bytes_read, filename, 0x8000 + (int)bytes_read - 1);
    
    return 0;
}

void reset_machine(machine_state_t *machine) {
    reset_processor(&machine->processor);
    
    for (int i = 0; i < 64; i++) {
        if (machine->memory[i] != NULL) {
            free(machine->memory[i]);
            machine->memory[i] = NULL;
        }
    }
    machine->memory[0] = (uint8_t*)malloc(65536 * sizeof(uint8_t)); // Reallocate 64KB for bank 0
}

machine_state_t* create_machine() {
    machine_state_t *machine = (machine_state_t*)malloc(sizeof(machine_state_t));
    initialize_machine(machine);
    return machine;
}

void destroy_machine(machine_state_t *machine) {
    for (int i = 0; i < 64; i++) {
        if (machine->memory[i] != NULL) {
            free(machine->memory[i]);
        }
    }
    free(machine);
}

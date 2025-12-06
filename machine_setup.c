#include "machine_setup.h"
#include "machine.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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
    // Device-specific read logic can be implemented here
    return 0; // Placeholder
}

uint16_t read_word_from_region_dev(memory_region_t *region, uint16_t address) {
    // Device-specific read logic can be implemented here
    return 0; // Placeholder
}

void write_byte_to_region_dev(memory_region_t *region, uint16_t address, uint8_t value) {
    // Device-specific write logic can be implemented here
}

void write_word_to_region_dev(memory_region_t *region, uint16_t address, uint16_t value) {
    // Device-specific write logic can be implemented here
}

void initialize_machine(machine_state_t *machine) {
    initialize_processor(&machine->processor);
    for (int i = 0; i < 64; i++) {
        machine->memory[i] = NULL; // Initialize memory pointers to NULL
    }

    machine->memory[0] = (uint8_t*)malloc(65536 * sizeof(uint8_t)); // Allocate 64KB for bank 0

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
    region0->end_offset = 0x7EFF;
    region0->data = (uint8_t *)malloc(32768 * sizeof(uint8_t));
    region0->read_byte = read_byte_from_region_nodev;  // Default read/write functions can be set later
    region0->write_byte = write_byte_to_region_nodev;
    region0->read_word = read_word_from_region_nodev;
    region0->write_word = write_word_to_region_nodev;
    region0->flags = MEM_READWRITE;
    region1->start_offset = 0x7F00;
    region1->end_offset = 0x7FFF;
    region1->data = (uint8_t *)malloc(256 * sizeof(uint8_t));
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
    region0->next = region1;
    region1->next = region2;
    region2->next = NULL;
    bank0->regions = region0;

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

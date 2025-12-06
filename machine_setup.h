#ifndef __MACHINE_SETUP_H__
#define __MACHINE_SETUP_H__
#include "machine.h"

void initialize_processor(processor_state_t *state);
void reset_processor(processor_state_t *state);
void initialize_machine(machine_state_t *machine);
void reset_machine(machine_state_t *machine);
machine_state_t* create_machine();
void destroy_machine(machine_state_t *machine);
uint8_t read_byte_from_region_nodev(memory_region_t *region, uint16_t address);
uint16_t read_word_from_region_nodev(memory_region_t *region, uint16_t address);
void write_byte_to_region_nodev(memory_region_t *region, uint16_t address, uint8_t value);
void write_word_to_region_nodev(memory_region_t *memory, uint16_t address, uint16_t value);
uint8_t read_byte_from_region_dev(memory_region_t *region, uint16_t address);
uint16_t read_word_from_region_dev(memory_region_t *region, uint16_t address);
void write_byte_to_region_dev(memory_region_t *region, uint16_t address, uint8_t value);
void write_word_to_region_dev(memory_region_t *region, uint16_t address, uint16_t value);
#endif // __MACHINE_SETUP_H__
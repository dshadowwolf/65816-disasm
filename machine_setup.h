#ifndef __MACHINE_SETUP_H__
#define __MACHINE_SETUP_H__
#include "machine.h"
#include "via6522.h"
#include "pia6521.h"
#include "acia6551.h"

void initialize_processor(processor_state_t *state);
void reset_processor(processor_state_t *state);
void initialize_machine(machine_state_t *machine);
void reset_machine(machine_state_t *machine);
machine_state_t* create_machine();
void destroy_machine(machine_state_t *machine);
void machine_clock_devices(machine_state_t *machine);
void cleanup_machine_with_via(machine_state_t *machine);
void usb_send_byte_to_cpu(uint8_t data);
uint8_t usb_receive_byte_from_cpu(void);
via6522_t* get_via_instance(void);
pia6521_t* get_pia_instance(void);
acia6551_t* get_acia_instance(void);
uint8_t read_byte_from_region_nodev(memory_region_t *region, uint16_t address);
uint16_t read_word_from_region_nodev(memory_region_t *region, uint16_t address);
void write_byte_to_region_nodev(memory_region_t *region, uint16_t address, uint8_t value);
void write_word_to_region_nodev(memory_region_t *memory, uint16_t address, uint16_t value);
uint8_t read_byte_from_region_dev(memory_region_t *region, uint16_t address);
uint16_t read_word_from_region_dev(memory_region_t *region, uint16_t address);
void write_byte_to_region_dev(memory_region_t *region, uint16_t address, uint8_t value);
void write_word_to_region_dev(memory_region_t *region, uint16_t address, uint16_t value);
#endif // __MACHINE_SETUP_H__
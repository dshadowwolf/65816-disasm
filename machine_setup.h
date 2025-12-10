#ifndef __MACHINE_SETUP_H__
#define __MACHINE_SETUP_H__
#include "machine.h"
#include "via6522.h"
#include "pia6521.h"
#include "acia6551.h"

// Structure to hold single-step execution results
typedef struct step_result_s {
    uint32_t address;          // Address where instruction was executed (PBR:PC)
    uint8_t opcode;            // Opcode byte
    uint16_t operand;          // Operand bytes (if any)
    uint8_t instruction_size;  // Total size of instruction (1-4 bytes)
    char mnemonic[8];          // Instruction mnemonic (e.g., "LDA")
    char operand_str[32];      // Formatted operand string
    bool halted;               // True if processor halted (STP instruction)
    bool waiting;              // True if processor waiting (WAI instruction)
} step_result_t;

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
int load_rom_from_file(machine_state_t *machine, const char *filename);
uint8_t read_byte_from_region_nodev(memory_region_t *region, uint16_t address);
uint16_t read_word_from_region_nodev(memory_region_t *region, uint16_t address);
void write_byte_to_region_nodev(memory_region_t *region, uint16_t address, uint8_t value);
void write_word_to_region_nodev(memory_region_t *memory, uint16_t address, uint16_t value);
uint8_t read_byte_from_region_dev(memory_region_t *region, uint16_t address);
uint16_t read_word_from_region_dev(memory_region_t *region, uint16_t address);
void write_byte_to_region_dev(memory_region_t *region, uint16_t address, uint8_t value);
void write_word_to_region_dev(memory_region_t *region, uint16_t address, uint16_t value);

// Single-step execution with disassembly
step_result_t* machine_step(machine_state_t *machine);
void free_step_result(step_result_t *result);

#endif // __MACHINE_SETUP_H__
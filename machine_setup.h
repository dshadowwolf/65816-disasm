#ifndef __MACHINE_SETUP_H__
#define __MACHINE_SETUP_H__
#include "machine.h"

void initialize_processor(processor_state_t *state);
void reset_processor(processor_state_t *state);
void initialize_machine(machine_state_t *machine);
void reset_machine(machine_state_t *machine);
machine_state_t* create_machine();
void destroy_machine(machine_state_t *machine);

#endif // __MACHINE_SETUP_H__
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "state.h"

#include "machine.h"

#define E_FLAG INTERRUPT_DISABLE
typedef struct p_state_s {
    unsigned char flags;
    unsigned int start;
} p_state_t;

#define SET_FLAG(state, flag) (state.flags) |= (flag)
#define CLEAR_FLAG(state, flag) (state.flags) &= ~(flag)
#define CHECK_FLAG(state, flag) !!((state.flags) & (flag))

// Legacy disassembler state (kept for compatibility)
static p_state_t processor_state;

// Global pointer to actual emulated machine state
static processor_state_t *g_emulated_processor = NULL;

void init() {
    memset(&processor_state, 0, sizeof(p_state_t));
}

// Set the emulated processor state to use for flag checks
void set_emulated_processor(processor_state_t *proc) {
    g_emulated_processor = proc;
}

// Check M flag: if emulated processor is set, use it; otherwise use legacy state
bool isMSet() {
    if (g_emulated_processor) {
        // In emulation mode, X and M are always 1 (8-bit mode)
        if (g_emulated_processor->emulation_mode) return true;
        return !!(g_emulated_processor->P & M_FLAG);
    }
    // Legacy fallback
    if(CHECK_FLAG(processor_state, E_FLAG)) return true;
    return CHECK_FLAG(processor_state, M_FLAG);
}

// Check X flag: if emulated processor is set, use it; otherwise use legacy state
bool isXSet() {
    if (g_emulated_processor) {
        // In emulation mode, X and M are always 1 (8-bit mode)
        if (g_emulated_processor->emulation_mode) return true;
        return !!(g_emulated_processor->P & X_FLAG);
    }
    // Legacy fallback
    if(CHECK_FLAG(processor_state, E_FLAG)) return true;
    return CHECK_FLAG(processor_state, X_FLAG);
}

bool isESet() {
    return CHECK_FLAG(processor_state, E_FLAG);
}

bool carrySet() {
    return CHECK_FLAG(processor_state, CARRY);
}

void REP(unsigned char x) {
    switch(x) {
        case 0x10:
            SET_FLAG(processor_state, X_FLAG);
            break;
        case 0x20:
            SET_FLAG(processor_state, M_FLAG);
            break;
        default:
            ;
    }
}

void SEP(unsigned char x) {
    switch(x) {
        case 0x10:
            CLEAR_FLAG(processor_state, X_FLAG);
            break;
        case 0x20:
            CLEAR_FLAG(processor_state, M_FLAG);
            break;
        default:
            ;
    }
}

void SEC(unsigned char unused) {
    uint8_t cc = unused;
    SET_FLAG(processor_state, CARRY);
}

void CLC(unsigned char unused) {
    uint8_t cc = unused;
    CLEAR_FLAG(processor_state, CARRY);
}

void XCE(unsigned char unused) {
    uint8_t cc = unused;
    if(carrySet() && !isESet()) SET_FLAG(processor_state, E_FLAG);
    else if(!carrySet() && isESet()) CLEAR_FLAG(processor_state, E_FLAG);
}

void set_state(unsigned char x) {
    processor_state.flags = x;
}

uint8_t get_state() {
    return processor_state.flags;
}

unsigned int get_start_offset() {
    return processor_state.start;
}

void set_start_offset(unsigned int x) {
    processor_state.start = x;
}

#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG

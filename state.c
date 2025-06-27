#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "state.h"

typedef struct p_state_s {
    unsigned char flags;
} p_state_t;

#define SET_FLAG(state, flag) (state.flags) |= (flag)
#define CLEAR_FLAG(state, flag) (state.flags) &= ~(flag)
#define CHECK_FLAG(state, flag) !!((state.flags) & (flag))

static p_state_t processor_state;

void init() {
    memset(&processor_state, 0, sizeof(p_state_t));
}

bool isMSet() {
    if(CHECK_FLAG(processor_state, E_FLAG)) return false;
    return CHECK_FLAG(processor_state, M_FLAG);
}

bool isXSet() {
    if(CHECK_FLAG(processor_state, E_FLAG)) return false;
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
            CLEAR_FLAG(processor_state, X_FLAG);
            break;
        case 0x20:
            CLEAR_FLAG(processor_state, M_FLAG);
            break;
        default:
            ;
    }
}

void SEP(unsigned char x) {
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

#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG

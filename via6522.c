#include "via6522.h"
#include <string.h>

static void update_irq(via6522_t* via);
static void update_ca2_output(via6522_t* via);
static void update_cb2_output(via6522_t* via);

void via6522_init(via6522_t* via) {
    memset(via, 0, sizeof(via6522_t));
    via6522_reset(via);
}

void via6522_reset(via6522_t* via) {
    via->ora = 0;
    via->orb = 0;
    via->ira = 0;
    via->irb = 0;
    via->ddra = 0;
    via->ddrb = 0;
    
    via->t1_counter = 0xFFFF;
    via->t1_latch = 0xFFFF;
    via->t2_counter = 0xFFFF;
    via->t2_latch_low = 0xFF;
    via->t1_running = false;
    via->t2_running = false;
    via->t1_pb7_state = false;
    
    via->sr = 0;
    via->sr_count = 0;
    
    via->acr = 0;
    via->pcr = 0;
    via->ifr = 0;
    via->ier = 0;
    
    via->ira_latch = 0;
    via->irb_latch = 0;
    
    via->ca1 = false;
    via->ca2 = false;
    via->cb1 = false;
    via->cb2 = false;
}

uint8_t via6522_read(via6522_t* via, uint8_t reg) {
    uint8_t value = 0;
    
    reg &= 0x0F;  // Only 16 registers
    
    switch (reg) {
        case VIA_ORB_IRB: {
            // Read input register B
            uint8_t input = via->irb;
            if (via->port_b_read) {
                input = via->port_b_read(via->callback_context);
            }
            
            // Mix input and output based on DDR
            value = (via->orb & via->ddrb) | (input & ~via->ddrb);
            
            // Clear CB1 and CB2 interrupt flags
            via->ifr &= ~(VIA_INT_CB1 | VIA_INT_CB2);
            update_irq(via);
            break;
        }
        
        case VIA_ORA_IRA:
        case VIA_ORA_IRA_NH: {
            // Read input register A
            uint8_t input = via->ira;
            if (via->port_a_read) {
                input = via->port_a_read(via->callback_context);
            }
            
            // Use latched value if latching is enabled
            if ((via->acr & VIA_ACR_PA_LATCH) && reg == VIA_ORA_IRA) {
                input = via->ira_latch;
            }
            
            // Mix input and output based on DDR
            value = (via->ora & via->ddra) | (input & ~via->ddra);
            
            // Clear CA1 and CA2 interrupt flags (except for no-handshake register)
            if (reg == VIA_ORA_IRA) {
                via->ifr &= ~(VIA_INT_CA1 | VIA_INT_CA2);
                update_irq(via);
            }
            break;
        }
        
        case VIA_DDRB:
            value = via->ddrb;
            break;
            
        case VIA_DDRA:
            value = via->ddra;
            break;
            
        case VIA_T1CL:
            // Read T1 counter low, clear T1 interrupt
            value = via->t1_counter & 0xFF;
            via->ifr &= ~VIA_INT_T1;
            update_irq(via);
            break;
            
        case VIA_T1CH:
            // Read T1 counter high
            value = (via->t1_counter >> 8) & 0xFF;
            break;
            
        case VIA_T1LL:
            // Read T1 latch low
            value = via->t1_latch & 0xFF;
            break;
            
        case VIA_T1LH:
            // Read T1 latch high
            value = (via->t1_latch >> 8) & 0xFF;
            break;
            
        case VIA_T2CL:
            // Read T2 counter low, clear T2 interrupt
            value = via->t2_counter & 0xFF;
            via->ifr &= ~VIA_INT_T2;
            update_irq(via);
            break;
            
        case VIA_T2CH:
            // Read T2 counter high
            value = (via->t2_counter >> 8) & 0xFF;
            break;
            
        case VIA_SR:
            // Read shift register, clear SR interrupt
            value = via->sr;
            via->ifr &= ~VIA_INT_SR;
            update_irq(via);
            break;
            
        case VIA_ACR:
            value = via->acr;
            break;
            
        case VIA_PCR:
            value = via->pcr;
            break;
            
        case VIA_IFR:
            // Bit 7 is set if any enabled interrupt is active
            value = via->ifr;
            if (via->ifr & via->ier & 0x7F) {
                value |= VIA_INT_ANY;
            }
            break;
            
        case VIA_IER:
            // Bit 7 always reads as 1
            value = via->ier | 0x80;
            break;
    }
    
    return value;
}

void via6522_write(via6522_t* via, uint8_t reg, uint8_t value) {
    reg &= 0x0F;
    
    switch (reg) {
        case VIA_ORB_IRB:
            via->orb = value;
            if (via->port_b_write) {
                // Only output bits set to output in DDR
                via->port_b_write(via->callback_context, value & via->ddrb);
            }
            
            // Clear CB1 and CB2 interrupt flags
            via->ifr &= ~(VIA_INT_CB1 | VIA_INT_CB2);
            update_cb2_output(via);
            update_irq(via);
            break;
            
        case VIA_ORA_IRA:
        case VIA_ORA_IRA_NH:
            via->ora = value;
            if (via->port_a_write) {
                // Only output bits set to output in DDR
                via->port_a_write(via->callback_context, value & via->ddra);
            }
            
            // Clear CA1 and CA2 interrupt flags (except for no-handshake)
            if (reg == VIA_ORA_IRA) {
                via->ifr &= ~(VIA_INT_CA1 | VIA_INT_CA2);
                update_ca2_output(via);
                update_irq(via);
            }
            break;
            
        case VIA_DDRB:
            via->ddrb = value;
            if (via->port_b_write) {
                via->port_b_write(via->callback_context, via->orb & via->ddrb);
            }
            break;
            
        case VIA_DDRA:
            via->ddra = value;
            if (via->port_a_write) {
                via->port_a_write(via->callback_context, via->ora & via->ddra);
            }
            break;
            
        case VIA_T1CL:
        case VIA_T1LL:
            // Write T1 latch low
            via->t1_latch = (via->t1_latch & 0xFF00) | value;
            break;
            
        case VIA_T1CH:
            // Write T1 latch high and counter, start timer, clear interrupt
            via->t1_latch = (via->t1_latch & 0x00FF) | (value << 8);
            via->t1_counter = via->t1_latch;
            via->t1_running = true;
            via->ifr &= ~VIA_INT_T1;
            
            // Set PB7 high if in PB7 mode
            if (via->acr & 0x80) {
                via->t1_pb7_state = true;
            }
            update_irq(via);
            break;
            
        case VIA_T1LH:
            // Write T1 latch high, clear interrupt
            via->t1_latch = (via->t1_latch & 0x00FF) | (value << 8);
            via->ifr &= ~VIA_INT_T1;
            update_irq(via);
            break;
            
        case VIA_T2CL:
            // Write T2 latch low
            via->t2_latch_low = value;
            break;
            
        case VIA_T2CH:
            // Write T2 counter high, load counter, start timer, clear interrupt
            via->t2_counter = (value << 8) | via->t2_latch_low;
            via->t2_running = true;
            via->ifr &= ~VIA_INT_T2;
            update_irq(via);
            break;
            
        case VIA_SR:
            via->sr = value;
            via->ifr &= ~VIA_INT_SR;
            update_irq(via);
            break;
            
        case VIA_ACR:
            via->acr = value;
            break;
            
        case VIA_PCR:
            via->pcr = value;
            update_ca2_output(via);
            update_cb2_output(via);
            break;
            
        case VIA_IFR:
            // Writing 1s to IFR clears those interrupt flags
            via->ifr &= ~(value & 0x7F);
            update_irq(via);
            break;
            
        case VIA_IER:
            // Bit 7 set/clear, bits 6-0 select interrupts
            if (value & 0x80) {
                // Set enabled interrupts
                via->ier |= (value & 0x7F);
            } else {
                // Clear enabled interrupts
                via->ier &= ~(value & 0x7F);
            }
            update_irq(via);
            break;
    }
}

void via6522_clock(via6522_t* via) {
    // Clock Timer 1
    if (via->t1_running) {
        if (via->t1_counter == 0) {
            // Timer underflow
            via->ifr |= VIA_INT_T1;
            
            uint8_t t1_mode = via->acr & 0xC0;
            if (t1_mode == VIA_ACR_T1_CONTINUOUS || t1_mode == VIA_ACR_T1_CONTINUOUS_PB7) {
                // Reload from latch and continue
                via->t1_counter = via->t1_latch;
            } else {
                // One-shot mode
                via->t1_counter = 0xFFFF;
            }
            
            // Toggle PB7 if in PB7 mode
            if (via->acr & 0x80) {
                via->t1_pb7_state = !via->t1_pb7_state;
            }
            
            update_irq(via);
        } else {
            via->t1_counter--;
        }
    }
    
    // Clock Timer 2
    if (via->t2_running) {
        if (via->t2_counter == 0) {
            // Timer underflow
            via->ifr |= VIA_INT_T2;
            via->t2_running = false;
            via->t2_counter = 0xFFFF;
            update_irq(via);
        } else {
            via->t2_counter--;
        }
    }
    
    // TODO: Implement shift register clocking based on ACR settings
}

void via6522_set_ca1(via6522_t* via, bool state) {
    bool old_state = via->ca1;
    via->ca1 = state;
    
    // Check for edge
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (via->pcr & 0x01) {
        // Positive edge
        trigger = pos_edge;
    } else {
        // Negative edge
        trigger = neg_edge;
    }
    
    if (trigger) {
        via->ifr |= VIA_INT_CA1;
        
        // Latch port A if enabled
        if (via->acr & VIA_ACR_PA_LATCH) {
            uint8_t input = via->ira;
            if (via->port_a_read) {
                input = via->port_a_read(via->callback_context);
            }
            via->ira_latch = input;
        }
        
        update_irq(via);
    }
}

void via6522_set_ca2_input(via6522_t* via, bool state) {
    // Only process if CA2 is in input mode
    uint8_t ca2_mode = (via->pcr >> 1) & 0x07;
    if (ca2_mode >= 4) return;  // Output mode
    
    bool old_state = via->ca2;
    via->ca2 = state;
    
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (via->pcr & 0x04) {
        // Positive edge
        trigger = pos_edge;
    } else {
        // Negative edge  
        trigger = neg_edge;
    }
    
    if (trigger) {
        via->ifr |= VIA_INT_CA2;
        update_irq(via);
        
        // Clear interrupt on next active edge if independent mode
        if (!(via->pcr & 0x02)) {
            // Independent interrupt mode - flag clears on next edge
        }
    }
}

void via6522_set_cb1(via6522_t* via, bool state) {
    bool old_state = via->cb1;
    via->cb1 = state;
    
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (via->pcr & 0x10) {
        // Positive edge
        trigger = pos_edge;
    } else {
        // Negative edge
        trigger = neg_edge;
    }
    
    if (trigger) {
        via->ifr |= VIA_INT_CB1;
        
        // Latch port B if enabled
        if (via->acr & VIA_ACR_PB_LATCH) {
            uint8_t input = via->irb;
            if (via->port_b_read) {
                input = via->port_b_read(via->callback_context);
            }
            via->irb_latch = input;
        }
        
        update_irq(via);
    }
}

void via6522_set_cb2_input(via6522_t* via, bool state) {
    // Only process if CB2 is in input mode
    uint8_t cb2_mode = (via->pcr >> 5) & 0x07;
    if (cb2_mode >= 4) return;  // Output mode
    
    bool old_state = via->cb2;
    via->cb2 = state;
    
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (via->pcr & 0x40) {
        // Positive edge
        trigger = pos_edge;
    } else {
        // Negative edge
        trigger = neg_edge;
    }
    
    if (trigger) {
        via->ifr |= VIA_INT_CB2;
        update_irq(via);
    }
}

bool via6522_get_irq(via6522_t* via) {
    return (via->ifr & via->ier & 0x7F) != 0;
}

void via6522_set_port_a_callbacks(via6522_t* via,
                                   uint8_t (*read_fn)(void*),
                                   void (*write_fn)(void*, uint8_t),
                                   void* context) {
    via->port_a_read = read_fn;
    via->port_a_write = write_fn;
    via->callback_context = context;
}

void via6522_set_port_b_callbacks(via6522_t* via,
                                   uint8_t (*read_fn)(void*),
                                   void (*write_fn)(void*, uint8_t),
                                   void* context) {
    via->port_b_read = read_fn;
    via->port_b_write = write_fn;
    via->callback_context = context;
}

void via6522_set_irq_callback(via6522_t* via,
                               void (*irq_fn)(void*, bool),
                               void* context) {
    via->irq_callback = irq_fn;
    via->irq_context = context;
}

// Internal helper functions

static void update_irq(via6522_t* via) {
    bool irq_state = via6522_get_irq(via);
    
    if (via->irq_callback) {
        via->irq_callback(via->irq_context, irq_state);
    }
}

static void update_ca2_output(via6522_t* via) {
    uint8_t ca2_mode = (via->pcr >> 1) & 0x07;
    
    switch (ca2_mode) {
        case 0:
        case 1:
        case 2:
        case 3:
            // Input modes - do nothing
            break;
            
        case 4:
            // Handshake mode - goes low on read/write ORA, high on CA1 edge
            // Handled in read/write functions
            break;
            
        case 5:
            // Pulse mode - one cycle low pulse on read/write ORA
            via->ca2 = false;
            // Would need clock to restore high
            break;
            
        case 6:
            // Manual output low
            via->ca2 = false;
            break;
            
        case 7:
            // Manual output high
            via->ca2 = true;
            break;
    }
}

static void update_cb2_output(via6522_t* via) {
    uint8_t cb2_mode = (via->pcr >> 5) & 0x07;
    
    switch (cb2_mode) {
        case 0:
        case 1:
        case 2:
        case 3:
            // Input modes - do nothing
            break;
            
        case 4:
            // Handshake mode
            break;
            
        case 5:
            // Pulse mode
            via->cb2 = false;
            break;
            
        case 6:
            // Manual output low
            via->cb2 = false;
            break;
            
        case 7:
            // Manual output high
            via->cb2 = true;
            break;
    }
}

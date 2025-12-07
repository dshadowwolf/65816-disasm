#include "pia6521.h"
#include <string.h>

static void update_irqa(pia6521_t* pia);
static void update_irqb(pia6521_t* pia);
static void update_ca2_output(pia6521_t* pia);
static void update_cb2_output(pia6521_t* pia);

void pia6521_init(pia6521_t* pia) {
    memset(pia, 0, sizeof(pia6521_t));
    pia6521_reset(pia);
}

void pia6521_reset(pia6521_t* pia) {
    pia->porta_data = 0;
    pia->porta_ddr = 0;
    pia->porta_ctrl = 0;
    
    pia->portb_data = 0;
    pia->portb_ddr = 0;
    pia->portb_ctrl = 0;
    
    pia->ca1 = false;
    pia->ca2 = false;
    pia->cb1 = false;
    pia->cb2 = false;
    
    pia->irqa1_flag = false;
    pia->irqa2_flag = false;
    pia->irqb1_flag = false;
    pia->irqb2_flag = false;
}

uint8_t pia6521_read(pia6521_t* pia, uint8_t reg) {
    uint8_t value = 0;
    
    reg &= 0x03;  // Only 4 registers
    
    switch (reg) {
        case PIA_PORTA_DATA: {
            if (pia->porta_ctrl & PIA_CR_DDR_ACCESS) {
                // Access Port A data register
                uint8_t input = 0;
                if (pia->porta_read) {
                    input = pia->porta_read(pia->callback_context);
                }
                
                // Mix output and input based on DDR
                value = (pia->porta_data & pia->porta_ddr) | (input & ~pia->porta_ddr);
                
                // Clear CA1 and CA2 interrupt flags on data read
                pia->irqa1_flag = false;
                pia->irqa2_flag = false;
                update_irqa(pia);
                
                // Handle CA2 handshake mode
                uint8_t ca2_mode = (pia->porta_ctrl & PIA_CR_CA2_MODE_MASK);
                if (ca2_mode == PIA_CA2_OUTPUT_HS) {
                    // In handshake mode, CA2 goes low on read
                    pia->ca2 = false;
                }
            } else {
                // Access Port A DDR
                value = pia->porta_ddr;
            }
            break;
        }
        
        case PIA_PORTA_CTRL: {
            // Build control register value with interrupt flags
            value = pia->porta_ctrl & 0x3F;  // Lower 6 bits from control register
            
            if (pia->irqa1_flag) {
                value |= PIA_CR_IRQA1_FLAG;
            }
            if (pia->irqa2_flag) {
                value |= PIA_CR_IRQA2_FLAG;
            }
            break;
        }
        
        case PIA_PORTB_DATA: {
            if (pia->portb_ctrl & PIA_CR_DDR_ACCESS) {
                // Access Port B data register
                uint8_t input = 0;
                if (pia->portb_read) {
                    input = pia->portb_read(pia->callback_context);
                }
                
                // Mix output and input based on DDR
                value = (pia->portb_data & pia->portb_ddr) | (input & ~pia->portb_ddr);
                
                // Clear CB1 and CB2 interrupt flags on data read
                pia->irqb1_flag = false;
                pia->irqb2_flag = false;
                update_irqb(pia);
                
                // Handle CB2 handshake mode
                uint8_t cb2_mode = (pia->portb_ctrl & PIA_CR_CA2_MODE_MASK);
                if (cb2_mode == PIA_CA2_OUTPUT_HS) {
                    // In handshake mode, CB2 goes low on read
                    pia->cb2 = false;
                }
            } else {
                // Access Port B DDR
                value = pia->portb_ddr;
            }
            break;
        }
        
        case PIA_PORTB_CTRL: {
            // Build control register value with interrupt flags
            value = pia->portb_ctrl & 0x3F;
            
            if (pia->irqb1_flag) {
                value |= PIA_CR_IRQA1_FLAG;
            }
            if (pia->irqb2_flag) {
                value |= PIA_CR_IRQA2_FLAG;
            }
            break;
        }
    }
    
    return value;
}

void pia6521_write(pia6521_t* pia, uint8_t reg, uint8_t value) {
    reg &= 0x03;
    
    switch (reg) {
        case PIA_PORTA_DATA: {
            if (pia->porta_ctrl & PIA_CR_DDR_ACCESS) {
                // Write to Port A data register
                pia->porta_data = value;
                
                if (pia->porta_write) {
                    // Only output bits set to output in DDR
                    pia->porta_write(pia->callback_context, value & pia->porta_ddr);
                }
                
                // Clear CA1 and CA2 interrupt flags on data write
                pia->irqa1_flag = false;
                pia->irqa2_flag = false;
                update_irqa(pia);
                
                // Handle CA2 handshake/pulse modes
                update_ca2_output(pia);
            } else {
                // Write to Port A DDR
                pia->porta_ddr = value;
                
                // Update outputs based on new DDR
                if (pia->porta_write) {
                    pia->porta_write(pia->callback_context, pia->porta_data & pia->porta_ddr);
                }
            }
            break;
        }
        
        case PIA_PORTA_CTRL: {
            // Write to control register (only lower 6 bits writable)
            pia->porta_ctrl = value & 0x3F;
            
            // Handle CA2 output modes
            update_ca2_output(pia);
            update_irqa(pia);
            break;
        }
        
        case PIA_PORTB_DATA: {
            if (pia->portb_ctrl & PIA_CR_DDR_ACCESS) {
                // Write to Port B data register
                pia->portb_data = value;
                
                if (pia->portb_write) {
                    pia->portb_write(pia->callback_context, value & pia->portb_ddr);
                }
                
                // Clear CB1 and CB2 interrupt flags on data write
                pia->irqb1_flag = false;
                pia->irqb2_flag = false;
                update_irqb(pia);
                
                // Handle CB2 handshake/pulse modes
                update_cb2_output(pia);
            } else {
                // Write to Port B DDR
                pia->portb_ddr = value;
                
                if (pia->portb_write) {
                    pia->portb_write(pia->callback_context, pia->portb_data & pia->portb_ddr);
                }
            }
            break;
        }
        
        case PIA_PORTB_CTRL: {
            // Write to control register
            pia->portb_ctrl = value & 0x3F;
            
            update_cb2_output(pia);
            update_irqb(pia);
            break;
        }
    }
}

void pia6521_set_ca1(pia6521_t* pia, bool state) {
    bool old_state = pia->ca1;
    pia->ca1 = state;
    
    // Check for edge
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    // Determine if this edge should trigger interrupt
    bool trigger = false;
    if (pia->porta_ctrl & PIA_CR_CA1_LOW_TO_HIGH) {
        // Positive edge active
        trigger = pos_edge;
    } else {
        // Negative edge active
        trigger = neg_edge;
    }
    
    if (trigger) {
        pia->irqa1_flag = true;
        update_irqa(pia);
        
        // In handshake mode, CA2 goes high on CA1 active edge
        uint8_t ca2_mode = (pia->porta_ctrl & PIA_CR_CA2_MODE_MASK);
        if (ca2_mode == PIA_CA2_OUTPUT_HS) {
            pia->ca2 = true;
        }
    }
}

void pia6521_set_ca2_input(pia6521_t* pia, bool state) {
    uint8_t ca2_mode = (pia->porta_ctrl & PIA_CR_CA2_MODE_MASK);
    
    // Only process if CA2 is in input mode
    if (ca2_mode >= PIA_CA2_OUTPUT_HS) {
        return;  // Output mode
    }
    
    bool old_state = pia->ca2;
    pia->ca2 = state;
    
    // Check if interrupts are enabled for CA2 input
    if (!(ca2_mode & 0x08)) {
        return;  // IRQ not enabled for CA2
    }
    
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (ca2_mode & 0x10) {
        // Positive edge
        trigger = pos_edge;
    } else {
        // Negative edge
        trigger = neg_edge;
    }
    
    if (trigger) {
        pia->irqa2_flag = true;
        update_irqa(pia);
    }
}

void pia6521_set_cb1(pia6521_t* pia, bool state) {
    bool old_state = pia->cb1;
    pia->cb1 = state;
    
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (pia->portb_ctrl & PIA_CR_CA1_LOW_TO_HIGH) {
        trigger = pos_edge;
    } else {
        trigger = neg_edge;
    }
    
    if (trigger) {
        pia->irqb1_flag = true;
        update_irqb(pia);
        
        // In handshake mode, CB2 goes high on CB1 active edge
        uint8_t cb2_mode = (pia->portb_ctrl & PIA_CR_CA2_MODE_MASK);
        if (cb2_mode == PIA_CA2_OUTPUT_HS) {
            pia->cb2 = true;
        }
    }
}

void pia6521_set_cb2_input(pia6521_t* pia, bool state) {
    uint8_t cb2_mode = (pia->portb_ctrl & PIA_CR_CA2_MODE_MASK);
    
    // Only process if CB2 is in input mode
    if (cb2_mode >= PIA_CA2_OUTPUT_HS) {
        return;  // Output mode
    }
    
    bool old_state = pia->cb2;
    pia->cb2 = state;
    
    // Check if interrupts are enabled for CB2 input
    if (!(cb2_mode & 0x08)) {
        return;
    }
    
    bool pos_edge = !old_state && state;
    bool neg_edge = old_state && !state;
    
    bool trigger = false;
    if (cb2_mode & 0x10) {
        trigger = pos_edge;
    } else {
        trigger = neg_edge;
    }
    
    if (trigger) {
        pia->irqb2_flag = true;
        update_irqb(pia);
    }
}

bool pia6521_get_irqa(pia6521_t* pia) {
    // IRQA is active when either flag is set
    return pia->irqa1_flag || pia->irqa2_flag;
}

bool pia6521_get_irqb(pia6521_t* pia) {
    // IRQB is active when either flag is set
    return pia->irqb1_flag || pia->irqb2_flag;
}

void pia6521_set_porta_callbacks(pia6521_t* pia,
                                  uint8_t (*read_fn)(void*),
                                  void (*write_fn)(void*, uint8_t),
                                  void* context) {
    pia->porta_read = read_fn;
    pia->porta_write = write_fn;
    pia->callback_context = context;
}

void pia6521_set_portb_callbacks(pia6521_t* pia,
                                  uint8_t (*read_fn)(void*),
                                  void (*write_fn)(void*, uint8_t),
                                  void* context) {
    pia->portb_read = read_fn;
    pia->portb_write = write_fn;
    pia->callback_context = context;
}

void pia6521_set_irqa_callback(pia6521_t* pia,
                                void (*irq_fn)(void*, bool),
                                void* context) {
    pia->irqa_callback = irq_fn;
    pia->irq_context = context;
}

void pia6521_set_irqb_callback(pia6521_t* pia,
                                void (*irq_fn)(void*, bool),
                                void* context) {
    pia->irqb_callback = irq_fn;
    pia->irq_context = context;
}

// Internal helper functions

static void update_irqa(pia6521_t* pia) {
    bool irq_state = pia6521_get_irqa(pia);
    
    if (pia->irqa_callback) {
        pia->irqa_callback(pia->irq_context, irq_state);
    }
}

static void update_irqb(pia6521_t* pia) {
    bool irq_state = pia6521_get_irqb(pia);
    
    if (pia->irqb_callback) {
        pia->irqb_callback(pia->irq_context, irq_state);
    }
}

static void update_ca2_output(pia6521_t* pia) {
    uint8_t ca2_mode = (pia->porta_ctrl & PIA_CR_CA2_MODE_MASK);
    
    switch (ca2_mode) {
        case PIA_CA2_INPUT_NEG:
        case PIA_CA2_INPUT_NEG_IRQ:
        case PIA_CA2_INPUT_POS:
        case PIA_CA2_INPUT_POS_IRQ:
            // Input modes - do nothing
            break;
            
        case PIA_CA2_OUTPUT_HS:
            // Handshake mode - controlled by read/write and CA1
            // CA2 goes low on read/write of data, high on CA1 active edge
            break;
            
        case PIA_CA2_OUTPUT_PULSE:
            // Pulse mode - one cycle low on write to Port A
            // For simplicity, we'll just set it low (would need clock to pulse)
            pia->ca2 = false;
            break;
            
        case PIA_CA2_OUTPUT_LOW:
            // Manual output low
            pia->ca2 = false;
            break;
            
        case PIA_CA2_OUTPUT_HIGH:
            // Manual output high
            pia->ca2 = true;
            break;
    }
}

static void update_cb2_output(pia6521_t* pia) {
    uint8_t cb2_mode = (pia->portb_ctrl & PIA_CR_CA2_MODE_MASK);
    
    switch (cb2_mode) {
        case PIA_CA2_INPUT_NEG:
        case PIA_CA2_INPUT_NEG_IRQ:
        case PIA_CA2_INPUT_POS:
        case PIA_CA2_INPUT_POS_IRQ:
            // Input modes
            break;
            
        case PIA_CA2_OUTPUT_HS:
            // Handshake mode
            break;
            
        case PIA_CA2_OUTPUT_PULSE:
            // Pulse mode
            pia->cb2 = false;
            break;
            
        case PIA_CA2_OUTPUT_LOW:
            pia->cb2 = false;
            break;
            
        case PIA_CA2_OUTPUT_HIGH:
            pia->cb2 = true;
            break;
    }
}

#ifndef __PIA6521_H__
#define __PIA6521_H__

#include <stdint.h>
#include <stdbool.h>

// 6521 PIA Register offsets
#define PIA_PORTA_DATA  0x00  // Port A Data/DDR (based on CRA bit 2)
#define PIA_PORTA_CTRL  0x01  // Port A Control Register
#define PIA_PORTB_DATA  0x02  // Port B Data/DDR (based on CRB bit 2)
#define PIA_PORTB_CTRL  0x03  // Port B Control Register

// Control Register (CRA/CRB) bit definitions
#define PIA_CR_CA1_LOW_TO_HIGH   0x01  // CA1/CB1: 0=negative edge, 1=positive edge
#define PIA_CR_DDR_ACCESS        0x04  // 0=access DDR, 1=access data register
#define PIA_CR_CA2_MODE_MASK     0x38  // CA2/CB2 mode bits
#define PIA_CR_IRQA1_FLAG        0x40  // CA1/CB1 interrupt flag (read-only)
#define PIA_CR_IRQA2_FLAG        0x80  // CA2/CB2 interrupt flag (read-only)

// CA2/CB2 control modes (bits 3-5 of CR)
#define PIA_CA2_INPUT_NEG        0x00  // Input, negative edge
#define PIA_CA2_INPUT_NEG_IRQ    0x08  // Input, negative edge with IRQ
#define PIA_CA2_INPUT_POS        0x10  // Input, positive edge
#define PIA_CA2_INPUT_POS_IRQ    0x18  // Input, positive edge with IRQ
#define PIA_CA2_OUTPUT_HS        0x20  // Output handshake
#define PIA_CA2_OUTPUT_PULSE     0x28  // Output pulse
#define PIA_CA2_OUTPUT_LOW       0x30  // Output manual low
#define PIA_CA2_OUTPUT_HIGH      0x38  // Output manual high

// IRQ enable bits (in CRA/CRB)
#define PIA_CR_IRQA1_ENABLE      0x01  // Enable IRQ on CA1/CB1 (combined with edge select)
#define PIA_CR_IRQA2_ENABLE      0x08  // Enable IRQ on CA2/CB2 (when in input mode)

typedef struct pia6521_s {
    // Port A registers
    uint8_t porta_data;    // Port A output register
    uint8_t porta_ddr;     // Port A data direction (1=output, 0=input)
    uint8_t porta_ctrl;    // Port A control register (CRA)
    
    // Port B registers
    uint8_t portb_data;    // Port B output register
    uint8_t portb_ddr;     // Port B data direction
    uint8_t portb_ctrl;    // Port B control register (CRB)
    
    // Control line states
    bool ca1;              // CA1 input state
    bool ca2;              // CA2 input/output state
    bool cb1;              // CB1 input state
    bool cb2;              // CB2 input/output state
    
    // Interrupt flags (stored in control registers)
    bool irqa1_flag;       // CA1 interrupt flag
    bool irqa2_flag;       // CA2 interrupt flag
    bool irqb1_flag;       // CB1 interrupt flag
    bool irqb2_flag;       // CB2 interrupt flag
    
    // External port read/write callbacks (optional)
    uint8_t (*porta_read)(void* context);
    void (*porta_write)(void* context, uint8_t value);
    uint8_t (*portb_read)(void* context);
    void (*portb_write)(void* context, uint8_t value);
    void* callback_context;
    
    // IRQ output callbacks (separate for IRQA and IRQB)
    void (*irqa_callback)(void* context, bool state);
    void (*irqb_callback)(void* context, bool state);
    void* irq_context;
} pia6521_t;

// Initialize a PIA chip
void pia6521_init(pia6521_t* pia);

// Reset the PIA to power-on state
void pia6521_reset(pia6521_t* pia);

// Register read/write
uint8_t pia6521_read(pia6521_t* pia, uint8_t reg);
void pia6521_write(pia6521_t* pia, uint8_t reg, uint8_t value);

// Control line inputs (for external hardware simulation)
void pia6521_set_ca1(pia6521_t* pia, bool state);
void pia6521_set_ca2_input(pia6521_t* pia, bool state);
void pia6521_set_cb1(pia6521_t* pia, bool state);
void pia6521_set_cb2_input(pia6521_t* pia, bool state);

// Get current IRQ states
bool pia6521_get_irqa(pia6521_t* pia);
bool pia6521_get_irqb(pia6521_t* pia);

// Set callback functions for port I/O
void pia6521_set_porta_callbacks(pia6521_t* pia,
                                  uint8_t (*read_fn)(void*),
                                  void (*write_fn)(void*, uint8_t),
                                  void* context);
void pia6521_set_portb_callbacks(pia6521_t* pia,
                                  uint8_t (*read_fn)(void*),
                                  void (*write_fn)(void*, uint8_t),
                                  void* context);

// Set IRQ callbacks
void pia6521_set_irqa_callback(pia6521_t* pia,
                                void (*irq_fn)(void*, bool),
                                void* context);
void pia6521_set_irqb_callback(pia6521_t* pia,
                                void (*irq_fn)(void*, bool),
                                void* context);

#endif // __PIA6521_H__

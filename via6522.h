#ifndef __VIA6522_H__
#define __VIA6522_H__

#include <stdint.h>
#include <stdbool.h>

// 6522 VIA Register offsets
#define VIA_ORB_IRB    0x00  // Output/Input Register B
#define VIA_ORA_IRA    0x01  // Output/Input Register A (with handshake)
#define VIA_DDRB       0x02  // Data Direction Register B
#define VIA_DDRA       0x03  // Data Direction Register A
#define VIA_T1CL       0x04  // Timer 1 Counter Low
#define VIA_T1CH       0x05  // Timer 1 Counter High
#define VIA_T1LL       0x06  // Timer 1 Latch Low
#define VIA_T1LH       0x07  // Timer 1 Latch High
#define VIA_T2CL       0x08  // Timer 2 Counter Low
#define VIA_T2CH       0x09  // Timer 2 Counter High
#define VIA_SR         0x0A  // Shift Register
#define VIA_ACR        0x0B  // Auxiliary Control Register
#define VIA_PCR        0x0C  // Peripheral Control Register
#define VIA_IFR        0x0D  // Interrupt Flag Register
#define VIA_IER        0x0E  // Interrupt Enable Register
#define VIA_ORA_IRA_NH 0x0F  // Output/Input Register A (no handshake)

// Interrupt flag bits
#define VIA_INT_CA2    0x01
#define VIA_INT_CA1    0x02
#define VIA_INT_SR     0x04
#define VIA_INT_CB2    0x08
#define VIA_INT_CB1    0x10
#define VIA_INT_T2     0x20
#define VIA_INT_T1     0x40
#define VIA_INT_ANY    0x80  // Set when any enabled interrupt is active

// Auxiliary Control Register bits
#define VIA_ACR_PA_LATCH   0x01  // Port A latching enable
#define VIA_ACR_PB_LATCH   0x02  // Port B latching enable
#define VIA_ACR_SR_MASK    0x1C  // Shift register control mask
#define VIA_ACR_T2_CTRL    0x20  // Timer 2 control
#define VIA_ACR_T1_CTRL    0xC0  // Timer 1 control mask

// Timer 1 control modes
#define VIA_ACR_T1_TIMED_INT      0x00  // Timed interrupt each time T1 loaded
#define VIA_ACR_T1_CONTINUOUS     0x40  // Continuous interrupts
#define VIA_ACR_T1_TIMED_PB7      0x80  // One-shot on PB7
#define VIA_ACR_T1_CONTINUOUS_PB7 0xC0  // Continuous on PB7

// Peripheral Control Register CA1/CB1 control
#define VIA_PCR_CA1_NEG_EDGE 0x00
#define VIA_PCR_CA1_POS_EDGE 0x01
#define VIA_PCR_CB1_NEG_EDGE 0x00
#define VIA_PCR_CB1_POS_EDGE 0x10

// CA2/CB2 control modes
#define VIA_PCR_CA2_INPUT_NEG     0x00
#define VIA_PCR_CA2_INPUT_NEG_IND 0x02
#define VIA_PCR_CA2_INPUT_POS     0x04
#define VIA_PCR_CA2_INPUT_POS_IND 0x06
#define VIA_PCR_CA2_OUTPUT_HS     0x08
#define VIA_PCR_CA2_OUTPUT_PULSE  0x0A
#define VIA_PCR_CA2_OUTPUT_LOW    0x0C
#define VIA_PCR_CA2_OUTPUT_HIGH   0x0E

typedef struct via6522_s {
    // I/O Registers
    uint8_t ora;          // Output Register A
    uint8_t orb;          // Output Register B
    uint8_t ira;          // Input Register A
    uint8_t irb;          // Input Register B
    uint8_t ddra;         // Data Direction Register A (1=output, 0=input)
    uint8_t ddrb;         // Data Direction Register B
    
    // Timer registers
    uint16_t t1_counter;  // Timer 1 counter
    uint16_t t1_latch;    // Timer 1 latch
    uint16_t t2_counter;  // Timer 2 counter
    uint8_t t2_latch_low; // Timer 2 low-order latch
    bool t1_running;      // Timer 1 active flag
    bool t2_running;      // Timer 2 active flag
    bool t1_pb7_state;    // Timer 1 PB7 output state
    
    // Shift register
    uint8_t sr;           // Shift register
    uint8_t sr_count;     // Shift counter
    
    // Control registers
    uint8_t acr;          // Auxiliary Control Register
    uint8_t pcr;          // Peripheral Control Register
    uint8_t ifr;          // Interrupt Flag Register
    uint8_t ier;          // Interrupt Enable Register
    
    // Latched input values
    uint8_t ira_latch;    // Latched input on CA1 edge
    uint8_t irb_latch;    // Latched input on CB1 edge
    
    // Control line states
    bool ca1;             // CA1 input state
    bool ca2;             // CA2 input/output state
    bool cb1;             // CB1 input state
    bool cb2;             // CB2 input/output state
    
    // External port read/write callbacks (optional)
    uint8_t (*port_a_read)(void* context);
    void (*port_a_write)(void* context, uint8_t value);
    uint8_t (*port_b_read)(void* context);
    void (*port_b_write)(void* context, uint8_t value);
    void* callback_context;
    
    // IRQ output callback
    void (*irq_callback)(void* context, bool state);
    void* irq_context;
} via6522_t;

// Initialize a VIA chip
void via6522_init(via6522_t* via);

// Reset the VIA to power-on state
void via6522_reset(via6522_t* via);

// Register read/write
uint8_t via6522_read(via6522_t* via, uint8_t reg);
void via6522_write(via6522_t* via, uint8_t reg, uint8_t value);

// Clock the VIA (call this each CPU cycle or at appropriate intervals)
void via6522_clock(via6522_t* via);

// Control line inputs (for external hardware simulation)
void via6522_set_ca1(via6522_t* via, bool state);
void via6522_set_ca2_input(via6522_t* via, bool state);
void via6522_set_cb1(via6522_t* via, bool state);
void via6522_set_cb2_input(via6522_t* via, bool state);

// Get current IRQ state
bool via6522_get_irq(via6522_t* via);

// Set callback functions for port I/O
void via6522_set_port_a_callbacks(via6522_t* via, 
                                   uint8_t (*read_fn)(void*),
                                   void (*write_fn)(void*, uint8_t),
                                   void* context);
void via6522_set_port_b_callbacks(via6522_t* via,
                                   uint8_t (*read_fn)(void*),
                                   void (*write_fn)(void*, uint8_t),
                                   void* context);

// Set IRQ callback
void via6522_set_irq_callback(via6522_t* via,
                               void (*irq_fn)(void*, bool),
                               void* context);

#endif // __VIA6522_H__

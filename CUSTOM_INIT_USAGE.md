# Custom Initial State API

This document describes how to use the custom initial state API to create machines with user-defined processor register values.

## Overview

The custom initialization API allows you to create a 65816 machine with specific initial register values, rather than the default reset state. This is useful for:

- Testing specific scenarios
- Resuming from a saved state
- Starting execution from a specific address with predetermined register values
- Setting up specific processor flags or modes

## API Functions

Three levels of initialization are provided:

### 1. Processor Level
```c
void initialize_processor_with_state(processor_state_t *state, const initial_state_t *init);
```
Initializes just the processor registers. Pass `NULL` for `init` to use default values.

### 2. Machine Level
```c
void initialize_machine_with_state(machine_state_t *machine, const initial_state_t *init);
```
Initializes both the processor and the entire machine (memory regions, devices). Pass `NULL` for `init` to use default processor state.

### 3. Creation Level (Recommended)
```c
machine_state_t* create_machine_with_state(const initial_state_t *init);
```
Allocates and initializes a complete machine. Pass `NULL` for `init` to use default state. This is the most convenient function to use.

## The initial_state_t Structure

```c
typedef struct initial_state_s {
    uint16_t A;                // Accumulator (full 16-bit value)
    uint16_t X;                // Index Register X
    uint16_t Y;                // Index Register Y
    uint16_t PC;               // Program Counter
    uint16_t SP;               // Stack Pointer
    uint16_t DP;               // Direct Page Register
    uint8_t P;                 // Processor Status Register
    uint8_t PBR;               // Program Bank Register
    uint8_t DBR;               // Data Bank Register
    bool emulation_mode;       // Emulation Mode Flag
    bool interrupts_disabled;  // Interrupt Disable Flag
} initial_state_t;
```

## Example Usage

### Basic Example
```c
#include "machine_setup.h"

int main() {
    // Define initial state
    initial_state_t init = {
        .A = 0x1234,                // Accumulator = $1234
        .X = 0x5678,                // X register = $5678
        .Y = 0x9ABC,                // Y register = $9ABC
        .PC = 0x8000,               // Start at $8000
        .SP = 0x01FF,               // Stack pointer
        .DP = 0x0000,               // Direct page
        .P = 0x30,                  // Status (M=1, X=1)
        .PBR = 0x01,                // Program bank = $01
        .DBR = 0x02,                // Data bank = $02
        .emulation_mode = false,    // Native mode
        .interrupts_disabled = true // I flag set
    };
    
    // Create machine with custom state
    machine_state_t *machine = create_machine_with_state(&init);
    
    // Machine is now ready to execute from $018000
    // with all specified register values
    
    destroy_machine(machine);
    return 0;
}
```

### Using Default State
```c
// Pass NULL to use default reset state
machine_state_t *machine = create_machine_with_state(NULL);
// Equivalent to: create_machine()
```

### Starting at Specific Address
```c
initial_state_t init = {
    .PC = 0x8000,      // Start at $8000
    .PBR = 0x01,       // In bank $01
    // All other fields default to 0/false
};
machine_state_t *machine = create_machine_with_state(&init);
// Execution starts at $018000
```

### Setting Up for Testing
```c
// Test scenario: 16-bit accumulator with specific value
initial_state_t init = {
    .A = 0xABCD,
    .P = 0x20,         // M=0 (16-bit A), X=1 (8-bit X/Y)
    .PC = 0x8000,
    .SP = 0x01FF,
    .emulation_mode = false
};
machine_state_t *machine = create_machine_with_state(&init);
```

## Notes

1. **24-bit Addressing**: The full execution address is formed as `(PBR << 16) | PC`. For example, PBR=$01 and PC=$8000 gives address $018000.

2. **Default Values**: When `NULL` is passed, the processor is initialized with:
   - All registers = 0
   - SP = $01FF
   - Emulation mode = false
   - Interrupts disabled = false

3. **Memory Setup**: The memory regions and devices are initialized the same way regardless of the initial processor state. Only the processor registers are customized.

4. **Status Register**: The P register bits follow the 65816 standard:
   - Bit 7: N (Negative)
   - Bit 6: V (Overflow)
   - Bit 5: M (Accumulator size: 1=8-bit, 0=16-bit)
   - Bit 4: X (Index size: 1=8-bit, 0=16-bit)
   - Bit 3: D (Decimal mode)
   - Bit 2: I (IRQ disable)
   - Bit 1: Z (Zero)
   - Bit 0: C (Carry)

5. **Emulation Mode**: When `emulation_mode = true`, the processor behaves like a 6502. When false, it operates in native 65816 mode.

## See Also

- `example_custom_init.c` - Working example demonstrating the API
- `machine_setup.h` - Full API declarations
- `EMULATED_STATE_USAGE.md` - Related documentation on processor state

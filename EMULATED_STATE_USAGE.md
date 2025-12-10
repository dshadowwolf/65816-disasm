# Using Emulated Machine State with Disassembler

## Overview

The `tbl.c` file contains the opcode table used by the disassembler. The `m_set()` and `x_set()` functions check the M and X flags to determine operand sizes. Previously, these used a separate internal state, but now they can reference the actual emulated processor state.

## How It Works

The `state.c` module now supports two modes:

1. **Legacy mode**: Uses internal disassembler state (for backward compatibility)
2. **Emulated mode**: Uses actual emulated processor state from `machine_state_t`

## Usage

### Setting Up Emulated State

Before using the disassembler with an emulated machine, call `set_emulated_processor()`:

```c
#include "machine_setup.h"
#include "state.h"

int main(void) {
    // Initialize machine
    machine_state_t machine;
    initialize_machine(&machine);
    
    // Connect disassembler to emulated processor state
    set_emulated_processor(&machine.processor);
    
    // Now when tbl.c checks isMSet() or isXSet(),
    // it will read the actual emulated processor flags
    
    // ... run your code ...
    
    cleanup_machine_with_via(&machine);
    return 0;
}
```

### Flag Checking Behavior

When `set_emulated_processor()` has been called with a valid processor pointer:

- `isMSet()` returns `true` if M flag (bit 5) is set in `processor.P` and not in emulation mode
- `isXSet()` returns `true` if X flag (bit 4) is set in `processor.P` and not in emulation mode
- Both return `false` in emulation mode (16-bit accumulator, 8-bit index registers)

### Example: Instruction Size Determination

```c
// In tbl.c, these functions determine operand sizes:

int m_set(int sz) {
    return isMSet() ? sz+1 : sz;  // Now checks machine.processor.P
}

int x_set(int sz) {
    return isXSet() ? sz+1 : sz;  // Now checks machine.processor.P
}
```

### Impact on Disassembly

The disassembler will now correctly show instruction operand sizes based on the actual processor state:

- **When M flag is clear (M=0)**: 16-bit accumulator operations show 16-bit immediates
- **When M flag is set (M=1)**: 8-bit accumulator operations show 8-bit immediates
- **When X flag is clear (X=0)**: 16-bit index register operations
- **When X flag is set (X=1)**: 8-bit index register operations

## Example Scenarios

### Scenario 1: After REP #$30 (Clear M and X flags)

```c
machine.processor.P &= ~(M_FLAG | X_FLAG);  // M=0, X=0
set_emulated_processor(&machine.processor);

// LDA #$1234 will disassemble with 16-bit immediate
// LDX #$5678 will disassemble with 16-bit immediate
```

### Scenario 2: After SEP #$30 (Set M and X flags)

```c
machine.processor.P |= (M_FLAG | X_FLAG);   // M=1, X=1
set_emulated_processor(&machine.processor);

// LDA #$12 will disassemble with 8-bit immediate
// LDX #$56 will disassemble with 8-bit immediate
```

### Scenario 3: Mixed Mode (REP #$20, SEP #$10)

```c
machine.processor.P &= ~M_FLAG;  // M=0 (16-bit accumulator)
machine.processor.P |= X_FLAG;   // X=1 (8-bit index)
set_emulated_processor(&machine.processor);

// LDA #$1234 - 16-bit immediate
// LDX #$56   - 8-bit immediate
```

## Legacy Mode

If `set_emulated_processor()` is not called (or called with NULL), the system falls back to the legacy internal state:

```c
set_emulated_processor(NULL);  // Use legacy mode

// Now isMSet() and isXSet() use internal disassembler state
// Modified by REP(), SEP(), etc. in state.c
```

## Integration with Processor Instructions

The processor instruction handlers (REP_CB, SEP_CB, etc.) modify `machine.processor.P` directly. When the emulated processor is connected, these changes are automatically reflected in disassembly:

```c
// Execute SEP #$30
SEP_CB(&machine, 0x30, 0);  // Sets M and X flags in machine.processor.P

// Subsequent disassembly will use 8-bit operand sizes
// because isMSet() and isXSet() now return true
```

## Benefits

1. **Accurate Disassembly**: Disassembler output matches actual processor state
2. **Live Debugging**: See instructions with correct sizes as processor executes
3. **State Synchronization**: No separate state to keep in sync
4. **Backward Compatible**: Legacy code still works without changes

## Notes

- The emulated processor pointer is stored globally in `state.c`
- Thread safety: Not thread-safe; use one disassembler context per thread
- Call `set_emulated_processor()` once during initialization
- Can switch back to legacy mode by calling with NULL

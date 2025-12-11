/*
 * Test for MVN (Move Block Negative) instruction
 * 
 * MVN moves a block of memory from source to destination, incrementing addresses.
 * Despite its name "Negative", it actually increments (moves forward).
 * 
 * Format: MVN srcbank, dstbank
 * - A contains count-1 (e.g., A=$0005 moves 6 bytes)
 * - X contains source address
 * - Y contains destination address
 * - DBR is set to srcbank after operation
 * - After completion: A=$FFFF, X=source+count, Y=dest+count
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "machine_setup.h"
#include "processor_helpers.h"
#include "processor.h"

void test_mvn_basic() {
    printf("Test: MVN basic block move (8 bytes)...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    // Set up emulation mode (like 6502)
    state->emulation_mode = 1;
    state->P |= 0x30; // Set M and X flags for 8-bit mode
    
    // Initialize source data at $1000-$1007
    uint16_t source_addr = 0x1000;
    uint8_t source_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    for (int i = 0; i < 8; i++) {
        write_byte_new(machine, source_addr + i, source_data[i]);
    }
    
    // Set up MVN parameters
    uint16_t dest_addr = 0x2000;
    state->A.full = 7;  // Move 8 bytes (count - 1)
    state->X = source_addr;
    state->Y = dest_addr;
    
    // Execute MVN srcbank=0, dstbank=0
    MVN(machine, 0x00, 0x00);
    
    // Verify destination has the copied data
    printf("  Verifying destination data...\n");
    for (int i = 0; i < 8; i++) {
        uint8_t byte = read_byte_new(machine, dest_addr + i);
        assert(byte == source_data[i]);
        printf("    Dest[$%04X] = $%02X (expected $%02X) ✓\n", 
               dest_addr + i, byte, source_data[i]);
    }
    
    // Verify registers after MVN
    printf("  Verifying final register state...\n");
    assert(state->A.full == 0xFFFF);
    printf("    A = $%04X (expected $FFFF) ✓\n", state->A.full);
    
    assert(state->X == source_addr + 8);
    printf("    X = $%04X (expected $%04X) ✓\n", state->X, source_addr + 8);
    
    assert(state->Y == dest_addr + 8);
    printf("    Y = $%04X (expected $%04X) ✓\n", state->Y, dest_addr + 8);
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvn_single_byte() {
    printf("Test: MVN single byte move...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Write single byte
    write_byte_new(machine, 0x1000, 0xAB);
    
    // Move 1 byte (A=0)
    state->A.full = 0;
    state->X = 0x1000;
    state->Y = 0x2000;
    
    MVN(machine, 0x00, 0x00);
    
    uint8_t byte = read_byte_new(machine, 0x2000);
    assert(byte == 0xAB);
    printf("  Dest[$2000] = $%02X (expected $AB) ✓\n", byte);
    
    assert(state->X == 0x1001);
    assert(state->Y == 0x2001);
    assert(state->A.full == 0xFFFF);
    printf("  X=$%04X, Y=$%04X, A=$%04X ✓\n", state->X, state->Y, state->A.full);
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvn_wraparound() {
    printf("Test: MVN with address wraparound...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Set up source data near end of 64K address space
    uint16_t source_addr = 0xFFFD;
    uint8_t source_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    for (int i = 0; i < 4; i++) {
        write_byte_new(machine, (source_addr + i) & 0xFFFF, source_data[i]);
    }
    
    // Move 4 bytes starting at $FFFD (wraps to $0000)
    state->A.full = 3;  // 4 bytes
    state->X = source_addr;
    state->Y = 0x3000;
    
    MVN(machine, 0x00, 0x00);
    
    // Verify all 4 bytes were copied
    printf("  Verifying wraparound copy...\n");
    for (int i = 0; i < 4; i++) {
        uint8_t byte = read_byte_new(machine, 0x3000 + i);
        assert(byte == source_data[i]);
        printf("    Dest[$%04X] = $%02X (expected $%02X) ✓\n", 
               0x3000 + i, byte, source_data[i]);
    }
    
    // X should wrap around
    assert(state->X == ((source_addr + 4) & 0xFFFF));
    printf("  X wrapped to $%04X ✓\n", state->X);
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvn_overlapping() {
    printf("Test: MVN with overlapping source/destination...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Set up data
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    for (int i = 0; i < 5; i++) {
        write_byte_new(machine, 0x1000 + i, data[i]);
    }
    
    // Copy forward with overlap: $1000-$1004 to $1002-$1006
    // This is safe because MVN increments (moves forward)
    state->A.full = 4;  // 5 bytes
    state->X = 0x1000;
    state->Y = 0x1002;
    
    MVN(machine, 0x00, 0x00);
    
    // Verify the copy
    printf("  Verifying overlapping forward copy...\n");
    // When copying $1000-$1004 to $1002-$1006, the data replicates:
    // Initial: 01 02 03 04 05
    // After:   01 02 01 02 01 02 01
    uint8_t expected[] = {0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01};
    for (int i = 0; i < 7; i++) {
        uint8_t byte = read_byte_new(machine, 0x1000 + i);
        assert(byte == expected[i]);
        printf("    Mem[$%04X] = $%02X (expected $%02X) ✓\n", 
               0x1000 + i, byte, expected[i]);
    }
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvn_pattern() {
    printf("Test: MVN with repeating pattern...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Create a pattern
    uint8_t pattern[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    for (int i = 0; i < 8; i++) {
        write_byte_new(machine, 0x5000 + i, pattern[i]);
    }
    
    // Move pattern to new location
    state->A.full = 7;
    state->X = 0x5000;
    state->Y = 0x6000;
    
    MVN(machine, 0x00, 0x00);
    
    // Verify pattern preserved
    printf("  Verifying pattern integrity...\n");
    for (int i = 0; i < 8; i++) {
        uint8_t byte = read_byte_new(machine, 0x6000 + i);
        assert(byte == pattern[i]);
        printf("    Pattern[%d] = $%02X ✓\n", i, byte);
    }
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvp_basic() {
    printf("Test: MVP basic block move (8 bytes, decrementing)...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    // Set up emulation mode
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Initialize source data at $1000-$1007
    uint16_t source_start = 0x1007;  // Start at end for decrement
    uint8_t source_data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    for (int i = 0; i < 8; i++) {
        write_byte_new(machine, 0x1000 + i, source_data[i]);
    }
    
    // Set up MVP parameters - moves from high to low addresses
    uint16_t dest_start = 0x2007;
    state->A.full = 7;  // Move 8 bytes (count - 1)
    state->X = source_start;  // Start at $1007
    state->Y = dest_start;    // Start at $2007
    
    // Execute MVP srcbank=0, dstbank=0
    MVP(machine, 0x00, 0x00);
    
    // Verify destination has the copied data (reversed order because we moved backwards)
    printf("  Verifying destination data...\n");
    for (int i = 0; i < 8; i++) {
        uint8_t byte = read_byte_new(machine, 0x2000 + i);
        // Data should be copied in same order: $2000 gets what was at $1000, etc.
        assert(byte == source_data[i]);
        printf("    Dest[$%04X] = $%02X (expected $%02X) ✓\n", 
               0x2000 + i, byte, source_data[i]);
    }
    
    // Verify registers after MVP
    printf("  Verifying final register state...\n");
    assert(state->A.full == 0xFFFF);
    printf("    A = $%04X (expected $FFFF) ✓\n", state->A.full);
    
    assert(state->X == (uint16_t)(source_start - 8));
    printf("    X = $%04X (expected $%04X) ✓\n", state->X, (uint16_t)(source_start - 8));
    
    assert(state->Y == (uint16_t)(dest_start - 8));
    printf("    Y = $%04X (expected $%04X) ✓\n", state->Y, (uint16_t)(dest_start - 8));
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvp_single_byte() {
    printf("Test: MVP single byte move...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Write single byte
    write_byte_new(machine, 0x1000, 0xCD);
    
    // Move 1 byte (A=0)
    state->A.full = 0;
    state->X = 0x1000;
    state->Y = 0x2000;
    
    MVP(machine, 0x00, 0x00);
    
    uint8_t byte = read_byte_new(machine, 0x2000);
    assert(byte == 0xCD);
    printf("  Dest[$2000] = $%02X (expected $CD) ✓\n", byte);
    
    assert(state->X == 0x0FFF);
    assert(state->Y == 0x1FFF);
    assert(state->A.full == 0xFFFF);
    printf("  X=$%04X, Y=$%04X, A=$%04X ✓\n", state->X, state->Y, state->A.full);
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvp_overlapping_backward() {
    printf("Test: MVP with overlapping source/destination (safe backward copy)...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Set up data at $1002-$1006
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    for (int i = 0; i < 5; i++) {
        write_byte_new(machine, 0x1002 + i, data[i]);
    }
    
    // Copy backward with overlap: $1002-$1006 to $1000-$1004
    // Start from end: X=$1006, Y=$1004
    // This is safe because MVP decrements (moves backward)
    state->A.full = 4;  // 5 bytes
    state->X = 0x1006;  // Source end
    state->Y = 0x1004;  // Dest end
    
    MVP(machine, 0x00, 0x00);
    
    // After MVP: memory at $1000-$1004 should be copy of $1002-$1006
    // But due to overlap, the data replicates backward (alternating pattern)
    printf("  Verifying overlapping backward copy...\n");
    // $1006(05)→$1004: $1004 = 05
    // $1005(04)→$1003: $1003 = 04
    // $1004(now 05!)→$1002: $1002 = 05
    // $1003(now 04!)→$1001: $1001 = 04
    // $1002(now 05!)→$1000: $1000 = 05
    // Result: 05 04 05 04 05
    uint8_t expected[] = {0x05, 0x04, 0x05, 0x04, 0x05};
    for (int i = 0; i < 5; i++) {
        uint8_t byte = read_byte_new(machine, 0x1000 + i);
        assert(byte == expected[i]);
        printf("    Mem[$%04X] = $%02X (expected $%02X) ✓\n", 
               0x1000 + i, byte, expected[i]);
    }
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

void test_mvp_wraparound() {
    printf("Test: MVP with address wraparound...\n");
    
    machine_state_t *machine = create_machine();
    processor_state_t *state = &machine->processor;
    
    state->emulation_mode = 1;
    state->P |= 0x30;
    
    // Set up source data at $0000-$0003 (will wrap from $0003 down)
    uint8_t source_data[] = {0xEE, 0xFF, 0x00, 0x11};
    for (int i = 0; i < 4; i++) {
        write_byte_new(machine, i, source_data[i]);
    }
    
    // Move 4 bytes starting at end ($0003) going down
    state->A.full = 3;  // 4 bytes
    state->X = 0x0003;  // Start at end
    state->Y = 0x3003;  // Destination end
    
    MVP(machine, 0x00, 0x00);
    
    // Verify all 4 bytes were copied
    printf("  Verifying wraparound copy...\n");
    for (int i = 0; i < 4; i++) {
        uint8_t byte = read_byte_new(machine, 0x3000 + i);
        assert(byte == source_data[i]);
        printf("    Dest[$%04X] = $%02X (expected $%02X) ✓\n", 
               0x3000 + i, byte, source_data[i]);
    }
    
    // X should wrap around (0003 - 4 = FFFF)
    assert(state->X == 0xFFFF);
    printf("  X wrapped to $%04X ✓\n", state->X);
    
    destroy_machine(machine);
    printf("  ✓ Test passed\n\n");
}

int main() {
    printf("=== MVN Instruction Tests ===\n\n");
    
    test_mvn_basic();
    test_mvn_single_byte();
    test_mvn_wraparound();
    test_mvn_overlapping();
    test_mvn_pattern();
    
    printf("\n=== MVP Instruction Tests ===\n\n");
    
    test_mvp_basic();
    test_mvp_single_byte();
    test_mvp_overlapping_backward();
    test_mvp_wraparound();
    
    printf("=== All MVN/MVP tests passed! ===\n");
    return 0;
}

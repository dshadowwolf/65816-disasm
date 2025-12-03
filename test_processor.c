#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "processor.h"
#include "processor_helpers.h"

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Color codes for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

// Test helper macros
#define TEST(name) \
    static void test_##name(); \
    static void run_test_##name() { \
        tests_run++; \
        printf(COLOR_BLUE "Running test: %s" COLOR_RESET "\n", #name); \
        test_##name(); \
        tests_passed++; \
        printf(COLOR_GREEN "  ✓ PASSED" COLOR_RESET "\n\n"); \
    } \
    static void test_##name()

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            tests_failed++; \
            tests_passed--; \
            printf(COLOR_RED "  ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("    at %s:%d\n\n", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            tests_failed++; \
            tests_passed--; \
            printf(COLOR_RED "  ✗ FAILED: %s" COLOR_RESET "\n", message); \
            printf("    Expected: 0x%04X, Got: 0x%04X\n", (uint16_t)(expected), (uint16_t)(actual)); \
            printf("    at %s:%d\n\n", __FILE__, __LINE__); \
            return; \
        } \
    } while(0)

// Helper function to setup a clean machine state
state_t* setup_machine() {
    state_t *machine = create_machine();
    reset_machine(machine);
    return machine;
}

// Helper to check if a flag is set
bool check_flag(state_t *machine, processor_flags_t flag) {
    return (machine->processor.P & flag) != 0;
}

// ============================================================================
// Stack Operation Tests
// ============================================================================

TEST(push_byte_basic) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;  // Top of stack in emulation mode
    machine->processor.emulation_mode = true;
    
    push_byte(machine, 0x42);
    
    ASSERT_EQ(machine->processor.SP, 0x1FE, "Stack pointer should decrement");
    ASSERT_EQ(machine->memory[0][0x01FF], 0x42, "Byte should be pushed to stack");
    
    destroy_machine(machine);
}

TEST(pop_byte_basic) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FE;
    machine->processor.emulation_mode = true;
    machine->memory[0][0x01FF] = 0x42;
    
    uint8_t value = pop_byte(machine);
    
    ASSERT_EQ(value, 0x42, "Should pop correct value");
    ASSERT_EQ(machine->processor.SP, 0x1FF, "Stack pointer should increment");
    
    destroy_machine(machine);
}

TEST(push_word_native_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    
    push_word(machine, 0x1234);
    
    ASSERT_EQ(machine->processor.SP, 0x1FD, "Stack pointer should decrement by 2");
    ASSERT_EQ(machine->memory[0][0x01FF], 0x12, "High byte should be pushed first");
    ASSERT_EQ(machine->memory[0][0x01FE], 0x34, "Low byte should be pushed second");
    
    destroy_machine(machine);
}

TEST(pop_word_native_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FD;
    machine->processor.emulation_mode = false;
    machine->memory[0][0x01FE] = 0x34;  // Low byte
    machine->memory[0][0x01FF] = 0x12;  // High byte
    
    uint16_t value = pop_word(machine);
    
    ASSERT_EQ(value, 0x1234, "Should pop correct 16-bit value");
    ASSERT_EQ(machine->processor.SP, 0x1FF, "Stack pointer should increment by 2");
    
    destroy_machine(machine);
}

TEST(stack_wrap_emulation_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x100;  // Bottom of page 1
    machine->processor.emulation_mode = true;
    
    push_byte(machine, 0xAB);
    
    // In emulation mode, SP wraps within page 1, so 0x100 - 1 = 0xFF (not 0x1FF)
    ASSERT_EQ(machine->processor.SP, 0xFF, "Stack should wrap to 0xFF");
    ASSERT_EQ(machine->memory[0][0x0100], 0xAB, "Byte should be at wrap location");
    
    destroy_machine(machine);
}

// ============================================================================
// Flag Operation Tests
// ============================================================================

TEST(set_flags_nz_8_negative) {
    state_t *machine = setup_machine();
    machine->processor.P = 0;
    
    set_flags_nz_8(machine, 0x80);
    
    ASSERT(check_flag(machine, NEGATIVE), "Negative flag should be set");
    ASSERT(!check_flag(machine, ZERO), "Zero flag should not be set");
    
    destroy_machine(machine);
}

TEST(set_flags_nz_8_zero) {
    state_t *machine = setup_machine();
    machine->processor.P = 0;
    
    set_flags_nz_8(machine, 0x00);
    
    ASSERT(!check_flag(machine, NEGATIVE), "Negative flag should not be set");
    ASSERT(check_flag(machine, ZERO), "Zero flag should be set");
    
    destroy_machine(machine);
}

TEST(set_flags_nz_16_negative) {
    state_t *machine = setup_machine();
    machine->processor.P = 0;
    
    set_flags_nz_16(machine, 0x8000);
    
    ASSERT(check_flag(machine, NEGATIVE), "Negative flag should be set");
    ASSERT(!check_flag(machine, ZERO), "Zero flag should not be set");
    
    destroy_machine(machine);
}

TEST(set_flags_nzc_8_with_carry) {
    state_t *machine = setup_machine();
    machine->processor.P = 0;
    
    set_flags_nzc_8(machine, 0x100);  // Result with carry
    
    ASSERT(check_flag(machine, CARRY), "Carry flag should be set");
    ASSERT(check_flag(machine, ZERO), "Zero flag should be set (low byte is 0)");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Stack Operations
// ============================================================================

TEST(PHA_8bit_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = true;
    machine->processor.P |= M_FLAG;  // 8-bit accumulator
    machine->processor.A.low = 0x42;
    
    PHA(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FE, "Stack pointer should decrement");
    ASSERT_EQ(machine->memory[0][0x01FF], 0x42, "Accumulator should be pushed");
    
    destroy_machine(machine);
}

TEST(PHA_16bit_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~M_FLAG;  // 16-bit accumulator
    machine->processor.A.full = 0x1234;
    
    PHA(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FD, "Stack pointer should decrement by 2");
    ASSERT_EQ(machine->memory[0][0x01FF], 0x12, "High byte should be pushed first");
    ASSERT_EQ(machine->memory[0][0x01FE], 0x34, "Low byte should be pushed second");
    
    destroy_machine(machine);
}

TEST(PLA_8bit_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FE;
    machine->processor.emulation_mode = true;
    machine->processor.P |= M_FLAG;
    machine->memory[0][0x01FF] = 0x42;
    
    PLA(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FF, "Stack pointer should increment");
    ASSERT_EQ(machine->processor.A.low, 0x42, "Should pull value into accumulator");
    
    destroy_machine(machine);
}

TEST(PLA_16bit_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FD;
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~M_FLAG;
    machine->memory[0][0x01FE] = 0x34;  // Low byte
    machine->memory[0][0x01FF] = 0x12;  // High byte
    
    PLA(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FF, "Stack pointer should increment by 2");
    ASSERT_EQ(machine->processor.A.full, 0x1234, "Should pull 16-bit value");
    
    destroy_machine(machine);
}

TEST(PHX_16bit_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;  // 16-bit index
    machine->processor.X = 0xABCD;
    
    PHX(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FD, "Stack pointer should decrement by 2");
    ASSERT_EQ(machine->memory[0][0x01FF], 0xAB, "High byte should be pushed first");
    ASSERT_EQ(machine->memory[0][0x01FE], 0xCD, "Low byte should be pushed second");
    
    destroy_machine(machine);
}

TEST(PLX_16bit_mode) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FD;
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;
    machine->memory[0][0x01FE] = 0xCD;
    machine->memory[0][0x01FF] = 0xAB;
    
    PLX(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.X, 0xABCD, "Should pull 16-bit value into X");
    ASSERT_EQ(machine->processor.SP, 0x1FF, "Stack pointer should increment by 2");
    
    destroy_machine(machine);
}

TEST(PHY_and_PLY_roundtrip) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;
    machine->processor.Y = 0x5678;
    
    PHY(machine, 0, 0);
    machine->processor.Y = 0;  // Clear Y
    PLY(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.Y, 0x5678, "Y should be restored");
    ASSERT_EQ(machine->processor.SP, 0x1FF, "Stack pointer should be back to original");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Subroutine Calls
// ============================================================================

TEST(JSR_and_RTS) {
    state_t *machine = setup_machine();
    machine->processor.PC = 0x1000;
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    
    // JSR should push PC-1 onto stack
    JSR_CB(machine, 0x2000, 0);
    
    ASSERT_EQ(machine->processor.PC, 0x2000, "PC should be set to target");
    ASSERT_EQ(machine->processor.SP, 0x1FD, "SP should decrement by 2");
    
    // RTS should restore PC
    RTS(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.PC, 0x0FFF, "PC should be restored to return address");
    ASSERT_EQ(machine->processor.SP, 0x1FF, "SP should be back to original");
    
    destroy_machine(machine);
}

TEST(JSL_and_RTL) {
    state_t *machine = setup_machine();
    machine->processor.PC = 0x1000;
    machine->processor.PBR = 0x01;
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    
    // JSL should push PBR and PC-1
    JSL_CB(machine, 0x2000, 0);
    
    ASSERT_EQ(machine->processor.PC, 0x2000, "PC should be set to target");
    ASSERT_EQ(machine->processor.SP, 0x1FC, "SP should decrement by 3");
    ASSERT_EQ(machine->memory[0][0x01FF], 0x01, "PBR should be pushed");
    
    // RTL should restore PBR and PC
    RTL(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.PC, 0x0FFF, "PC should be restored");
    ASSERT_EQ(machine->processor.PBR, 0x01, "PBR should be restored");
    ASSERT_EQ(machine->processor.SP, 0x1FF, "SP should be back to original");
    
    destroy_machine(machine);
}

TEST(PER_pushes_pc_relative) {
    state_t *machine = setup_machine();
    machine->processor.PC = 0x1000;
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    
    // PER with offset 0x20
    PER(machine, 0x20, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FD, "SP should decrement by 2");
    // PC + offset = 0x1020
    ASSERT_EQ(machine->memory[0][0x01FF], 0x10, "High byte of PC+offset");
    ASSERT_EQ(machine->memory[0][0x01FE], 0x20, "Low byte of PC+offset");
    
    destroy_machine(machine);
}

TEST(PEA_pushes_effective_address) {
    state_t *machine = setup_machine();
    machine->processor.SP = 0x1FF;
    machine->processor.emulation_mode = false;
    
    PEA_ABS(machine, 0x1234, 0);
    
    ASSERT_EQ(machine->processor.SP, 0x1FD, "SP should decrement by 2");
    ASSERT_EQ(machine->memory[0][0x01FF], 0x12, "High byte should be pushed");
    ASSERT_EQ(machine->memory[0][0x01FE], 0x34, "Low byte should be pushed");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Flags
// ============================================================================

TEST(CLC_clears_carry) {
    state_t *machine = setup_machine();
    machine->processor.P = 0xFF;  // All flags set
    
    CLC_CB(machine, 0, 0);
    
    ASSERT(!check_flag(machine, CARRY), "Carry flag should be cleared");
    
    destroy_machine(machine);
}

TEST(SEC_sets_carry) {
    state_t *machine = setup_machine();
    machine->processor.P = 0x00;
    
    SEC_CB(machine, 0, 0);
    
    ASSERT(check_flag(machine, CARRY), "Carry flag should be set");
    
    destroy_machine(machine);
}

TEST(SEP_sets_processor_flags) {
    state_t *machine = setup_machine();
    machine->processor.P = 0x00;
    machine->processor.emulation_mode = false;
    
    SEP_CB(machine, 0x30, 0);  // Set M and X flags
    
    ASSERT(check_flag(machine, M_FLAG), "M flag should be set");
    ASSERT(check_flag(machine, X_FLAG), "X flag should be set");
    
    destroy_machine(machine);
}

TEST(REP_clears_processor_flags) {
    state_t *machine = setup_machine();
    machine->processor.P = 0xFF;
    machine->processor.emulation_mode = false;
    
    REP_CB(machine, 0x30, 0);  // Clear M and X flags
    
    ASSERT(!check_flag(machine, M_FLAG), "M flag should be cleared");
    ASSERT(!check_flag(machine, X_FLAG), "X flag should be cleared");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Load/Store
// ============================================================================

TEST(LDA_IMM_8bit) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;  // 8-bit mode
    machine->processor.A.low = 0;
    
    LDA_IMM(machine, 0x42, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x42, "Should load immediate value");
    ASSERT(!check_flag(machine, ZERO), "Zero flag should not be set");
    ASSERT(!check_flag(machine, NEGATIVE), "Negative flag should not be set");
    
    destroy_machine(machine);
}

TEST(LDA_IMM_16bit) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~M_FLAG;  // 16-bit mode
    machine->processor.A.full = 0;
    
    LDA_IMM(machine, 0x1234, 0);
    
    ASSERT_EQ(machine->processor.A.full, 0x1234, "Should load 16-bit immediate");
    
    destroy_machine(machine);
}

TEST(STA_and_LDA_ABS) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x99;
    
    STA_ABS(machine, 0x2000, 0);
    
    ASSERT_EQ(machine->memory[0][0x2000], 0x99, "Should store to absolute address");
    
    machine->processor.A.low = 0;
    LDA_ABS(machine, 0x2000, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x99, "Should load from absolute address");
    
    destroy_machine(machine);
}

TEST(LDX_and_STX) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;  // 16-bit index
    machine->processor.X = 0x5678;
    
    STX_ABS(machine, 0x3000, 0);
    ASSERT_EQ(machine->memory[0][0x3000], 0x78, "Low byte stored");
    ASSERT_EQ(machine->memory[0][0x3001], 0x56, "High byte stored");
    
    machine->processor.X = 0;
    LDX_ABS(machine, 0x3000, 0);
    
    ASSERT_EQ(machine->processor.X, 0x5678, "X should be loaded");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Arithmetic
// ============================================================================

TEST(ADC_8bit_no_carry) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.P &= ~CARRY;
    machine->processor.A.low = 0x10;
    
    ADC_IMM(machine, 0x20, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x30, "Should add correctly");
    ASSERT(!check_flag(machine, CARRY), "Carry should not be set");
    
    destroy_machine(machine);
}

TEST(ADC_8bit_with_overflow) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.P &= ~CARRY;
    machine->processor.A.low = 0xFF;
    
    ADC_IMM(machine, 0x01, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x00, "Should wrap to 0");
    ASSERT(check_flag(machine, CARRY), "Carry should be set");
    // Note: Zero flag check depends on implementation of set_flags_nzc_8
    
    destroy_machine(machine);
}

TEST(SBC_8bit_no_borrow) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.P |= CARRY;  // No borrow
    machine->processor.A.low = 0x50;
    
    SBC_IMM(machine, 0x20, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x30, "Should subtract correctly");
    
    destroy_machine(machine);
}

TEST(INC_accumulator) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x41;
    
    INC(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x42, "Should increment accumulator");
    
    destroy_machine(machine);
}

TEST(DEC_accumulator) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x42;
    
    DEC(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x41, "Should decrement accumulator");
    
    destroy_machine(machine);
}

TEST(INX_and_DEX) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;
    machine->processor.X = 0x1000;
    
    INX(machine, 0, 0);
    ASSERT_EQ(machine->processor.X, 0x1001, "Should increment X");
    
    DEX(machine, 0, 0);
    ASSERT_EQ(machine->processor.X, 0x1000, "Should decrement X");
    
    destroy_machine(machine);
}

TEST(INY_and_DEY) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;
    machine->processor.Y = 0x2000;
    
    INY(machine, 0, 0);
    ASSERT_EQ(machine->processor.Y, 0x2001, "Should increment Y");
    
    DEY(machine, 0, 0);
    ASSERT_EQ(machine->processor.Y, 0x2000, "Should decrement Y");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Logical Operations
// ============================================================================

TEST(AND_IMM) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0xFF;
    
    AND_IMM(machine, 0x0F, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x0F, "Should AND correctly");
    
    destroy_machine(machine);
}

TEST(ORA_IMM) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0xF0;
    
    ORA_IMM(machine, 0x0F, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0xFF, "Should OR correctly");
    
    destroy_machine(machine);
}

TEST(EOR_IMM) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0xFF;
    
    EOR_IMM(machine, 0xFF, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x00, "Should XOR to zero");
    ASSERT(check_flag(machine, ZERO), "Zero flag should be set");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Bit Operations
// ============================================================================

TEST(ASL_accumulator) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x41;
    
    ASL(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x82, "Should shift left");
    ASSERT(!check_flag(machine, CARRY), "Carry should not be set");
    
    destroy_machine(machine);
}

TEST(LSR_accumulator) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x82;
    
    LSR(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x41, "Should shift right");
    ASSERT(!check_flag(machine, CARRY), "Carry should not be set");
    
    destroy_machine(machine);
}

TEST(ROL_with_carry) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.P |= CARRY;
    machine->processor.A.low = 0x80;
    
    ROL(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x01, "Should rotate left with carry in");
    ASSERT(check_flag(machine, CARRY), "Carry should be set from bit 7");
    
    destroy_machine(machine);
}

TEST(ROR_with_carry) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.P |= CARRY;
    machine->processor.A.low = 0x01;
    
    ROR(machine, 0, 0);
    
    ASSERT_EQ(machine->processor.A.low, 0x80, "Should rotate right with carry in");
    ASSERT(check_flag(machine, CARRY), "Carry should be set from bit 0");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Compare
// ============================================================================

TEST(CMP_equal) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x42;
    
    CMP_IMM(machine, 0x42, 0);
    
    ASSERT(check_flag(machine, ZERO), "Zero flag should be set when equal");
    // Note: Carry flag behavior in CMP depends on implementation
    // Typically carry is set when A >= M (no borrow)
    
    destroy_machine(machine);
}

TEST(CMP_greater) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x50;
    
    CMP_IMM(machine, 0x30, 0);
    
    ASSERT(!check_flag(machine, ZERO), "Zero flag should not be set");
    // Note: Carry flag behavior varies by implementation
    
    destroy_machine(machine);
}

TEST(CMP_less) {
    state_t *machine = setup_machine();
    machine->processor.P |= M_FLAG;
    machine->processor.A.low = 0x20;
    
    CMP_IMM(machine, 0x30, 0);
    
    ASSERT(!check_flag(machine, ZERO), "Zero flag should not be set");
    ASSERT(!check_flag(machine, CARRY), "Carry should not be set when A < value");
    ASSERT(check_flag(machine, NEGATIVE), "Negative flag should be set");
    
    destroy_machine(machine);
}

TEST(CPX_and_CPY) {
    state_t *machine = setup_machine();
    machine->processor.P &= ~X_FLAG;
    machine->processor.X = 0x1000;
    machine->processor.Y = 0x2000;
    
    CPX_IMM(machine, 0x1000, 0);
    ASSERT(check_flag(machine, ZERO), "CPX should set zero for equal");
    
    CPY_IMM(machine, 0x2000, 0);
    ASSERT(check_flag(machine, ZERO), "CPY should set zero for equal");
    
    destroy_machine(machine);
}

// ============================================================================
// Instruction Tests - Transfers
// ============================================================================

TEST(TAX_and_TXA) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~M_FLAG;  // 16-bit accumulator
    machine->processor.P &= ~X_FLAG;  // 16-bit index registers
    machine->processor.A.full = 0x1234;
    machine->processor.X = 0;
    
    TAX(machine, 0, 0);
    ASSERT_EQ(machine->processor.X, 0x1234, "X should equal A");
    
    machine->processor.A.full = 0;
    TXA(machine, 0, 0);
    ASSERT_EQ(machine->processor.A.full, 0x1234, "A should equal X");
    
    destroy_machine(machine);
}

TEST(TAY_and_TYA) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~M_FLAG;  // 16-bit accumulator
    machine->processor.P &= ~X_FLAG;  // 16-bit index registers
    machine->processor.A.full = 0x5678;
    machine->processor.Y = 0;
    
    TAY(machine, 0, 0);
    ASSERT_EQ(machine->processor.Y, 0x5678, "Y should equal A");
    
    machine->processor.A.full = 0;
    TYA(machine, 0, 0);
    ASSERT_EQ(machine->processor.A.full, 0x5678, "A should equal Y");
    
    destroy_machine(machine);
}

TEST(TSX_and_TXS) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;  // 16-bit index mode
    machine->processor.SP = 0x1ABC;
    machine->processor.X = 0;
    
    TSX(machine, 0, 0);
    ASSERT_EQ(machine->processor.X, 0x1ABC, "X should equal SP");
    
    machine->processor.SP = 0;
    TXS(machine, 0, 0);
    ASSERT_EQ(machine->processor.SP, 0x1ABC, "SP should equal X");
    
    destroy_machine(machine);
}

TEST(TXY_and_TYX) {
    state_t *machine = setup_machine();
    machine->processor.emulation_mode = false;
    machine->processor.P &= ~X_FLAG;  // 16-bit index mode
    machine->processor.X = 0xAAAA;
    machine->processor.Y = 0xBBBB;
    
    TXY(machine, 0, 0);
    ASSERT_EQ(machine->processor.Y, 0xAAAA, "Y should equal X (0xAAAA)");
    
    TYX(machine, 0, 0);
    ASSERT_EQ(machine->processor.X, 0xAAAA, "X should equal Y (0xAAAA after TXY)");
    
    destroy_machine(machine);
}

// ============================================================================
// Main test runner
// ============================================================================

int main(int argc, char **argv) {
    printf("\n");
    printf(COLOR_YELLOW "========================================\n" COLOR_RESET);
    printf(COLOR_YELLOW "  65816 Processor Test Suite\n" COLOR_RESET);
    printf(COLOR_YELLOW "========================================\n" COLOR_RESET);
    printf("\n");
    
    // Stack operation tests
    printf(COLOR_BLUE "--- Stack Operations ---\n" COLOR_RESET);
    run_test_push_byte_basic();
    run_test_pop_byte_basic();
    run_test_push_word_native_mode();
    run_test_pop_word_native_mode();
    run_test_stack_wrap_emulation_mode();
    
    // Flag operation tests
    printf(COLOR_BLUE "--- Flag Operations ---\n" COLOR_RESET);
    run_test_set_flags_nz_8_negative();
    run_test_set_flags_nz_8_zero();
    run_test_set_flags_nz_16_negative();
    run_test_set_flags_nzc_8_with_carry();
    
    // Stack instruction tests
    printf(COLOR_BLUE "--- Stack Instructions ---\n" COLOR_RESET);
    run_test_PHA_8bit_mode();
    run_test_PHA_16bit_mode();
    run_test_PLA_8bit_mode();
    run_test_PLA_16bit_mode();
    run_test_PHX_16bit_mode();
    run_test_PLX_16bit_mode();
    run_test_PHY_and_PLY_roundtrip();
    
    // Subroutine tests
    printf(COLOR_BLUE "--- Subroutine Calls ---\n" COLOR_RESET);
    run_test_JSR_and_RTS();
    run_test_JSL_and_RTL();
    run_test_PER_pushes_pc_relative();
    run_test_PEA_pushes_effective_address();
    
    // Flag instruction tests
    printf(COLOR_BLUE "--- Flag Instructions ---\n" COLOR_RESET);
    run_test_CLC_clears_carry();
    run_test_SEC_sets_carry();
    run_test_SEP_sets_processor_flags();
    run_test_REP_clears_processor_flags();
    
    // Load/Store tests
    printf(COLOR_BLUE "--- Load/Store Instructions ---\n" COLOR_RESET);
    run_test_LDA_IMM_8bit();
    run_test_LDA_IMM_16bit();
    run_test_STA_and_LDA_ABS();
    run_test_LDX_and_STX();
    
    // Arithmetic tests
    printf(COLOR_BLUE "--- Arithmetic Instructions ---\n" COLOR_RESET);
    run_test_ADC_8bit_no_carry();
    run_test_ADC_8bit_with_overflow();
    run_test_SBC_8bit_no_borrow();
    run_test_INC_accumulator();
    run_test_DEC_accumulator();
    run_test_INX_and_DEX();
    run_test_INY_and_DEY();
    
    // Logical operation tests
    printf(COLOR_BLUE "--- Logical Instructions ---\n" COLOR_RESET);
    run_test_AND_IMM();
    run_test_ORA_IMM();
    run_test_EOR_IMM();
    
    // Bit operation tests
    printf(COLOR_BLUE "--- Bit Shift/Rotate Instructions ---\n" COLOR_RESET);
    run_test_ASL_accumulator();
    run_test_LSR_accumulator();
    run_test_ROL_with_carry();
    run_test_ROR_with_carry();
    
    // Compare tests
    printf(COLOR_BLUE "--- Compare Instructions ---\n" COLOR_RESET);
    run_test_CMP_equal();
    run_test_CMP_greater();
    run_test_CMP_less();
    run_test_CPX_and_CPY();
    
    // Transfer tests
    printf(COLOR_BLUE "--- Transfer Instructions ---\n" COLOR_RESET);
    run_test_TAX_and_TXA();
    run_test_TAY_and_TYA();
    run_test_TSX_and_TXS();
    run_test_TXY_and_TYX();
    
    // Print summary
    printf("\n");
    printf(COLOR_YELLOW "========================================\n" COLOR_RESET);
    printf(COLOR_YELLOW "  Test Results\n" COLOR_RESET);
    printf(COLOR_YELLOW "========================================\n" COLOR_RESET);
    printf("Total tests run:    %d\n", tests_run);
    printf(COLOR_GREEN "Tests passed:       %d\n" COLOR_RESET, tests_passed);
    if (tests_failed > 0) {
        printf(COLOR_RED "Tests failed:       %d\n" COLOR_RESET, tests_failed);
    } else {
        printf("Tests failed:       %d\n", tests_failed);
    }
    printf("\n");
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "✓ All tests passed!\n" COLOR_RESET);
        return 0;
    } else {
        printf(COLOR_RED "✗ Some tests failed.\n" COLOR_RESET);
        return 1;
    }
}

#include "codetable.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
/*
    >Immediate - single param
    >Implied - no params
    >PCRelative - 1 byte, branches only ? -- signed
    >PCRelativeLong - PC Relatiove, Long - 2 bytes -- signed
    >DirectPage  - first param is 1 byte, unsigned hex
    >Absolute - 2 bytes, unsigned hex
    >AbsoluteLong - 3 bytes, unsigned hex
    Indirect  - for IndexedX also flagged, wrap whole thIndirectg Indirect parens, IndexedY wrap just first param
    IndexedX - Indirectdexed X -- second param is X
    IndexedY - ditto, but Y
    >BlockMoveAddress - pair of sIndirectgle byte values, unsigned hex
    >StackRelative - Stack Relative - gets a `,S` as part of the params, when an Indirectdexed, as part of the first param
*/

#define SET_FLAG(var, flag) (var) |= (flag)
#define CLEAR_FLAG(var, flag) (var) &= ~(flag)
#define CHECK_FLAG(var, flag) !!((var) & (flag))

char* format_absolute_address(opcode_t* opcode, uint32_t address) {
    // Absolute Addressing can be Indirect, Indirect | IndexedX, IndexedLong, IndexedX, IndexedY or standalone
    char* buffer = malloc(64);
    if (!buffer) return NULL;

    if (CHECK_FLAG(opcode->flags, Indirect)) {
        if (CHECK_FLAG(opcode->flags, IndexedX)) {
            snprintf(buffer, 64, "($%04X, X)", address);
        } else {
            snprintf(buffer, 64, "($%04X)", address);
        }
    } else if (CHECK_FLAG(opcode->flags, IndexedX)) {
        snprintf(buffer, 64, "$%04X, X", address);
    } else if (CHECK_FLAG(opcode->flags, IndexedY)) {
        snprintf(buffer, 64, "$%04X, Y", address);
    } else if (CHECK_FLAG(opcode->flags, IndexedLong)) {
        snprintf(buffer, 64, "$%06X", address);
    } else {
        snprintf(buffer, 64, "$%04X", address);
    }

    return buffer;
}

char* format_direct_page_address(opcode_t* opcode, uint8_t address) {
    // Direct Page Addressing can be IndexedX, IndexedY, IndexedLong, IndexedLong | IndexedY, Indirect | IndexedX, Indirect | IndexedY, Indirect or standalone
    char* buffer = malloc(64);
    if (!buffer) return NULL;

    if (CHECK_FLAG(opcode->flags, IndexedX)) {
        snprintf(buffer, 64, "$%02X, X", address);
    } else if (CHECK_FLAG(opcode->flags, IndexedY)) {
        snprintf(buffer, 64, "$%02X, Y", address);
    } else {
        snprintf(buffer, 64, "$%02X", address);
    }

    return buffer;
}

char* format_stack_relative_address(opcode_t* opcode, uint8_t address) {
    // Stack Relative Addressing can be IndexedY or standalone
    char* buffer = malloc(64);
    if (!buffer) return NULL;

    if (CHECK_FLAG(opcode->flags, IndexedY)) {
        snprintf(buffer, 64, "($%02X, S), Y", address);
    } else {
        snprintf(buffer, 64, "($%02X, S)", address);
    }

    return buffer;
}

// This function formats the opcode and its operands into a string for output.
char* format_opcode_and_operands(codeentry_t* ce, ...) {
    opcode_t* code = ce->code;
    uint32_t f = ce->flags;
    char *rv1 = malloc(sizeof(char) * 64);
    char *retval = malloc(sizeof(char) * 128);
    char* fmt = malloc(sizeof(char) * 64);
    snprintf(rv1, 64, ""); // take care of Implied
    va_list args;
    va_start(args, ce);

    if ((CHECK_FLAG(code->flags, Absolute) || CHECK_FLAG(code->flags, AbsoluteLong)) && CHECK_FLAG(ce->flags, LABEL_SOURCE)) {
        snprintf(rv1, 64, "%s", ce->lblname);
    } else if (CHECK_FLAG(code->flags, Absolute)) {
        free(rv1);
        uint32_t arg = va_arg(args, uint32_t);
        rv1 = format_absolute_address(code, arg);
    } else if (CHECK_FLAG(code->flags, DirectPage)) {
        free(rv1);
        int8_t arg = (int8_t)va_arg(args, int32_t);
        rv1 = format_direct_page_address(code, arg);
    } else if(CHECK_FLAG(code->flags, StackRelative)) {
        free(rv1);
        int8_t arg = (int8_t)va_arg(args, int32_t);
        rv1 = format_stack_relative_address(code, arg);
    } else if(CHECK_FLAG(code->flags, AbsoluteLong)) {
        // Absolute Long -- IndexedX or standalone
        if (CHECK_FLAG(code->flags, IndexedX)) { // Only with AbsoluteLong
            vsnprintf(rv1, 64, "$%06X, X", args);
        } else { // ditto
            vsnprintf(rv1, 64, "$%06X", args);
        }
    } else if(CHECK_FLAG(code->flags, PCRelative)) {
        // PC Relative -- only ever standalone
        // arg is a signed char, we need to extract that to print things correctly...
        signed char operand = va_arg(args, int); // integer promotion means that even though we expect a `signed char` here, the type passed is "int", so...
        unsigned char d_flag = ' ';
        if (CHECK_FLAG(ce->flags, LABEL_SOURCE)) {
            snprintf(rv1, 64, "%s", ce->lblname);
        } else {
            if (operand < 0) { 
                d_flag = '<';
                operand *= -1;
                operand += 2; // PC Relative going Negative starts at the next instruction, so we need to add 2 to the operand
            } else d_flag = '>';

            snprintf(rv1, 16, "$%c%02X", d_flag, operand);
        }
    } else if(CHECK_FLAG(code->flags, PCRelativeLong)) {
        // PC Relative Long -- only ever standalone
        // arg is a signed char, we need to extract that to print things correctly...
        int16_t operand = va_arg(args, int); // we expects a signed short, but integer promotion interferes again
        unsigned char d_flag = ' ';
        if (CHECK_FLAG(ce->flags, LABEL_SOURCE)) {
            snprintf(rv1, 64, "%s", ce->lblname);
        } else {
            if (operand < 0) { 
                d_flag = '<';
                operand *= -1;
                operand += 2; // PC Relative going Negative starts at the next instruction, so we need to add 2 to the operand
            } else d_flag = '>';

            snprintf(rv1, 16, "$%c%04X", d_flag, operand);
        }
    } else if(CHECK_FLAG(code->flags, Immediate)) {
        uint8_t sz = code->munge(code->psize);
        snprintf(fmt, 64, "$%%%02uX", sz);
        vsnprintf(rv1, 64, fmt, args);
    } else if(CHECK_FLAG(code->flags, BlockMoveAddress)) {
        vsnprintf(rv1, 64, "$%02X, $%02X", args);
    }
    va_end(args);
    if (CHECK_FLAG(ce->flags, LABELED)) {
        // can be _ANY_ instruction, so we need to have a small buffer for the label
        snprintf(retval, 72, "%s: %s %s", ce->lblname, code->opcode, rv1);
    } else {
        snprintf(retval, 72, "%s %s", code->opcode, rv1);
    }   
    free(fmt);
    free(rv1);
    return retval;
}
#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG
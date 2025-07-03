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
    fprintf(stderr, "format_opcode_and_operands: %p\n", ce);
    fprintf(stderr, "format_opcode_and_operands: %p\n", ce->code);
    fprintf(stderr, "format_opcode_and_operands: 0x%08X -- 0x%08X\n", code->flags, ce->flags);
    if ((CHECK_FLAG(code->flags, Absolute) || CHECK_FLAG(code->flags, AbsoluteLong)) && CHECK_FLAG(ce->flags, LABEL_SOURCE)) {
        snprintf(fmt, 64, "%s", ce->lblname);
    } else if (CHECK_FLAG(code->flags, Absolute) || CHECK_FLAG(code->flags, DirectPage)) {
        // Absolute Addressing can be Indirect, Indirect | IndexedX, IndexedLong, IndexedX, IndexedY or standalone
        // Direct Page addressing can be IndexedX, IndexedY, IndexedLong, IndexedLong | IndexedY, Indirect | IndexedX, Indirect | IndexedY, Indirect or standalone
        uint8_t sz = CHECK_FLAG(code->flags, DirectPage)?2:4;
        if (CHECK_FLAG(code->flags, Indirect)) {
            if (CHECK_FLAG(code->flags, IndexedX)) {
                snprintf(fmt, 64, "($0x%%%02uX, X)", sz);
            } else if(CHECK_FLAG(code->flags, IndexedY)) {
                snprintf(fmt, 64, "($0x%%%02uX), Y", sz);
            } else {
                snprintf(fmt, 64, "($0x%%%02uX)", sz);
            }
        } else if(CHECK_FLAG(code->flags, IndexedLong)) {
            if (CHECK_FLAG(code->flags, IndexedY)) {
            } else {
                snprintf(fmt, 64, "[$0x%%%02uX]", sz);
            }
        } else if(CHECK_FLAG(code->flags, IndexedX)) {
            snprintf(fmt, 64, "$0x%%%02uX, X", sz);
        } else if(CHECK_FLAG(code->flags, IndexedY)) {
            snprintf(fmt, 64, "$0x%%%02uX, Y", sz);
        } else {
            snprintf(fmt, 64, "$0x%%%02uX", sz);
        }
        vsnprintf(rv1, 64, fmt, args);
    } else if(CHECK_FLAG(code->flags, StackRelative) || CHECK_FLAG(code->flags, AbsoluteLong)) {
        // Stack Relative -- Indirect | IndexedY or standalone
        // Absolute Long -- IndexedX or standalone
        if (CHECK_FLAG(code->flags, Indirect) && CHECK_FLAG(code->flags, IndexedY)) {
            vsnprintf(rv1, 64, "($0x%02X, S), Y", args); // this should only ever happen with StackRelative, so this is safe
        } else if (CHECK_FLAG(code->flags, IndexedX)) { // Only with AbsoluteLong
            vsnprintf(rv1, 64, "$0x%06X, X", args);
        } else { // ditto
            vsnprintf(rv1, 64, "$0x%06X", args);
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

            snprintf(rv1, 16, "$%c0x%02X", d_flag, operand);
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

            snprintf(rv1, 16, "$%c0x%02X", d_flag, operand);
        }
    } else if(CHECK_FLAG(code->flags, Immediate)) {
        uint8_t sz = code->munge(code->psize);
        snprintf(fmt, 64, "$0x%%%02uX", sz);
        snprintf(fmt, 64, "#0x%02u", sz);
        vsnprintf(rv1, 64, fmt, args);
        free(fmt);
    } else if(CHECK_FLAG(code->flags, BlockMoveAddress)) {
        vsnprintf(rv1, 64, "$0x%02X, $0x%02X", args);
    }
    va_end(args);
    if (CHECK_FLAG(ce->flags, LABELED)) {
        // can be _ANY_ instruction, so we need to have a small buffer for the label
        snprintf(retval, 72, "%s: %s %s", ce->lblname, code->opcode, rv1);
    } else {
        snprintf(retval, 72, "%s %s\n", code->opcode, rv1);
    }   
    free(fmt);
    free(rv1);
    return retval;
}
#undef SET_FLAG
#undef CLEAR_FLAG
#undef CHECK_FLAG
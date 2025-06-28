#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "state.h"
#include "ops.h"

int m_set(int sz) {
    return isMSet()?sz+1:sz;
}

int x_set(int sz) {
    return isXSet()?sz+1:sz;
}

int base(int sz) {
    return sz;
}

void JMP(uint32_t target_offset, uint32_t source_offset) {
    // TODO: stub!
    uint16_t addr = va_arg(args, int);
    if (isESet()) {
        // 6502 emulation mode
        addr &= 0xFFFF; // mask to 16 bits
    }
    // make a label -- TODO: fix the f*cking naming to be dynamic
    make_label(source_offset, target_offset, "JMP_LABEL");
}

void BRL(uint32_t target_offset, uint32_t source_offset) {
    // TODO: stub!
    uint16_t addr = va_arg(args, int);
    if (isESet()) {
        // 6502 emulation mode
        addr &= 0xFFFF; // mask to 16 bits
    }
    // make a label -- TODO: fix the f*cking naming to be dynamic
    make_label(source_offset, target_offset, "LOCAL_LONG");
}

void BRA(uint32_t target_offset, uint32_t source_offset) {
    // TODO: stub!
    uint16_t addr = va_arg(args, int);
    if (isESet()) {
        // 6502 emulation mode
        addr &= 0xFFFF; // mask to 16 bits
    }
    // make a label -- TODO: fix the f*cking naming to be dynamic
    make_label(source_offset, target_offset, "LOCAL_SHORT");
}

// HACK! Fix this at some point!
extern int READ_BMA(bool);
extern int READ_8(bool);
extern int READ_8_16(bool);
extern int READ_16(bool);
extern int READ_24(bool);

// Note that BRK and COP both consume the byte following them, though I cannot find any documentation saying BRK has a parameter.
// To cover for this, we give it a dummy parameter that gets read from the byte-stream.
const opcode_t opcodes[256] = {
    { "BRK", 01,  base, NULL, NULL,    READ_8,                            Immediate /* BRK        s */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* ORA    (d,x) */ }, // 00
    { "COP", 01,  base, NULL, NULL,    READ_8,                            Immediate /* COP        # */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* ORA      d,s */ }, // 02
    { "TSB", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* TSB        d */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* ORA        d */ }, // 04
    { "ASL", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* ASL        d */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* ORA      [d] */ }, // 06
    { "PHP", 00,  base, NULL, NULL,      NULL,                              Implied /* PHP        s */ }, { "ORA", 01, m_set, NULL, NULL, READ_8_16,                            Immediate /* ORA        # */ }, // 08
    { "ASL", 00,  base, NULL, NULL,      NULL,                              Implied /* ASL        A */ }, { "PHD", 00,  base, NULL, NULL,      NULL,                              Implied /* PHD        s */ }, // 0A
    { "TSB", 02,  base, NULL, NULL,   READ_16,                             Absolute /* TSB        a */ }, { "ORA", 02,  base, NULL, NULL,   READ_16,                             Absolute /* ORA        a */ }, // 0C
    { "ASL", 02,  base, NULL, NULL,   READ_16,                             Absolute /* ASL        a */ }, { "ORA", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* ORA       al */ }, // 0E
    { "BPL", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BPL        r */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* ORA    (d),y */ }, // 10
    { "ORA", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* ORA      (d) */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* ORA  (d,s),y */ }, // 12
    { "TRB", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* TRB        d */ }, { "ORA", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* ORA      d,x */ }, // 14
    { "ASL", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* ASL      d,x */ }, { "ORA", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* ORA    [d],y */ }, // 16
    { "CLC", 00,  base,  CLC, NULL,      NULL,                              Implied /* CLC        i */ }, { "ORA", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* ORA      a,y */ }, // 18
    { "INC", 00,  base, NULL, NULL,      NULL,                              Implied /* INC        A */ }, { "TCS", 00,  base, NULL, NULL,      NULL,                              Implied /* TCS        i */ }, // 1A
    { "TRB", 02,  base, NULL, NULL,   READ_16,                             Absolute /* TRB        a */ }, { "ORA", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* ORA      a,x */ }, // 1C
    { "ASL", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* ASL      a,x */ }, { "ORA", 03,  base, NULL, NULL,   READ_24,              AbsoluteLong | IndexedX /* ORA     al,x */ }, // 1E
    { "JSR", 02,  base, NULL,  JMP,  READ_16 ,                             Absolute /* JSR        a */ }, { "AND", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* AND    (d,x) */ }, // 20
    { "JSL", 03,  base, NULL,  JMP,   READ_24,                         AbsoluteLong /* JSL       al */ }, { "AND", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* AND      d,s */ }, // 22
    { "BIT", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* BIT        d */ }, { "AND", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* AND        d */ }, // 24
    { "ROL", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* ROL        d */ }, { "AND", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* AND      [d] */ }, // 26
    { "PLP", 00,  base, NULL, NULL,      NULL,                              Implied /* PLP        s */ }, { "AND", 01, m_set, NULL, NULL, READ_8_16,                            Immediate /* AND        # */ }, // 28
    { "ROL", 00,  base, NULL, NULL,      NULL,                              Implied /* ROL        A */ }, { "PLD", 00,  base, NULL, NULL,      NULL,                              Implied /* PLD        s */ }, // 2A
    { "BIT", 02,  base, NULL, NULL,   READ_16,                             Absolute /* BIT        a */ }, { "AND", 02,  base, NULL, NULL,   READ_16,                             Absolute /* AND        a */ }, // 2C
    { "ROL", 02,  base, NULL, NULL,   READ_16,                             Absolute /* ROL        a */ }, { "AND", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* AND       al */ }, // 2E
    { "BMI", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BMI        R */ }, { "AND", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* AND    (d),y */ }, // 30
    { "AND", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* AND      (d) */ }, { "AND", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* AND  (d,s),y */ }, // 32
    { "BIT", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* BIT      d,x */ }, { "AND", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* AND      d,x */ }, // 34
    { "ROL", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* ROL      d,x */ }, { "AND", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* AND    [d],y */ }, // 36
    { "SEC", 00,  base,  SEC, NULL,      NULL,                              Implied /* SEC        i */ }, { "AND", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* AND      a,y */ }, // 38
    { "DEC", 00,  base, NULL, NULL,      NULL,                              Implied /* DEC        A */ }, { "TSC", 00,  base, NULL, NULL,      NULL,                              Implied /* TSC        i */ }, // 3A
    { "BIT", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* BIT      a,x */ }, { "AND", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* AND      a,x */ }, // 3C
    { "ROL", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* ROL      a,x */ }, { "AND", 03,  base, NULL, NULL,   READ_24,   Absolute | IndirectLong | IndexedX /* AND     al,x */ }, // 3E
    { "RTI", 00,  base, NULL, NULL,      NULL,                              Implied /* RTI        s */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* EOR    (d,x) */ }, // 40
    { "WDM", 01,  base, NULL, NULL,    READ_8,                              Implied /* WDM        i */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* EOR      d,s */ }, // 42
    { "MVP", 02,  base, NULL, NULL,  READ_BMA,                     BlockMoveAddress /* MVP  src,dst */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* EOR        d */ }, // 44
    { "LSR", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* LSR        d */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* EOR      [d] */ }, // 46
    { "PHA", 00,  base, NULL, NULL,      NULL,                              Implied /* PHA        s */ }, { "EOR", 01, m_set, NULL, NULL, READ_8_16,                            Immediate /* EOR        # */ }, // 48
    { "LSR", 00,  base, NULL, NULL,      NULL,                              Implied /* LSR        A */ }, { "PHK", 00,  base, NULL, NULL,      NULL,                              Implied /* PHK        s */ }, // 4A
    { "JMP", 02,  base, NULL,  JMP,   READ_16,                             Absolute /* JMP        a */ }, { "EOR", 02,  base, NULL, NULL,   READ_16,                             Absolute /* EOR        a */ }, // 4C
    { "LSR", 02,  base, NULL, NULL,   READ_16,                             Absolute /* LSR        a */ }, { "EOR", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* EOR       al */ }, // 4E
    { "BVC", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BVC        r */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* EOR    (d),y */ }, // 50
    { "EOR", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* EOR      (d) */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* EOR  (d,s),y */ }, // 52
    { "MVN", 02,  base, NULL, NULL,  READ_BMA,                     BlockMoveAddress /* MVN  src,dst */ }, { "EOR", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* EOR      d,x */ }, // 54
    { "LSR", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* LSR      d,x */ }, { "EOR", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* EOR    [d],y */ }, // 56
    { "CLI", 00,  base, NULL, NULL,      NULL,                              Implied /* CLI        i */ }, { "EOR", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* EOR      a,y */ }, // 58
    { "PHY", 00,  base, NULL, NULL,      NULL,                              Implied /* PHY        s */ }, { "TCD", 00,  base, NULL, NULL,      NULL,                              Implied /* TCD        i */ }, // 5A
    { "JMP", 03,  base, NULL,  JMP,   READ_24,                         AbsoluteLong /* JMP       al */ }, { "EOR", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* EOR      a,x */ }, // 5C
    { "LSR", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* LSR      a,x */ }, { "EOR", 03,  base, NULL, NULL,   READ_24,              AbsoluteLong | IndexedX /* EOR     al,x */ }, // 5E
    { "RTS", 00,  base, NULL, NULL,      NULL,                              Implied /* RTS        s */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* ADC   (dp,X) */ }, // 60
    { "PER", 02,  base, NULL, NULL,   READ_16,                       PCRelativeLong /* PER        s */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* ADC     sr,S */ }, // 62
    { "STZ", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* STZ        d */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* ADC       dp */ }, // 64
    { "ROR", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* ROR        d */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* ADC     [dp] */ }, // 66
    { "PLA", 00,  base, NULL, NULL,      NULL,                              Implied /* PLA        s */ }, { "ADC", 01, m_set, NULL, NULL, READ_8_16,                            Immediate /* ADC        # */ }, // 68
    { "ROR", 00,  base, NULL, NULL,      NULL,                              Implied /* ROR        A */ }, { "RTL", 00,  base, NULL, NULL,      NULL,                              Implied /* RTL        s */ }, // 6A
    { "JMP", 02,  base, NULL,  JMP,   READ_16,                  Absolute | Indirect /* JMP      (a) */ }, { "ADC", 02,  base, NULL, NULL,   READ_16,                             Absolute /* ADC     addr */ }, // 6C
    { "ROR", 02,  base, NULL, NULL,   READ_16,                             Absolute /* ROR        a */ }, { "ADC", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* ADC       al */ }, // 6E
    { "BVS", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BVS        r */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* ADC    (d),y */ }, // 70
    { "ADC", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* ADC      (d) */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* ADC (sr,S),y */ }, // 72
    { "STZ", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* STZ      d,x */ }, { "ADC", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* ADC      d,x */ }, // 74
    { "ROR", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* ROR      d,x */ }, { "ADC", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* ADC    [d],y */ }, // 76
    { "SEI", 00,  base, NULL, NULL,      NULL,                              Implied /* SEI        i */ }, { "ADC", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* ADC      a,y */ }, // 78
    { "PLY", 00,  base, NULL, NULL,      NULL,                              Implied /* PLY        s */ }, { "TDC", 00,  base, NULL, NULL,      NULL,                              Implied /* TDC        i */ }, // 7A
    { "JMP", 02,  base, NULL,  JMP,   READ_16,       Absolute | Indirect | IndexedX /* JMP    (a,x) */ }, { "ADC", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* ADC      a,x */ }, // 7C
    { "ROR", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* ROR      a,x */ }, { "ADC", 03,  base, NULL, NULL,   READ_16,              AbsoluteLong | IndexedX /* ADC     al,x */ }, // 7E
    { "BRA", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BRA        r */ }, { "STA", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* STA    (d,x) */ }, // 80
    { "BRL", 02,  base, NULL,  BRL,   READ_16,                       PCRelativeLong /* BRL       rl */ }, { "STA", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* STA      d,s */ }, // 82
    { "STY", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* STY        d */ }, { "STA", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* STA        d */ }, // 84
    { "STX", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* STX        d */ }, { "STA", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* STA      [d] */ }, // 86
    { "DEY", 00,  base, NULL, NULL,      NULL,                              Implied /* DEY        i */ }, { "BIT", 01, m_set, NULL, NULL, READ_8_16,                            Immediate /* BIT        # */ }, // 88
    { "TXA", 00,  base, NULL, NULL,      NULL,                              Implied /* TXA        i */ }, { "PHB", 00,  base, NULL, NULL,      NULL,                              Implied /* PHB        s */ }, // 8A
    { "STY", 02,  base, NULL, NULL,   READ_16,                             Absolute /* STY        a */ }, { "STA", 02,  base, NULL, NULL,   READ_16,                             Absolute /* STA        a */ }, // 8C
    { "STX", 02,  base, NULL, NULL,   READ_16,                             Absolute /* STX        a */ }, { "STA", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* STA       al */ }, // 8E
    { "BCC", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BCC        r */ }, { "STA", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* STA    (d),y */ }, // 90
    { "STA", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* STA      (d) */ }, { "STA", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* STA  (d,s),y */ }, // 92
    { "STY", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* STY      d,x */ }, { "STA", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* STA      d,x */ }, // 94
    { "STX", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedY /* STX      d,y */ }, { "STA", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* STA    [d],y */ }, // 96
    { "TYA", 00,  base, NULL, NULL,      NULL,                              Implied /* TYA        i */ }, { "STA", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* STA      a,y */ }, // 98
    { "TXS", 00,  base, NULL, NULL,      NULL,                              Implied /* TXS        i */ }, { "TXY", 00,  base, NULL, NULL,      NULL,                              Implied /* TXY        i */ }, // 9A
    { "STZ", 02,  base, NULL, NULL,   READ_16,                             Absolute /* STZ        a */ }, { "STA", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* STA      a,x */ }, // 9C
    { "STZ", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* STZ      a,x */ }, { "STA", 03,  base, NULL, NULL,   READ_24,              AbsoluteLong | IndexedX /* STA     al,x */ }, // 9E
    { "LDY", 01, x_set, NULL, NULL, READ_8_16,                            Immediate /* LDY        # */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* LDA    (d,x) */ }, // A0
    { "LDX", 01, x_set, NULL, NULL, READ_8_16,                            Immediate /* LDX        # */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* LDA      d,s */ }, // A2
    { "LDY", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* LDY        d */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* LDA        d */ }, // A4
    { "LDX", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* LDX        d */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* LDA      [d] */ }, // A6
    { "TAY", 00,  base, NULL, NULL,      NULL,                              Implied /* TAY        i */ }, { "LDA", 01, m_set, NULL, NULL, READ_8_16,                            Immediate /* LDA        # */ }, // A8
    { "TAX", 00,  base, NULL, NULL,      NULL,                              Implied /* TAX        i */ }, { "PLB", 00,  base, NULL, NULL,      NULL,                              Implied /* PLB        s */ }, // AA
    { "LDY", 02,  base, NULL, NULL,   READ_16,                             Absolute /* LDY        a */ }, { "LDA", 02,  base, NULL, NULL,   READ_16,                             Absolute /* LDA        a */ }, // AC
    { "LDX", 02,  base, NULL, NULL,   READ_16,                             Absolute /* LDX        a */ }, { "LDA", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* LDA       al */ }, // AE
    { "BCS", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BCS        r */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* LDA    (d),y */ }, // B0
    { "LDA", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* LDA      (d) */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* LDA  (d,s),y */ }, // B2
    { "LDY", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* LDY      d,x */ }, { "LDA", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* LDA      d,x */ }, // B4
    { "LDX", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedY /* LDX      d,y */ }, { "LDA", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* LDA    [d],y */ }, // B6
    { "CLV", 00,  base, NULL, NULL,      NULL,                              Implied /* CLV        i */ }, { "LDA", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* LDA      a,y */ }, // B8
    { "TSX", 00,  base, NULL, NULL,      NULL,                              Implied /* TSX        i */ }, { "TYX", 00,  base, NULL, NULL,      NULL,                              Implied /* TYX        i */ }, // BA
    { "LDY", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* LDY      a,x */ }, { "LDA", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* LDA      a,x */ }, // BC
    { "LDX", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* LDX      a,y */ }, { "LDA", 03,  base, NULL, NULL,   READ_24,              AbsoluteLong | IndexedX /* LDA     al,x */ }, // BE
    { "CPY", 01, x_set, NULL, NULL,    READ_8,                            Immediate /* CPY        # */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* CMP    (d,x) */ }, // C0
    { "REP", 01,  base,  REP, NULL,    READ_8,                            Immediate /* REP        # */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* CMP      d,s */ }, // C2
    { "CPY", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* CPY        d */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* CMP        d */ }, // C4
    { "DEC", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* DEC        d */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* CMP      [d] */ }, // C6
    { "INY", 00,  base, NULL, NULL,      NULL,                              Implied /* INY        i */ }, { "CMP", 01, m_set, NULL, NULL,    READ_8,                            Immediate /* CMP        # */ }, // C8
    { "DEX", 00,  base, NULL, NULL,      NULL,                              Implied /* DEX        i */ }, { "WAI", 00,  base, NULL, NULL,      NULL,                              Implied /* WAI        i */ }, // CA
    { "CPY", 02,  base, NULL, NULL,   READ_16,                             Absolute /* CPY        a */ }, { "CMP", 02,  base, NULL, NULL,   READ_16,                             Absolute /* CMP        a */ }, // CC
    { "DEC", 02,  base, NULL, NULL,   READ_16,                             Absolute /* DEC        a */ }, { "CMP", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* CMP       al */ }, // CE
    { "BNE", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BNE        r */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* CMP    (d),y */ }, // D0
    { "CMP", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* CMP      (d) */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* CMP  (d,s),y */ }, // D2
    { "PEI", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* PEI     (dp) */ }, { "CMP", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* CMP      d,x */ }, // D4
    { "DEC", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* DEC      d,x */ }, { "CMP", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* CMP    [d],y */ }, // D6
    { "CLD", 00,  base, NULL, NULL,      NULL,                              Implied /* CLD        i */ }, { "CMP", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* CMP      a,y */ }, // D8
    { "PHX", 00,  base, NULL, NULL,      NULL,                              Implied /* PHX        s */ }, { "STP", 00,  base, NULL, NULL,      NULL,                              Implied /* STP        i */ }, // DA
    { "JMP", 02,  base, NULL,  JMP,   READ_16,              Absolute | IndirectLong /* JMP      [a] */ }, { "CMP", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* CMP      a,x */ }, // DC
    { "DEC", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* DEC      a,x */ }, { "CMP", 03,  base, NULL, NULL,   READ_24,              AbsoluteLong | IndexedX /* CMP     al,x */ }, // DE
    { "CPX", 01, x_set, NULL, NULL,    READ_8,                            Immediate /* CPX        # */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedX /* SBC    (d,x) */ }, // E0
    { "SEP", 01,  base,  SEP, NULL,    READ_8,                            Immediate /* SEP        # */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,                        StackRelative /* SBC      d,s */ }, // E2
    { "CPX", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* CPX        d */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* SBC        d */ }, // E4
    { "INC", 01,  base, NULL, NULL,    READ_8,                           DirectPage /* INC        d */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,            DirectPage | IndirectLong /* SBC      [d] */ }, // E6
    { "INX", 00,  base, NULL, NULL,      NULL,                              Implied /* INX        i */ }, { "SBC", 01, m_set, NULL, NULL,    READ_8,                            Immediate /* SBC        # */ }, // E8
    { "NOP", 00,  base, NULL, NULL,      NULL,                              Implied /* NOP        i */ }, { "XBA", 00,  base, NULL, NULL,      NULL,                              Implied /* XBA        i */ }, // EA
    { "CPX", 02,  base, NULL, NULL,   READ_16,                             Absolute /* CPX        a */ }, { "SBC", 02,  base, NULL, NULL,   READ_16,                             Absolute /* SBC        a */ }, // EC
    { "INC", 02,  base, NULL, NULL,   READ_16,                             Absolute /* INC        a */ }, { "SBC", 03,  base, NULL, NULL,   READ_24,                         AbsoluteLong /* SBC       al */ }, // EE
    { "BEQ", 01,  base, NULL,  BRA,    READ_8,                           PCRelative /* BEQ        r */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,     DirectPage | Indirect | IndexedY /* SBC    (d),y */ }, // F0
    { "SBC", 01,  base, NULL, NULL,    READ_8,                DirectPage | Indirect /* SBC      (d) */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,  StackRelative | Indirect | IndexedY /* SBC  (d,s),y */ }, // F2
    { "PEA", 02,  base, NULL, NULL,   READ_16,                             Absolute /* PEA        s */ }, { "SBC", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* SBC      d,x */ }, // F4
    { "INC", 01,  base, NULL, NULL,    READ_8,                DirectPage | IndexedX /* INC      d,x */ }, { "SBC", 01,  base, NULL, NULL,    READ_8, DirectPage | IndirectLong | IndexedY /* SBC    [d],y */ }, // F6
    { "SED", 00,  base, NULL, NULL,      NULL,                              Implied /* SED        i */ }, { "SBC", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedY /* SBC      a,y */ }, // F8
    { "PLX", 00,  base, NULL, NULL,      NULL,                              Implied /* PLX        s */ }, { "XCE", 00,  base,  XCE, NULL,      NULL,                              Implied /* XCE        i */ }, // FA
    { "JSR", 02,  base, NULL, NULL,   READ_16,       Absolute | Indirect | IndexedX /* JSR    (a,x) */ }, { "SBC", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* SBC      a,x */ }, // FC
    { "INC", 02,  base, NULL, NULL,   READ_16,                  Absolute | IndexedX /* INC      a,x */ }, { "SBC", 03,  base, NULL, NULL,   READ_16,              AbsoluteLong | IndexedX /* SBC     al,x */ }  // FE
};

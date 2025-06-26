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

void JMP(uint32_t offset, ...) {
    // TODO: stub!
    va_list args;
    va_start(args, offset);
    uint16_t addr = va_arg(args, int);
    if (isESet()) {
        // 6502 emulation mode
        addr &= 0xFFFF; // mask to 16 bits
    }
    // make a label
    va_end(args);
}

void BRL(uint32_t offset, ...) {
    // TODO: stub!
    va_list args;
    va_start(args, offset);
    uint16_t addr = va_arg(args, int);
    if (isESet()) {
        // 6502 emulation mode
        addr &= 0xFFFF; // mask to 16 bits
    }
    // make a label
    va_end(args);
}

void BRA(uint32_t offset, ...) {
    // TODO: stub!
    va_list args;
    va_start(args, offset);
    uint16_t addr = va_arg(args, int);
    if (isESet()) {
        // 6502 emulation mode
        addr &= 0xFFFF; // mask to 16 bits
    }
    // make a label
    va_end(args);
}

const opcode_t opcodes[256] = {
    { "BRK", 0,  base, NULL, NULL, Implied                        /* BRK s       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* ORA (d,x)    */ }, // 00
    { "COP", 0,  base, NULL, NULL, Immediate                      /* COP #       */ }, { "ORA", 1,  base, NULL, NULL, StackRelative                       /* ORA d,s      */ }, // 02
    { "TSB", 1,  base, NULL, NULL, DirectPage                     /* TSB d       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage                          /* ORA d        */ }, // 04
    { "ASL", 1,  base, NULL, NULL, DirectPage                     /* ASL d       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* ORA [d]      */ }, // 06
    { "PHP", 0,  base, NULL, NULL, Implied                        /* PHP s       */ }, { "ORA", 1, m_set, NULL, NULL, Immediate                           /* ORA #        */ }, // 08
    { "ASL", 0,  base, NULL, NULL, Implied                        /* ASL A       */ }, { "PHD", 0,  base, NULL, NULL, Implied                             /* PHD s        */ }, // 0A
    { "TSB", 2,  base, NULL, NULL, Absolute                       /* TSB a       */ }, { "ORA", 2,  base, NULL, NULL, Absolute                            /* ORA a        */ }, // 0C
    { "ASL", 2,  base, NULL, NULL, Absolute                       /* ASL a       */ }, { "ORA", 3,  base, NULL, NULL, AbsoluteLong                        /* ORA al       */ }, // 0E
    { "BPL", 1,  base, NULL, NULL, PCRelative                     /* BPL r       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* ORA (d),y    */ }, // 10
    { "ORA", 1,  base, NULL, NULL, DirectPage | Indirect          /* ORA (d)     */ }, { "ORA", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* ORA (d,s),y  */ }, // 12
    { "TRB", 1,  base, NULL, NULL, DirectPage                     /* TRB d       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | IndexedX               /* ORA d,x      */ }, // 14
    { "ASL", 1,  base, NULL, NULL, DirectPage | IndexedX          /* ASL d,x     */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* ORA [d],y    */ }, // 16
    { "CLC", 0,  base,  CLC, NULL, Implied                        /* CLC i       */ }, { "ORA", 2,  base, NULL, NULL, Absolute | IndexedY                 /* ORA a,y      */ }, // 18
    { "INC", 0,  base, NULL, NULL, Implied                        /* INC A       */ }, { "TCS", 0,  base, NULL, NULL, Implied                             /* TCS i        */ }, // 1A
    { "TRB", 2,  base, NULL, NULL, Absolute                       /* TRB a       */ }, { "ORA", 2,  base, NULL, NULL, Absolute | IndexedX                 /* ORA a,x      */ }, // 1C
    { "ASL", 2,  base, NULL, NULL, Absolute | IndexedX            /* ASL a,x     */ }, { "ORA", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* ORA al,x     */ }, // 1E
    { "JSR", 2,  base, NULL,  JMP, Absolute                       /* JSR a       */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedX               /* AND (d,x)    */ }, // 20
    { "JSL", 3,  base, NULL,  JMP, AbsoluteLong                   /* JSL al      */ }, { "AND", 1,  base, NULL, NULL, StackRelative                       /* AND d,s      */ }, // 22
    { "BIT", 1,  base, NULL, NULL, DirectPage                     /* BIT d       */ }, { "AND", 1,  base, NULL, NULL, DirectPage                          /* AND d        */ }, // 24 
    { "ROL", 1,  base, NULL, NULL, DirectPage                     /* ROL d       */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* AND [d]      */ }, // 26
    { "PLP", 0,  base, NULL, NULL, Implied                        /* PLP s       */ }, { "AND", 1, m_set, NULL, NULL, Immediate                           /* AND #        */ }, // 28
    { "ROL", 0,  base, NULL, NULL, Implied                        /* ROL A       */ }, { "PLD", 0,  base, NULL, NULL, Implied                             /* PLD s        */ }, // 2A
    { "BIT", 2,  base, NULL, NULL, Absolute                       /* BIT a       */ }, { "AND", 2,  base, NULL, NULL, Absolute                            /* AND a        */ }, // 2C
    { "ROL", 2,  base, NULL, NULL, Absolute                       /* ROL a       */ }, { "AND", 3,  base, NULL, NULL, AbsoluteLong                        /* AND al       */ }, // 2E
    { "BMI", 1,  base, NULL, NULL, PCRelative                     /* BMI R       */ }, { "AND", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* AND (d),y    */ }, // 30
    { "AND", 1,  base, NULL, NULL, DirectPage | Indirect          /* AND (d)     */ }, { "AND", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* AND (d,s),y  */ }, // 32
    { "BIT", 1,  base, NULL, NULL, DirectPage | IndexedX          /* BIT d,x     */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedX               /* AND d,x      */ }, // 34
    { "ROL", 1,  base, NULL, NULL, DirectPage | IndexedX          /* ROL d,x     */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* AND [d],y    */ }, // 36
    { "SEC", 0,  base,  SEC, NULL, Implied                        /* SEC i       */ }, { "AND", 2,  base, NULL, NULL, Absolute | IndexedY                 /* AND a,y      */ }, // 38
    { "DEC", 0,  base, NULL, NULL, Implied                        /* DEC A       */ }, { "TSC", 0,  base, NULL, NULL, Implied                             /* TSC i        */ }, // 3A
    { "BIT", 2,  base, NULL, NULL, Absolute | IndexedX            /* BIT a,x     */ }, { "AND", 2,  base, NULL, NULL, Absolute | IndexedX                 /* AND a,x      */ }, // 3C
    { "ROL", 2,  base, NULL, NULL, Absolute | IndexedX            /* ROL a,x     */ }, { "AND", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* AND al,x     */ }, // 3E
    { "RTI", 0,  base, NULL, NULL, Implied                        /* RTI s       */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* EOR (d,x)    */ }, // 40
    { "WDM", 1,  base, NULL, NULL, Implied                        /* WDM i       */ }, { "EOR", 1,  base, NULL, NULL, StackRelative                       /* EOR d,s      */ }, // 42
    { "MVP", 2,  base, NULL, NULL, BlockMoveAddress               /* MVP src,dst */ }, { "EOR", 1,  base, NULL, NULL, DirectPage                          /* EOR d        */ }, // 44
    { "LSR", 1,  base, NULL, NULL, DirectPage                     /* LSR d       */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* EOR [d]      */ }, // 46
    { "PHA", 0,  base, NULL, NULL, Implied                        /* PHA s       */ }, { "EOR", 1, m_set, NULL, NULL, Immediate                           /* EOR #        */ }, // 48
    { "LSR", 0,  base, NULL, NULL, Implied                        /* LSR A       */ }, { "PHK", 0,  base, NULL, NULL, Implied                             /* PHK s        */ }, // 4A
    { "JMP", 2,  base, NULL,  JMP, Absolute                       /* JMP a       */ }, { "EOR", 2,  base, NULL, NULL, Absolute                            /* EOR a        */ }, // 4C
    { "LSR", 2,  base, NULL, NULL, Absolute                       /* LSR a       */ }, { "EOR", 3,  base, NULL, NULL, AbsoluteLong                        /* EOR al       */ }, // 4E
    { "BVC", 1,  base, NULL, NULL, PCRelative                     /* BVC r       */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* EOR (d),y    */ }, // 50
    { "EOR", 1,  base, NULL, NULL, DirectPage | Indirect          /* EOR (d)     */ }, { "EOR", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* EOR (d,s),y  */ }, // 52
    { "MVN", 2,  base, NULL, NULL, BlockMoveAddress               /* MVN src,dst */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | IndexedX               /* EOR d,x      */ }, // 54
    { "LSR", 1,  base, NULL, NULL, DirectPage | IndexedX          /* LSR d,x     */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* EOR [d],y    */ }, // 56
    { "CLI", 0,  base, NULL, NULL, Implied                        /* CLI i       */ }, { "EOR", 2,  base, NULL, NULL, Absolute | IndexedY                 /* EOR a,y      */ }, // 58
    { "PHY", 0,  base, NULL, NULL, Implied                        /* PHY s       */ }, { "TCD", 0,  base, NULL, NULL, Implied                             /* TCD i        */ }, // 5A
    { "JMP", 3,  base, NULL,  JMP, AbsoluteLong                   /* JMP al      */ }, { "EOR", 2,  base, NULL, NULL, Absolute | IndexedX                 /* EOR a,x      */ }, // 5C
    { "LSR", 2,  base, NULL, NULL, Absolute | IndexedX            /* LSR a,x     */ }, { "EOR", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* EOR al,x     */ }, // 5E
    { "RTS", 0,  base, NULL, NULL, Implied                        /* RTS s       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* ADC (d,X)    */ }, // 60
    { "PER", 2,  base, NULL, NULL, PCRelativeLong                 /* PER s       */ }, { "ADC", 1,  base, NULL, NULL, StackRelative                       /* ADC d,S      */ }, // 62
    { "STZ", 1,  base, NULL, NULL, DirectPage                     /* STZ d       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage                          /* ADC d        */ }, // 64
    { "ROR", 1,  base, NULL, NULL, DirectPage                     /* ROR d       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* ADC [d]      */ }, // 66
    { "PLA", 0,  base, NULL, NULL, Implied                        /* PLA s       */ }, { "ADC", 1, m_set, NULL, NULL, Immediate                           /* ADC #        */ }, // 68
    { "ROR", 0,  base, NULL, NULL, Implied                        /* ROR A       */ }, { "RTL", 0,  base, NULL, NULL, Implied                             /* RTL s        */ }, // 6A
    { "JMP", 2,  base, NULL,  JMP, Absolute | Indirect            /* JMP (a)     */ }, { "ADC", 2,  base, NULL, NULL, Absolute                            /* ADC addr     */ }, // 6C
    { "ROR", 2,  base, NULL, NULL, Absolute                       /* ROR a       */ }, { "ADC", 3,  base, NULL, NULL, AbsoluteLong                        /* ADC al       */ }, // 6E
    { "BVS", 1,  base, NULL,  BRA, PCRelative                     /* BVS r       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* ADC (d),y    */ }, // 70
    { "ADC", 1,  base, NULL, NULL, DirectPage | Indirect          /* ADC (d)     */ }, { "ADC", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* ADC (d,S),y  */ }, // 72
    { "STZ", 1,  base, NULL, NULL, DirectPage | IndexedX          /* STZ d,x     */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | IndexedX               /* ADC d,x      */ }, // 74
    { "ROR", 1,  base, NULL, NULL, DirectPage | IndexedX          /* ROR d,x     */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* ADC [d],y    */ }, // 76
    { "SEI", 0,  base, NULL, NULL, Implied                        /* SEI i       */ }, { "ADC", 2,  base, NULL, NULL, Absolute | IndexedY                 /* ADC a,y      */ }, // 78
    { "PLY", 0,  base, NULL, NULL, Implied                        /* PLY s       */ }, { "TDC", 0,  base, NULL, NULL, Implied                             /* TDC i        */ }, // 7A
    { "JMP", 2,  base, NULL,  JMP, Absolute | Indirect | IndexedX /* JMP (a,x)   */ }, { "ADC", 2,  base, NULL, NULL, Absolute | IndexedX                 /* ADC a,x      */ }, // 7C
    { "ROR", 2,  base, NULL, NULL, Absolute | IndexedX            /* ROR a,x     */ }, { "ADC", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* ADC al,x     */ }, // 7E
    { "BRA", 1,  base, NULL, NULL, PCRelative                     /* BRA r       */ }, { "STA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* STA (d,x)    */ }, // 80
    { "BRL", 2,  base, NULL,  BRL, PCRelativeLong                 /* BRL rl      */ }, { "STA", 1,  base, NULL, NULL, StackRelative                       /* STA d,s      */ }, // 82
    { "STY", 1,  base, NULL, NULL, DirectPage                     /* STY d       */ }, { "STA", 1,  base, NULL, NULL, DirectPage                          /* STA d        */ }, // 84
    { "STX", 1,  base, NULL, NULL, DirectPage                     /* STX d       */ }, { "STA", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* STA [d]      */ }, // 86 
    { "DEY", 0,  base, NULL, NULL, Implied                        /* DEY i       */ }, { "BIT", 1, m_set, NULL, NULL, Immediate                           /* BIT #        */ }, // 88
    { "TXA", 0,  base, NULL, NULL, Implied                        /* TXA i       */ }, { "PHB", 0,  base, NULL, NULL, Implied                             /* PHB s        */ }, // 8A
    { "STY", 2,  base, NULL, NULL, Absolute                       /* STY a       */ }, { "STA", 2,  base, NULL, NULL, Absolute                            /* STA a        */ }, // 8C
    { "STX", 2,  base, NULL, NULL, Absolute                       /* STX a       */ }, { "STA", 3,  base, NULL, NULL, AbsoluteLong                        /* STA al       */ }, // 8E
    { "BCC", 1,  base, NULL,  BRA, PCRelative                     /* BCC r       */ }, { "STA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* STA (d),y    */ }, // 90
    { "STA", 1,  base, NULL, NULL, DirectPage | Indirect          /* STA (d)     */ }, { "STA", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* STA (d,s),y  */ }, // 92
    { "STY", 1,  base, NULL, NULL, DirectPage | IndexedX          /* STY d,x     */ }, { "STA", 1,  base, NULL, NULL, DirectPage | IndexedX               /* STA d,x      */ }, // 94
    { "STX", 1,  base, NULL, NULL, DirectPage | IndexedY          /* STX d,y     */ }, { "STA", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* STA [d],y    */ }, // 96
    { "TYA", 0,  base, NULL, NULL, Implied                        /* TYA i       */ }, { "STA", 2,  base, NULL, NULL, Absolute | IndexedY                 /* STA a,y      */ }, // 98
    { "TXS", 0,  base, NULL, NULL, Implied                        /* TXS i       */ }, { "TXY", 0,  base, NULL, NULL, Implied                             /* TXY i        */ }, // 9A
    { "STZ", 2,  base, NULL, NULL, Absolute                       /* STZ a       */ }, { "STA", 2,  base, NULL, NULL, Absolute | IndexedX                 /* STA a,x      */ }, // 9C
    { "STZ", 2,  base, NULL, NULL, Absolute | IndexedX            /* STZ a,x     */ }, { "STA", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* STA al,x     */ }, // 9E
    { "LDY", 1, x_set, NULL, NULL, Immediate                      /* LDY #       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* LDA (d,x)    */ }, // A0
    { "LDX", 1, x_set, NULL, NULL, Immediate                      /* LDX #       */ }, { "LDA", 1,  base, NULL, NULL, StackRelative                       /* LDA d,s      */ }, // A2
    { "LDY", 1,  base, NULL, NULL, DirectPage                     /* LDY d       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage                          /* LDA d        */ }, // A4
    { "LDX", 1,  base, NULL, NULL, DirectPage                     /* LDX d       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* LDA [d]      */ }, // A6
    { "TAY", 0,  base, NULL, NULL, Implied                        /* TAY i       */ }, { "LDA", 1, m_set, NULL, NULL, Immediate                           /* LDA #        */ }, // A8
    { "TAX", 0,  base, NULL, NULL, Implied                        /* TAX i       */ }, { "PLB", 0,  base, NULL, NULL, Implied                             /* PLB s        */ }, // AA
    { "LDY", 2,  base, NULL, NULL, Absolute                       /* LDY a       */ }, { "LDA", 2,  base, NULL, NULL, Absolute                            /* LDA a        */ }, // AC
    { "LDX", 2,  base, NULL, NULL, Absolute                       /* LDX a       */ }, { "LDA", 3,  base, NULL, NULL, AbsoluteLong                        /* LDA al       */ }, // AE
    { "BCS", 1,  base, NULL,  BRA, PCRelative                     /* BCS r       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* LDA (d),y    */ }, // B0
    { "LDA", 1,  base, NULL, NULL, DirectPage | Indirect          /* LDA (d)     */ }, { "LDA", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* LDA (d,s),y  */ }, // B2
    { "LDY", 1,  base, NULL, NULL, DirectPage | IndexedX          /* LDY d,x     */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | IndexedX               /* LDA d,x      */ }, // B4
    { "LDX", 1,  base, NULL, NULL, DirectPage | IndexedY          /* LDX d,y     */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* LDA [d],y    */ }, // B6
    { "CLV", 0,  base, NULL, NULL, Implied                        /* CLV i       */ }, { "LDA", 2,  base, NULL, NULL, Absolute | IndexedY                 /* LDA a,y      */ }, // B8
    { "TSX", 0,  base, NULL, NULL, Implied                        /* TSX i       */ }, { "TYX", 0,  base, NULL, NULL, Implied                             /* TYX i        */ }, // BA
    { "LDY", 2,  base, NULL, NULL, Absolute | IndexedX            /* LDY a,x     */ }, { "LDA", 2,  base, NULL, NULL, Absolute | IndexedX                 /* LDA a,x      */ }, // BC
    { "LDX", 2,  base, NULL, NULL, Absolute | IndexedY            /* LDX a,y     */ }, { "LDA", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* LDA al,x     */ }, // BE
    { "CPY", 1, x_set, NULL, NULL, Immediate                      /* CPY #       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* CMP (d,x)    */ }, // C0
    { "REP", 1,  base,  REP, NULL, Immediate                      /* REP #       */ }, { "CMP", 1,  base, NULL, NULL, StackRelative                       /* CMP d,s      */ }, // C2
    { "CPY", 1,  base, NULL, NULL, DirectPage                     /* CPY d       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage                          /* CMP d        */ }, // C4
    { "DEC", 1,  base, NULL, NULL, DirectPage                     /* DEC d       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* CMP [d]      */ }, // C6
    { "INY", 0,  base, NULL, NULL, Implied                        /* INY i       */ }, { "CMP", 1, m_set, NULL, NULL, Immediate                           /* CMP #        */ }, // C8
    { "DEX", 0,  base, NULL, NULL, Implied                        /* DEX i       */ }, { "WAI", 0,  base, NULL, NULL, Implied                             /* WAI i        */ }, // CA
    { "CPY", 2,  base, NULL, NULL, Absolute                       /* CPY a       */ }, { "CMP", 2,  base, NULL, NULL, Absolute                            /* CMP a        */ }, // CC
    { "DEC", 2,  base, NULL, NULL, Absolute                       /* DEC a       */ }, { "CMP", 3,  base, NULL, NULL, AbsoluteLong                        /* CMP al       */ }, // CE
    { "BNE", 1,  base, NULL,  BRA, PCRelative                     /* BNE r       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* CMP (d),y    */ }, // D0
    { "CMP", 1,  base, NULL, NULL, DirectPage | Indirect          /* CMP (d)     */ }, { "CMP", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* CMP (d,s),y  */ }, // D2
    { "PEI", 1,  base, NULL, NULL, DirectPage | Indirect          /* PEI (d)     */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | IndexedX               /* CMP d,x      */ }, // D4
    { "DEC", 1,  base, NULL, NULL, DirectPage | IndexedX          /* DEC d,x     */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* CMP [d],y    */ }, // D6
    { "CLD", 0,  base, NULL, NULL, Implied                        /* CLD i       */ }, { "CMP", 2,  base, NULL, NULL, Absolute | IndexedY                 /* CMP a,y      */ }, // D8 
    { "PHX", 0,  base, NULL, NULL, Implied                        /* PHX s       */ }, { "STP", 0,  base, NULL, NULL, Implied                             /* STP i        */ }, // DA
    { "JMP", 2,  base, NULL,  JMP, Absolute | IndexedLong         /* JMP [a]     */ }, { "CMP", 2,  base, NULL, NULL, Absolute | IndexedX                 /* CMP a,x      */ }, // DC
    { "DEC", 2,  base, NULL, NULL, Absolute | IndexedX            /* DEC a,x     */ }, { "CMP", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* CMP al,x     */ }, // DE
    { "CPX", 1, x_set, NULL, NULL, Immediate                      /* CPX #       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX    /* SBC (d,x)    */ }, // E0
    { "SEP", 1,  base,  SEP, NULL, Immediate                      /* SEP #       */ }, { "SBC", 1,  base, NULL, NULL, StackRelative                       /* SBC d,s      */ }, // E2
    { "CPX", 1,  base, NULL, NULL, DirectPage                     /* CPX d       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage                          /* SBC d        */ }, // E4
    { "INC", 1,  base, NULL, NULL, DirectPage                     /* INC d       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | IndexedLong            /* SBC [d]      */ }, // E6
    { "INX", 0,  base, NULL, NULL, Implied                        /* INX i       */ }, { "SBC", 1, m_set, NULL, NULL, Immediate                           /* SBC #        */ }, // E8
    { "NOP", 0,  base, NULL, NULL, Implied                        /* NOP i       */ }, { "XBA", 0,  base, NULL, NULL, Implied                             /* XBA i        */ }, // EA
    { "CPX", 2,  base, NULL, NULL, Absolute                       /* CPX a       */ }, { "SBC", 2,  base, NULL, NULL, Absolute                            /* SBC a        */ }, // EC
    { "INC", 2,  base, NULL, NULL, Absolute                       /* INC a       */ }, { "SBC", 3,  base, NULL, NULL, AbsoluteLong                        /* SBC al       */ }, // EE
    { "BEQ", 1,  base, NULL,  BRA, PCRelative                     /* BEQ r       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY    /* SBC (d),y    */ }, // F0
    { "SBC", 1,  base, NULL, NULL, DirectPage | Indirect          /* SBC (d)     */ }, { "SBC", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY  /* SBC (d,s),y  */ }, // F2
    { "PEA", 2,  base, NULL, NULL, Absolute                       /* PEA s       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | IndexedX               /* SBC d,x      */ }, // F4
    { "INC", 1,  base, NULL, NULL, DirectPage | IndexedX          /* INC d,x     */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY /* SBC [d],y    */ }, // F6
    { "SED", 0,  base, NULL, NULL, Implied                        /* SED i       */ }, { "SBC", 2,  base, NULL, NULL, Absolute | IndexedY                 /* SBC a,y      */ }, // F8
    { "PLX", 0,  base, NULL, NULL, Implied                        /* PLX s       */ }, { "XCE", 0,  base,  XCE, NULL, Implied                             /* XCE i        */ }, // FA
    { "JSR", 2,  base, NULL,  JMP, Absolute | Indirect | IndexedX /* JSR (a,x)   */ }, { "SBC", 2,  base, NULL, NULL, Absolute | IndexedX                 /* SBC a,x      */ }, // FC
    { "INC", 2,  base, NULL, NULL, Absolute | IndexedX            /* INC a,x     */ }, { "SBC", 3,  base, NULL, NULL, AbsoluteLong | IndexedX             /* SBC al,x     */ }  // FE
};


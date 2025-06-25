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
    { "BRK", 0,  base, NULL, NULL, Implied                        /* BRK s       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* ORA (d,x)  */ }, { "COP", 0,  base, NULL, NULL, Immediate              /* COP #   */ }, { "ORA", 1,  base, NULL, NULL, StackRelative                        /* ORA d,s      */ }, // 00
    { "TSB", 1,  base, NULL, NULL, DirectPage                     /* TSB d       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage                       /* ORA d      */ }, { "ASL", 1,  base, NULL, NULL, DirectPage             /* ASL d   */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* ORA [d]      */ }, // 04
    { "PHP", 0,  base, NULL, NULL, Implied                        /* PHP s       */ }, { "ORA", 1, m_set, NULL, NULL, Immediate                        /* ORA #      */ }, { "ASL", 0,  base, NULL, NULL, Implied                /* ASL A   */ }, { "PHD", 0,  base, NULL, NULL, Implied                              /* PHD s        */ }, // 08
    { "TSB", 2,  base, NULL, NULL, Absolute                       /* TSB a       */ }, { "ORA", 2,  base, NULL, NULL, Absolute                         /* ORA a      */ }, { "ASL", 2,  base, NULL, NULL, Absolute               /* ASL a   */ }, { "ORA", 3,  base, NULL, NULL, AbsoluteLong                         /* ORA al       */ }, // 0C
    { "BPL", 1,  base, NULL, NULL, PCRelative                     /* BPL r       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* ORA (d),y  */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | Indirect  /* ORA (d) */ }, { "ORA", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* ORA (d,s),y  */ }, // 10
    { "TRB", 1,  base, NULL, NULL, DirectPage                     /* TRB d       */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | IndexedX            /* ORA d,x    */ }, { "ASL", 1,  base, NULL, NULL, DirectPage | IndexedX  /* ASL d,x */ }, { "ORA", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* ORA [d],y    */ }, // 14
    { "CLC", 0,  base,  CLC, NULL, Implied                        /* CLC i       */ }, { "ORA", 2,  base, NULL, NULL, Absolute | IndexedY              /* ORA a,y    */ }, { "INC", 0,  base, NULL, NULL, Implied                /* INC A   */ }, { "TCS", 0,  base, NULL, NULL, Implied                              /* TCS i        */ }, // 18
    { "TRB", 2,  base, NULL, NULL, Absolute                       /* TRB a       */ }, { "ORA", 2,  base, NULL, NULL, Absolute | IndexedX              /* ORA a,x    */ }, { "ASL", 2,  base, NULL, NULL, Absolute | IndexedX    /* ASL a,x */ }, { "ORA", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* ORA al,x     */ }, // 1C
    { "JSR", 2,  base, NULL,  JMP, Absolute                       /* JSR a       */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedX            /* AND (d,x)  */ }, { "JSL", 3,  base, NULL,  JMP, AbsoluteLong           /* JSL al  */ }, { "AND", 1,  base, NULL, NULL, StackRelative                        /* AND d,s      */ }, // 20
    { "BIT", 1,  base, NULL, NULL, DirectPage                     /* BIT d       */ }, { "AND", 1,  base, NULL, NULL, DirectPage                       /* AND d      */ }, { "ROL", 1,  base, NULL, NULL, DirectPage             /* ROL d   */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* AND [d]      */ }, // 24
    { "PLP", 0,  base, NULL, NULL, Implied                        /* PLP s       */ }, { "AND", 1, m_set, NULL, NULL, Immediate                        /* AND #      */ }, { "ROL", 0,  base, NULL, NULL, Implied                /* ROL A   */ }, { "PLD", 0,  base, NULL, NULL, Implied                              /* PLD s        */ }, // 28
    { "BIT", 2,  base, NULL, NULL, Absolute                       /* BIT a       */ }, { "AND", 2,  base, NULL, NULL, Absolute                         /* AND a      */ }, { "ROL", 2,  base, NULL, NULL, Absolute               /* ROL a   */ }, { "AND", 3,  base, NULL, NULL, AbsoluteLong                         /* AND al       */ }, // 2C
    { "BMI", 1,  base, NULL, NULL, PCRelative                     /* BMI R       */ }, { "AND", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* AND (d),y  */ }, { "AND", 1,  base, NULL, NULL, DirectPage | Indirect  /* AND (d) */ }, { "AND", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* AND (d,s),y  */ }, // 30
    { "BIT", 1,  base, NULL, NULL, DirectPage | IndexedX          /* BIT d,x     */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedX            /* AND d,x    */ }, { "ROL", 1,  base, NULL, NULL, DirectPage | IndexedX  /* ROL d,x */ }, { "AND", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* AND [d],y    */ }, // 34
    { "SEC", 0,  base,  SEC, NULL, Implied                        /* SEC i       */ }, { "AND", 2,  base, NULL, NULL, Absolute | IndexedY              /* AND a,y    */ }, { "DEC", 0,  base, NULL, NULL, Implied                /* DEC A   */ }, { "TSC", 0,  base, NULL, NULL, Implied                              /* TSC i        */ }, // 38
    { "BIT", 2,  base, NULL, NULL, Absolute | IndexedX            /* BIT a,x     */ }, { "AND", 2,  base, NULL, NULL, Absolute | IndexedX              /* AND a,x    */ }, { "ROL", 2,  base, NULL, NULL, Absolute | IndexedX    /* ROL a,x */ }, { "AND", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* AND al,x     */ }, // 3C
    { "RTI", 0,  base, NULL, NULL, Implied                        /* RTI s       */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* EOR (d,x)  */ }, { "WDM", 1,  base, NULL, NULL, Implied                /* WDM i   */ }, { "EOR", 1,  base, NULL, NULL, StackRelative                        /* EOR d,s      */ }, // 40
    { "MVP", 2,  base, NULL, NULL, BlockMoveAddress               /* MVP src,dst */ }, { "EOR", 1,  base, NULL, NULL, DirectPage                       /* EOR d      */ }, { "LSR", 1,  base, NULL, NULL, DirectPage             /* LSR d   */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* EOR [d]      */ }, // 44
    { "PHA", 0,  base, NULL, NULL, Implied                        /* PHA s       */ }, { "EOR", 1, m_set, NULL, NULL, Immediate                        /* EOR #      */ }, { "LSR", 0,  base, NULL, NULL, Implied                /* LSR A   */ }, { "PHK", 0,  base, NULL, NULL, Implied                              /* PHK s        */ }, // 48
    { "JMP", 2,  base, NULL,  JMP, Absolute                       /* JMP a       */ }, { "EOR", 2,  base, NULL, NULL, Absolute                         /* EOR a      */ }, { "LSR", 2,  base, NULL, NULL, Absolute               /* LSR a   */ }, { "EOR", 3,  base, NULL, NULL, AbsoluteLong                         /* EOR al       */ }, // 4C
    { "BVC", 1,  base, NULL, NULL, PCRelative                     /* BVC r       */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* EOR (d),y  */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | Indirect  /* EOR (d) */ }, { "EOR", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* EOR (d,s),y  */ }, // 50
    { "MVN", 2,  base, NULL, NULL, BlockMoveAddress               /* MVN src,dst */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | IndexedX            /* EOR d,x    */ }, { "LSR", 1,  base, NULL, NULL, DirectPage | IndexedX  /* LSR d,x */ }, { "EOR", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* EOR [d],y    */ }, // 54
    { "CLI", 0,  base, NULL, NULL, Implied                        /* CLI i       */ }, { "EOR", 2,  base, NULL, NULL, Absolute | IndexedY              /* EOR a,y    */ }, { "PHY", 0,  base, NULL, NULL, Implied                /* PHY s   */ }, { "TCD", 0,  base, NULL, NULL, Implied                              /* TCD i        */ }, // 58
    { "JMP", 3,  base, NULL,  JMP, AbsoluteLong                   /* JMP al      */ }, { "EOR", 2,  base, NULL, NULL, Absolute | IndexedX              /* EOR a,x    */ }, { "LSR", 2,  base, NULL, NULL, Absolute | IndexedX    /* LSR a,x */ }, { "EOR", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* EOR al,x     */ }, // 5C
    { "RTS", 0,  base, NULL, NULL, Implied                        /* RTS s       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* ADC (d,X)  */ }, { "PER", 2,  base, NULL, NULL, PCRelativeLong         /* PER s   */ }, { "ADC", 1,  base, NULL, NULL, StackRelative                        /* ADC d,S      */ }, // 60
    { "STZ", 1,  base, NULL, NULL, DirectPage                     /* STZ d       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage                       /* ADC d      */ }, { "ROR", 1,  base, NULL, NULL, DirectPage             /* ROR d   */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* ADC [d]      */ }, // 64
    { "PLA", 0,  base, NULL, NULL, Implied                        /* PLA s       */ }, { "ADC", 1, m_set, NULL, NULL, Immediate                        /* ADC #      */ }, { "ROR", 0,  base, NULL, NULL, Implied                /* ROR A   */ }, { "RTL", 0,  base, NULL, NULL, Implied                              /* RTL s        */ }, // 68
    { "JMP", 2,  base, NULL,  JMP, Absolute | Indirect            /* JMP (a)     */ }, { "ADC", 2,  base, NULL, NULL, Absolute                         /* ADC addr   */ }, { "ROR", 2,  base, NULL, NULL, Absolute               /* ROR a   */ }, { "ADC", 3,  base, NULL, NULL, AbsoluteLong                         /* ADC al       */ }, // 6C
    { "BVS", 1,  base, NULL,  BRA, PCRelative                     /* BVS r       */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* ADC (d),y  */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | Indirect  /* ADC (d) */ }, { "ADC", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* ADC (d,S),y  */ }, // 70
    { "STZ", 1,  base, NULL, NULL, DirectPage | IndexedX          /* STZ d,x     */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | IndexedX            /* ADC d,x    */ }, { "ROR", 1,  base, NULL, NULL, DirectPage | IndexedX  /* ROR d,x */ }, { "ADC", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* ADC [d],y    */ }, // 74
    { "SEI", 0,  base, NULL, NULL, Implied                        /* SEI i       */ }, { "ADC", 2,  base, NULL, NULL, Absolute | IndexedY              /* ADC a,y    */ }, { "PLY", 0,  base, NULL, NULL, Implied                /* PLY s   */ }, { "TDC", 0,  base, NULL, NULL, Implied                              /* TDC i        */ }, // 78
    { "JMP", 2,  base, NULL,  JMP, Absolute | Indirect | IndexedX /* JMP (a,x)   */ }, { "ADC", 2,  base, NULL, NULL, Absolute | IndexedX              /* ADC a,x    */ }, { "ROR", 2,  base, NULL, NULL, Absolute | IndexedX    /* ROR a,x */ }, { "ADC", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* ADC al,x     */ }, // 7C
    { "BRA", 1,  base, NULL, NULL, PCRelative                     /* BRA r       */ }, { "STA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* STA (d,x)  */ }, { "BRL", 2,  base, NULL, NULL, PCRelativeLong         /* BRL rl  */ }, { "STA", 1,  base, NULL, NULL, StackRelative                        /* STA d,s      */ }, // 80
    { "STY", 1,  base, NULL, NULL, DirectPage                     /* STY d       */ }, { "STA", 1,  base, NULL, NULL, DirectPage                       /* STA d      */ }, { "STX", 1,  base, NULL, NULL, DirectPage             /* STX d   */ }, { "STA", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* STA [d]      */ }, // 84
    { "DEY", 0,  base, NULL, NULL, Implied                        /* DEY i       */ }, { "BIT", 1, m_set, NULL, NULL, Immediate                        /* BIT #      */ }, { "TXA", 0,  base, NULL, NULL, Implied                /* TXA i   */ }, { "PHB", 0,  base, NULL, NULL, Implied                              /* PHB s        */ }, // 88
    { "STY", 2,  base, NULL, NULL, Absolute                       /* STY a       */ }, { "STA", 2,  base, NULL, NULL, Absolute                         /* STA a      */ }, { "STX", 2,  base, NULL, NULL, Absolute               /* STX a   */ }, { "STA", 3,  base, NULL, NULL, AbsoluteLong                         /* STA al       */ }, // 8C
    { "BCC", 1,  base, NULL,  BRA, PCRelative                     /* BCC r       */ }, { "STA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* STA (d),y  */ }, { "STA", 1,  base, NULL, NULL, DirectPage | Indirect  /* STA (d) */ }, { "STA", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* STA (d,s),y  */ }, // 90
    { "STY", 1,  base, NULL, NULL, DirectPage | IndexedX          /* STY d,x     */ }, { "STA", 1,  base, NULL, NULL, DirectPage | IndexedX            /* STA d,x    */ }, { "STX", 1,  base, NULL, NULL, DirectPage | IndexedY  /* STX d,y */ }, { "STA", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* STA [d],y    */ }, // 94
    { "TYA", 0,  base, NULL, NULL, Implied                        /* TYA i       */ }, { "STA", 2,  base, NULL, NULL, Absolute | IndexedY              /* STA a,y    */ }, { "TXS", 0,  base, NULL, NULL, Implied                /* TXS i   */ }, { "TXY", 0,  base, NULL, NULL, Implied                              /* TXY i        */ }, // 98
    { "STZ", 2,  base, NULL, NULL, Absolute                       /* STZ a       */ }, { "STA", 2,  base, NULL, NULL, Absolute | IndexedX              /* STA a,x    */ }, { "STZ", 2,  base, NULL, NULL, Absolute | IndexedX    /* STZ a,x */ }, { "STA", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* STA al,x     */ }, // 9C
    { "LDY", 1, x_set, NULL, NULL, Immediate                      /* LDY #       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* LDA (d,x)  */ }, { "LDX", 1, x_set, NULL, NULL, Immediate              /* LDX #   */ }, { "LDA", 1,  base, NULL, NULL, StackRelative                        /* LDA d,s      */ }, // A0
    { "LDY", 1,  base, NULL, NULL, DirectPage                     /* LDY d       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage                       /* LDA d      */ }, { "LDX", 1,  base, NULL, NULL, DirectPage             /* LDX d   */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* LDA [d]      */ }, // A4
    { "TAY", 0,  base, NULL, NULL, Implied                        /* TAY i       */ }, { "LDA", 1, m_set, NULL, NULL, Immediate                        /* LDA #      */ }, { "TAX", 0,  base, NULL, NULL, Implied                /* TAX i   */ }, { "PLB", 0,  base, NULL, NULL, Implied                              /* PLB s        */ }, // A8
    { "LDY", 2,  base, NULL, NULL, Absolute                       /* LDY a       */ }, { "LDA", 2,  base, NULL, NULL, Absolute                         /* LDA a      */ }, { "LDX", 2,  base, NULL, NULL, Absolute               /* LDX a   */ }, { "LDA", 3,  base, NULL, NULL, AbsoluteLong                         /* LDA al       */ }, // AC
    { "BCS", 1,  base, NULL,  BRA, PCRelative                     /* BCS r       */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* LDA (d),y  */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | Indirect  /* LDA (d) */ }, { "LDA", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* LDA (d,s),y  */ }, // B0
    { "LDY", 1,  base, NULL, NULL, DirectPage | IndexedX          /* LDY d,x     */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | IndexedX            /* LDA d,x    */ }, { "LDX", 1,  base, NULL, NULL, DirectPage | IndexedY  /* LDX d,y */ }, { "LDA", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* LDA [d],y    */ }, // B4
    { "CLV", 0,  base, NULL, NULL, Implied                        /* CLV i       */ }, { "LDA", 2,  base, NULL, NULL, Absolute | IndexedY              /* LDA a,y    */ }, { "TSX", 0,  base, NULL, NULL, Implied                /* TSX i   */ }, { "TYX", 0,  base, NULL, NULL, Implied                              /* TYX i        */ }, // B8
    { "LDY", 2,  base, NULL, NULL, Absolute | IndexedX            /* LDY a,x     */ }, { "LDA", 2,  base, NULL, NULL, Absolute | IndexedX              /* LDA a,x    */ }, { "LDX", 2,  base, NULL, NULL, Absolute | IndexedY    /* LDX a,y */ }, { "LDA", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* LDA al,x     */ }, // BC
    { "CPY", 1, x_set, NULL, NULL, Immediate                      /* CPY #       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* CMP (d,x)  */ }, { "REP", 1,  base,  REP, NULL, Immediate              /* REP #   */ }, { "CMP", 1,  base, NULL, NULL, StackRelative                        /* CMP d,s      */ }, // C0
    { "CPY", 1,  base, NULL, NULL, DirectPage                     /* CPY d       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage                       /* CMP d      */ }, { "DEC", 1,  base, NULL, NULL, DirectPage             /* DEC d   */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* CMP [d]      */ }, // C4
    { "INY", 0,  base, NULL, NULL, Implied                        /* INY i       */ }, { "CMP", 1, m_set, NULL, NULL, Immediate                        /* CMP #      */ }, { "DEX", 0,  base, NULL, NULL, Implied                /* DEX i   */ }, { "WAI", 0,  base, NULL, NULL, Implied                              /* WAI i        */ }, // C8
    { "CPY", 2,  base, NULL, NULL, Absolute                       /* CPY a       */ }, { "CMP", 2,  base, NULL, NULL, Absolute                         /* CMP a      */ }, { "DEC", 2,  base, NULL, NULL, Absolute               /* DEC a   */ }, { "CMP", 3,  base, NULL, NULL, AbsoluteLong                         /* CMP al       */ }, // CC
    { "BNE", 1,  base, NULL,  BRA, PCRelative                     /* BNE r       */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* CMP (d),y  */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | Indirect  /* CMP (d) */ }, { "CMP", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* CMP (d,s),y  */ }, // D0
    { "PEI", 1,  base, NULL, NULL, DirectPage | Indirect          /* PEI (d)     */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | IndexedX            /* CMP d,x    */ }, { "DEC", 1,  base, NULL, NULL, DirectPage | IndexedX  /* DEC d,x */ }, { "CMP", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* CMP [d],y    */ }, // D4
    { "CLD", 0,  base, NULL, NULL, Implied                        /* CLD i       */ }, { "CMP", 2,  base, NULL, NULL, Absolute | IndexedY              /* CMP a,y    */ }, { "PHX", 0,  base, NULL, NULL, Implied                /* PHX s   */ }, { "STP", 0,  base, NULL, NULL, Implied                              /* STP i        */ }, // D8
    { "JMP", 2,  base, NULL,  JMP, Absolute | IndexedLong         /* JMP [a]     */ }, { "CMP", 2,  base, NULL, NULL, Absolute | IndexedX              /* CMP a,x    */ }, { "DEC", 2,  base, NULL, NULL, Absolute | IndexedX    /* DEC a,x */ }, { "CMP", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* CMP al,x     */ }, // DC
    { "CPX", 1, x_set, NULL, NULL, Immediate                      /* CPX #       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedX /* SBC (d,x)  */ }, { "SEP", 1,  base,  SEP, NULL, Immediate              /* SEP #   */ }, { "SBC", 1,  base, NULL, NULL, StackRelative                        /* SBC d,s      */ }, // E0
    { "CPX", 1,  base, NULL, NULL, DirectPage                     /* CPX d       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage                       /* SBC d      */ }, { "INC", 1,  base, NULL, NULL, DirectPage             /* INC d   */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | IndexedLong             /* SBC [d]      */ }, // E4
    { "INX", 0,  base, NULL, NULL, Implied                        /* INX i       */ }, { "SBC", 1, m_set, NULL, NULL, Immediate                        /* SBC #      */ }, { "NOP", 0,  base, NULL, NULL, Implied                /* NOP i   */ }, { "XBA", 0,  base, NULL, NULL, Implied                              /* XBA i        */ }, // E8
    { "CPX", 2,  base, NULL, NULL, Absolute                       /* CPX a       */ }, { "SBC", 2,  base, NULL, NULL, Absolute                         /* SBC a      */ }, { "INC", 2,  base, NULL, NULL, Absolute               /* INC a   */ }, { "SBC", 3,  base, NULL, NULL, AbsoluteLong                         /* SBC al       */ }, // EC
    { "BEQ", 1,  base, NULL,  BRA, PCRelative                     /* BEQ r       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | Indirect | IndexedY /* SBC (d),y  */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | Indirect  /* SBC (d) */ }, { "SBC", 1,  base, NULL, NULL, StackRelative| Indirect | IndexedY   /* SBC (d,s),y  */ }, // F0
    { "PEA", 2,  base, NULL, NULL, Absolute                       /* PEA s       */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | IndexedX            /* SBC d,x    */ }, { "INC", 1,  base, NULL, NULL, DirectPage | IndexedX  /* INC d,x */ }, { "SBC", 1,  base, NULL, NULL, DirectPage | IndexedLong | IndexedY  /* SBC [d],y    */ }, // F4
    { "SED", 0,  base, NULL, NULL, Implied                        /* SED i       */ }, { "SBC", 2,  base, NULL, NULL, Absolute | IndexedY              /* SBC a,y    */ }, { "PLX", 0,  base, NULL, NULL, Implied                /* PLX s   */ }, { "XCE", 0,  base,  XCE, NULL, Implied                              /* XCE i        */ }, // F8
    { "JSR", 2,  base, NULL,  JMP, Absolute | Indirect | IndexedX /* JSR (a,x)   */ }, { "SBC", 2,  base, NULL, NULL, Absolute | IndexedX              /* SBC a,x    */ }, { "INC", 2,  base, NULL, NULL, Absolute | IndexedX    /* INC a,x */ }, { "SBC", 3,  base, NULL, NULL, AbsoluteLong | IndexedX              /* SBC al,x     */ }  // FC
};


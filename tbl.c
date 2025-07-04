#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "state.h"
#include "ops.h"

extern void make_label(uint32_t, uint32_t, const char*);

int m_set(int sz) {
    return isMSet()?sz+1:sz;
}

int x_set(int sz) {
    return isXSet()?sz+1:sz;
}

int base(int sz) {
    return sz;
}
/*
JSR, JSL and JMP turned into NOP's because we don't know where the start offset of the code
actually is, just the offset from the start of the buffer for it. Because of this we cannot,
effectively, compute where to insert labels. Making them NOP's for now.
*/
void JSR(uint32_t target_offset, uint32_t source_offset) {
//    make_label(source_offset, target_offset, "SUBROUTINE");
}

void JSL(uint32_t target_offset, uint32_t source_offset) {
//    make_label(source_offset, target_offset, "SUBROUTINE_LONG");
}

void JMP(uint32_t target_offset, uint32_t source_offset) {
//    make_label(source_offset, target_offset, "JMP_LABEL");
}

void BRL(uint32_t target_offset, uint32_t source_offset) {
    int32_t p = source_offset + (int8_t)target_offset;
    make_label(source_offset, p, "LOCAL_LONG");
}

void BRA(uint32_t target_offset, uint32_t source_offset) {
    int32_t p = source_offset + (int8_t)target_offset;
    make_label(source_offset, p, "LOCAL_SHORT");
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
    { "BRK",   1,  base, NULL, NULL, NULL     , Immediate                             /* BRK  s         */ }, //  0x00
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* ORA  (d,x)     */ }, //  0x01
    { "COP",   1,  base, NULL, NULL, READ_8   , Immediate                             /* COP  #         */ }, //  0x02
    { "ORA",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* ORA  d,s       */ }, //  0x03
    { "TSB",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* TSB  d         */ }, //  0x04
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ORA  d         */ }, //  0x05
    { "ASL",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ASL  d         */ }, //  0x06
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* ORA  [d]       */ }, //  0x07
    { "PHP",   0,  base, NULL, NULL, NULL     , Implied                               /* PHP  s         */ }, //  0x08
    { "ORA",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* ORA  #         */ }, //  0x09
    { "ASL",   0,  base, NULL, NULL, NULL     , Implied                               /* ASL  A         */ }, //  0x0A
    { "PHD",   0,  base, NULL, NULL, NULL     , Implied                               /* PHD  s         */ }, //  0x0B
    { "TSB",   2,  base, NULL, NULL, READ_16  , Absolute                              /* TSB  a         */ }, //  0x0C
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ORA  a         */ }, //  0x0D
    { "ASL",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ASL  a         */ }, //  0x0E
    { "ORA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* ORA  al        */ }, //  0x0F
    { "BPL",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BPL  r         */ }, //  0x10
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* ORA  (d),y     */ }, //  0x11
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* ORA  (d)       */ }, //  0x12
    { "ORA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* ORA  (d,s),y   */ }, //  0x13
    { "TRB",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* TRB  d         */ }, //  0x14
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ORA  d,x       */ }, //  0x15
    { "ASL",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ASL  d,x       */ }, //  0x16
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* ORA  [d],y     */ }, //  0x17
    { "CLC",   0,  base,  CLC, NULL, NULL     , Implied                               /* CLC  i         */ }, //  0x18
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* ORA  a,y       */ }, //  0x19
    { "INC",   0,  base, NULL, NULL, NULL     , Implied                               /* INC  A         */ }, //  0x1A
    { "TCS",   0,  base, NULL, NULL, NULL     , Implied                               /* TCS  i         */ }, //  0x1B
    { "TRB",   2,  base, NULL, NULL, READ_16  , Absolute                              /* TRB  a         */ }, //  0x1C
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ORA  a,x       */ }, //  0x1D
    { "ASL",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ASL  a,x       */ }, //  0x1E
    { "ORA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* ORA  al,x      */ }, //  0x1F
    { "JSR",   2,  base, NULL,  JSR, READ_16  , Absolute                              /* JSR  a         */ }, //  0x20
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* AND  (d,x)     */ }, //  0x21
    { "JSL",   3,  base, NULL,  JSL, READ_24  , AbsoluteLong                          /* JSL  al        */ }, //  0x22
    { "AND",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* AND  d,s       */ }, //  0x23
    { "BIT",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* BIT  d         */ }, //  0x24
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* AND  d         */ }, //  0x25
    { "ROL",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ROL  d         */ }, //  0x26
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* AND  [d]       */ }, //  0x27
    { "PLP",   0,  base, NULL, NULL, NULL     , Implied                               /* PLP  s         */ }, //  0x28
    { "AND",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* AND  #         */ }, //  0x29
    { "ROL",   0,  base, NULL, NULL, NULL     , Implied                               /* ROL  A         */ }, //  0x2A
    { "PLD",   0,  base, NULL, NULL, NULL     , Implied                               /* PLD  s         */ }, //  0x2B
    { "BIT",   2,  base, NULL, NULL, READ_16  , Absolute                              /* BIT  a         */ }, //  0x2C
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute                              /* AND  a         */ }, //  0x2D
    { "ROL",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ROL  a         */ }, //  0x2E
    { "AND",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* AND  al        */ }, //  0x2F
    { "BMI",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BMI  R         */ }, //  0x30
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* AND  (d),y     */ }, //  0x31
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* AND  (d)       */ }, //  0x32
    { "AND",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* AND  (d,s),y   */ }, //  0x33
    { "BIT",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* BIT  d,x       */ }, //  0x34
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* AND  d,x       */ }, //  0x35
    { "ROL",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ROL  d,x       */ }, //  0x36
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* AND  [d],y     */ }, //  0x37
    { "SEC",   0,  base,  SEC, NULL, NULL     , Implied                               /* SEC  i         */ }, //  0x38
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* AND  a,y       */ }, //  0x39
    { "DEC",   0,  base, NULL, NULL, NULL     , Implied                               /* DEC  A         */ }, //  0x3A
    { "TSC",   0,  base, NULL, NULL, NULL     , Implied                               /* TSC  i         */ }, //  0x3B
    { "BIT",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* BIT  a,x       */ }, //  0x3C
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* AND  a,x       */ }, //  0x3D
    { "ROL",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ROL  a,x       */ }, //  0x3E
    { "AND",   3,  base, NULL, NULL, READ_24  , Absolute | IndirectLong | IndexedX    /* AND  al,x      */ }, //  0x3F
    { "RTI",   0,  base, NULL, NULL, NULL     , Implied                               /* RTI  s         */ }, //  0x40
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* EOR  (d,x)     */ }, //  0x41
    { "WDM",   1,  base, NULL, NULL, READ_8   , Implied                               /* WDM  i         */ }, //  0x42
    { "EOR",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* EOR  d,s       */ }, //  0x43
    { "MVP",   2,  base, NULL, NULL, READ_BMA , BlockMoveAddress                      /* MVP  src,dst   */ }, //  0x44
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* EOR  d         */ }, //  0x45
    { "LSR",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LSR  d         */ }, //  0x46
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* EOR  [d]       */ }, //  0x47
    { "PHA",   0,  base, NULL, NULL, NULL     , Implied                               /* PHA  s         */ }, //  0x48
    { "EOR",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* EOR  #         */ }, //  0x49
    { "LSR",   0,  base, NULL, NULL, NULL     , Implied                               /* LSR  A         */ }, //  0x4A
    { "PHK",   0,  base, NULL, NULL, NULL     , Implied                               /* PHK  s         */ }, //  0x4B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute                              /* JMP  a         */ }, //  0x4C
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute                              /* EOR  a         */ }, //  0x4D
    { "LSR",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LSR  a         */ }, //  0x4E
    { "EOR",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* EOR  al        */ }, //  0x4F
    { "BVC",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BVC  r         */ }, //  0x50
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* EOR  (d),y     */ }, //  0x51
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* EOR  (d)       */ }, //  0x52
    { "EOR",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* EOR  (d,s),y   */ }, //  0x53
    { "MVN",   2,  base, NULL, NULL, READ_BMA , BlockMoveAddress                      /* MVN  src,dst   */ }, //  0x54
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* EOR  d,x       */ }, //  0x55
    { "LSR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* LSR  d,x       */ }, //  0x56
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* EOR  [d],y     */ }, //  0x57
    { "CLI",   0,  base, NULL, NULL, NULL     , Implied                               /* CLI  i         */ }, //  0x58
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* EOR  a,y       */ }, //  0x59
    { "PHY",   0,  base, NULL, NULL, NULL     , Implied                               /* PHY  s         */ }, //  0x5A
    { "TCD",   0,  base, NULL, NULL, NULL     , Implied                               /* TCD  i         */ }, //  0x5B
    { "JMP",   3,  base, NULL,  JMP, READ_24  , AbsoluteLong                          /* JMP  al        */ }, //  0x5C
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* EOR  a,x       */ }, //  0x5D
    { "LSR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* LSR  a,x       */ }, //  0x5E
    { "EOR",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* EOR  al,x      */ }, //  0x5F
    { "RTS",   0,  base, NULL, NULL, NULL     , Implied                               /* RTS  s         */ }, //  0x60
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* ADC  (dp,X)    */ }, //  0x61
    { "PER",   2,  base, NULL, NULL, READ_16  , PCRelativeLong                        /* PER  s         */ }, //  0x62
    { "ADC",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* ADC  sr,S      */ }, //  0x63
    { "STZ",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STZ  d         */ }, //  0x64
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ADC  dp        */ }, //  0x65
    { "ROR",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ROR  d         */ }, //  0x66
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* ADC  [dp]      */ }, //  0x67
    { "PLA",   0,  base, NULL, NULL, NULL     , Implied                               /* PLA  s         */ }, //  0x68
    { "ADC",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* ADC  #         */ }, //  0x69
    { "ROR",   0,  base, NULL, NULL, NULL     , Implied                               /* ROR  A         */ }, //  0x6A
    { "RTL",   0,  base, NULL, NULL, NULL     , Implied                               /* RTL  s         */ }, //  0x6B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | Indirect                   /* JMP  (a)       */ }, //  0x6C
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ADC  addr      */ }, //  0x6D
    { "ROR",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ROR  a         */ }, //  0x6E
    { "ADC",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* ADC  al        */ }, //  0x6F
    { "BVS",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BVS  r         */ }, //  0x70
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* ADC  (d),y     */ }, //  0x71
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* ADC  (d)       */ }, //  0x72
    { "ADC",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* ADC  (sr,S),y  */ }, //  0x73
    { "STZ",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* STZ  d,x       */ }, //  0x74
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ADC  d,x       */ }, //  0x75
    { "ROR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ROR  d,x       */ }, //  0x76
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* ADC  [d],y     */ }, //  0x77
    { "SEI",   0,  base, NULL, NULL, NULL     , Implied                               /* SEI  i         */ }, //  0x78
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* ADC  a,y       */ }, //  0x79
    { "PLY",   0,  base, NULL, NULL, NULL     , Implied                               /* PLY  s         */ }, //  0x7A
    { "TDC",   0,  base, NULL, NULL, NULL     , Implied                               /* TDC  i         */ }, //  0x7B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | Indirect | IndexedX        /* JMP  (a,x)     */ }, //  0x7C
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ADC  a,x       */ }, //  0x7D
    { "ROR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ROR  a,x       */ }, //  0x7E
    { "ADC",   3,  base, NULL, NULL, READ_16  , AbsoluteLong | IndexedX               /* ADC  al,x      */ }, //  0x7F
    { "BRA",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BRA  r         */ }, //  0x80
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* STA  (d,x)     */ }, //  0x81
    { "BRL",   2,  base, NULL,  BRL, READ_16  , PCRelativeLong                        /* BRL  rl        */ }, //  0x82
    { "STA",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* STA  d,s       */ }, //  0x83
    { "STY",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STY  d         */ }, //  0x84
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STA  d         */ }, //  0x85
    { "STX",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STX  d         */ }, //  0x86
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* STA  [d]       */ }, //  0x87
    { "DEY",   0,  base, NULL, NULL, NULL     , Implied                               /* DEY  i         */ }, //  0x88
    { "BIT",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* BIT  #         */ }, //  0x89
    { "TXA",   0,  base, NULL, NULL, NULL     , Implied                               /* TXA  i         */ }, //  0x8A
    { "PHB",   0,  base, NULL, NULL, NULL     , Implied                               /* PHB  s         */ }, //  0x8B
    { "STY",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STY  a         */ }, //  0x8C
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STA  a         */ }, //  0x8D
    { "STX",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STX  a         */ }, //  0x8E
    { "STA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* STA  al        */ }, //  0x8F
    { "BCC",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BCC  r         */ }, //  0x90
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* STA  (d),y     */ }, //  0x91
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* STA  (d)       */ }, //  0x92
    { "STA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* STA  (d,s),y   */ }, //  0x93
    { "STY",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* STY  d,x       */ }, //  0x94
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* STA  d,x       */ }, //  0x95
    { "STX",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedY                 /* STX  d,y       */ }, //  0x96
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* STA  [d],y     */ }, //  0x97
    { "TYA",   0,  base, NULL, NULL, NULL     , Implied                               /* TYA  i         */ }, //  0x98
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* STA  a,y       */ }, //  0x99
    { "TXS",   0,  base, NULL, NULL, NULL     , Implied                               /* TXS  i         */ }, //  0x9A
    { "TXY",   0,  base, NULL, NULL, NULL     , Implied                               /* TXY  i         */ }, //  0x9B
    { "STZ",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STZ  a         */ }, //  0x9C
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* STA  a,x       */ }, //  0x9D
    { "STZ",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* STZ  a,x       */ }, //  0x9E
    { "STA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* STA  al,x      */ }, //  0x9F
    { "LDY",   1, x_set, NULL, NULL, READ_8_16, Immediate                             /* LDY  #         */ }, //  0xA0
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* LDA  (d,x)     */ }, //  0xA1
    { "LDX",   1, x_set, NULL, NULL, READ_8_16, Immediate                             /* LDX  #         */ }, //  0xA2
    { "LDA",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* LDA  d,s       */ }, //  0xA3
    { "LDY",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LDY  d         */ }, //  0xA4
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LDA  d         */ }, //  0xA5
    { "LDX",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LDX  d         */ }, //  0xA6
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* LDA  [d]       */ }, //  0xA7
    { "TAY",   0,  base, NULL, NULL, NULL     , Implied                               /* TAY  i         */ }, //  0xA8
    { "LDA",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* LDA  #         */ }, //  0xA9
    { "TAX",   0,  base, NULL, NULL, NULL     , Implied                               /* TAX  i         */ }, //  0xAA
    { "PLB",   0,  base, NULL, NULL, NULL     , Implied                               /* PLB  s         */ }, //  0xAB
    { "LDY",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LDY  a         */ }, //  0xAC
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LDA  a         */ }, //  0xAD
    { "LDX",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LDX  a         */ }, //  0xAE
    { "LDA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* LDA  al        */ }, //  0xAF
    { "BCS",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BCS  r         */ }, //  0xB0
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* LDA  (d),y     */ }, //  0xB1
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* LDA  (d)       */ }, //  0xB2
    { "LDA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* LDA  (d,s),y   */ }, //  0xB3
    { "LDY",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* LDY  d,x       */ }, //  0xB4
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* LDA  d,x       */ }, //  0xB5
    { "LDX",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedY                 /* LDX  d,y       */ }, //  0xB6
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* LDA  [d],y     */ }, //  0xB7
    { "CLV",   0,  base, NULL, NULL, NULL     , Implied                               /* CLV  i         */ }, //  0xB8
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* LDA  a,y       */ }, //  0xB9
    { "TSX",   0,  base, NULL, NULL, NULL     , Implied                               /* TSX  i         */ }, //  0xBA
    { "TYX",   0,  base, NULL, NULL, NULL     , Implied                               /* TYX  i         */ }, //  0xBB
    { "LDY",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* LDY  a,x       */ }, //  0xBC
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* LDA  a,x       */ }, //  0xBD
    { "LDX",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* LDX  a,y       */ }, //  0xBE
    { "LDA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* LDA  al,x      */ }, //  0xBF
    { "CPY",   1, x_set, NULL, NULL, READ_8   , Immediate                             /* CPY  #         */ }, //  0xC0
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* CMP  (d,x)     */ }, //  0xC1
    { "REP",   1,  base,  REP, NULL, READ_8   , Immediate                             /* REP  #         */ }, //  0xC2
    { "CMP",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* CMP  d,s       */ }, //  0xC3
    { "CPY",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* CPY  d         */ }, //  0xC4
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* CMP  d         */ }, //  0xC5
    { "DEC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* DEC  d         */ }, //  0xC6
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* CMP  [d]       */ }, //  0xC7
    { "INY",   0,  base, NULL, NULL, NULL     , Implied                               /* INY  i         */ }, //  0xC8
    { "CMP",   1, m_set, NULL, NULL, READ_8   , Immediate                             /* CMP  #         */ }, //  0xC9
    { "DEX",   0,  base, NULL, NULL, NULL     , Implied                               /* DEX  i         */ }, //  0xCA
    { "WAI",   0,  base, NULL, NULL, NULL     , Implied                               /* WAI  i         */ }, //  0xCB
    { "CPY",   2,  base, NULL, NULL, READ_16  , Absolute                              /* CPY  a         */ }, //  0xCC
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute                              /* CMP  a         */ }, //  0xCD
    { "DEC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* DEC  a         */ }, //  0xCE
    { "CMP",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* CMP  al        */ }, //  0xCF
    { "BNE",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BNE  r         */ }, //  0xD0
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* CMP  (d),y     */ }, //  0xD1
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* CMP  (d)       */ }, //  0xD2
    { "CMP",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* CMP  (d,s),y   */ }, //  0xD3
    { "PEI",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* PEI  (dp)      */ }, //  0xD4
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* CMP  d,x       */ }, //  0xD5
    { "DEC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* DEC  d,x       */ }, //  0xD6
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* CMP  [d],y     */ }, //  0xD7
    { "CLD",   0,  base, NULL, NULL, NULL     , Implied                               /* CLD  i         */ }, //  0xD8
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* CMP  a,y       */ }, //  0xD9
    { "PHX",   0,  base, NULL, NULL, NULL     , Implied                               /* PHX  s         */ }, //  0xDA
    { "STP",   0,  base, NULL, NULL, NULL     , Implied                               /* STP  i         */ }, //  0xDB
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | IndirectLong               /* JMP  [a]       */ }, //  0xDC
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* CMP  a,x       */ }, //  0xDD
    { "DEC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* DEC  a,x       */ }, //  0xDE
    { "CMP",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* CMP  al,x      */ }, //  0xDF
    { "CPX",   1, x_set, NULL, NULL, READ_8   , Immediate                             /* CPX  #         */ }, //  0xE0
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* SBC  (d,x)     */ }, //  0xE1
    { "SEP",   1,  base,  SEP, NULL, READ_8   , Immediate                             /* SEP  #         */ }, //  0xE2
    { "SBC",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* SBC  d,s       */ }, //  0xE3
    { "CPX",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* CPX  d         */ }, //  0xE4
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* SBC  d         */ }, //  0xE5
    { "INC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* INC  d         */ }, //  0xE6
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* SBC  [d]       */ }, //  0xE7
    { "INX",   0,  base, NULL, NULL, NULL     , Implied                               /* INX  i         */ }, //  0xE8
    { "SBC",   1, m_set, NULL, NULL, READ_8   , Immediate                             /* SBC  #         */ }, //  0xE9
    { "NOP",   0,  base, NULL, NULL, NULL     , Implied                               /* NOP  i         */ }, //  0xEA
    { "XBA",   0,  base, NULL, NULL, NULL     , Implied                               /* XBA  i         */ }, //  0xEB
    { "CPX",   2,  base, NULL, NULL, READ_16  , Absolute                              /* CPX  a         */ }, //  0xEC
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* SBC  a         */ }, //  0xED
    { "INC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* INC  a         */ }, //  0xEE
    { "SBC",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* SBC  al        */ }, //  0xEF
    { "BEQ",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BEQ  r         */ }, //  0xF0
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* SBC  (d),y     */ }, //  0xF1
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* SBC  (d)       */ }, //  0xF2
    { "SBC",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* SBC  (d,s),y   */ }, //  0xF3
    { "PEA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* PEA  s         */ }, //  0xF4
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* SBC  d,x       */ }, //  0xF5
    { "INC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* INC  d,x       */ }, //  0xF6
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* SBC  [d],y     */ }, //  0xF7
    { "SED",   0,  base, NULL, NULL, NULL     , Implied                               /* SED  i         */ }, //  0xF8
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* SBC  a,y       */ }, //  0xF9
    { "PLX",   0,  base, NULL, NULL, NULL     , Implied                               /* PLX  s         */ }, //  0xFA
    { "XCE",   0,  base,  XCE, NULL, NULL     , Implied                               /* XCE  i         */ }, //  0xFB
    { "JSR",   2,  base, NULL, NULL, READ_16  , Absolute | Indirect | IndexedX        /* JSR  (a,x)     */ }, //  0xFC
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* SBC  a,x       */ }, //  0xFD
    { "INC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* INC  a,x       */ }, //  0xFE
    { "SBC",   3,  base, NULL, NULL, READ_16  , AbsoluteLong | IndexedX               /* SBC  al,x      */ }, //  0xFF
};
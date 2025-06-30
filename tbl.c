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

void JMP(uint32_t target_offset, uint32_t source_offset) {
    // TODO: stub!
    // make a label -- TODO: fix the f*cking naming to be dynamic
    make_label(source_offset, target_offset, "JMP_LABEL");
}

void BRL(uint32_t target_offset, uint32_t source_offset) {
    // TODO: stub!
    // make a label -- TODO: fix the f*cking naming to be dynamic
    make_label(source_offset, target_offset, "LOCAL_LONG");
}

void BRA(uint32_t target_offset, uint32_t source_offset) {
    // TODO: stub!
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
    { "BRK",   1,  base, NULL, NULL, NULL     , Immediate                             /* BRK  s         */ }, //   0X0
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* ORA  (d,x)     */ }, //   0X1
    { "COP",   1,  base, NULL, NULL, READ_8   , Immediate                             /* COP  #         */ }, //   0X2
    { "ORA",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* ORA  d,s       */ }, //   0X3
    { "TSB",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* TSB  d         */ }, //   0X4
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ORA  d         */ }, //   0X5
    { "ASL",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ASL  d         */ }, //   0X6
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* ORA  [d]       */ }, //   0X7
    { "PHP",   0,  base, NULL, NULL, NULL     , Implied                               /* PHP  s         */ }, //   0X8
    { "ORA",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* ORA  #         */ }, //   0X9
    { "ASL",   0,  base, NULL, NULL, NULL     , Implied                               /* ASL  A         */ }, //   0XA
    { "PHD",   0,  base, NULL, NULL, NULL     , Implied                               /* PHD  s         */ }, //   0XB
    { "TSB",   2,  base, NULL, NULL, READ_16  , Absolute                              /* TSB  a         */ }, //   0XC
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ORA  a         */ }, //   0XD
    { "ASL",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ASL  a         */ }, //   0XE
    { "ORA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* ORA  al        */ }, //   0XF
    { "BPL",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BPL  r         */ }, //  0X10
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* ORA  (d),y     */ }, //  0X11
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* ORA  (d)       */ }, //  0X12
    { "ORA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* ORA  (d,s),y   */ }, //  0X13
    { "TRB",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* TRB  d         */ }, //  0X14
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ORA  d,x       */ }, //  0X15
    { "ASL",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ASL  d,x       */ }, //  0X16
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* ORA  [d],y     */ }, //  0X17
    { "CLC",   0,  base,  CLC, NULL, NULL     , Implied                               /* CLC  i         */ }, //  0X18
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* ORA  a,y       */ }, //  0X19
    { "INC",   0,  base, NULL, NULL, NULL     , Implied                               /* INC  A         */ }, //  0X1A
    { "TCS",   0,  base, NULL, NULL, NULL     , Implied                               /* TCS  i         */ }, //  0X1B
    { "TRB",   2,  base, NULL, NULL, READ_16  , Absolute                              /* TRB  a         */ }, //  0X1C
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ORA  a,x       */ }, //  0X1D
    { "ASL",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ASL  a,x       */ }, //  0X1E
    { "ORA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* ORA  al,x      */ }, //  0X1F
    { "JSR",   2,  base, NULL,  JMP, READ_16  , Absolute                              /* JSR  a         */ }, //  0X20
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* AND  (d,x)     */ }, //  0X21
    { "JSL",   3,  base, NULL,  JMP, READ_24  , AbsoluteLong                          /* JSL  al        */ }, //  0X22
    { "AND",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* AND  d,s       */ }, //  0X23
    { "BIT",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* BIT  d         */ }, //  0X24
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* AND  d         */ }, //  0X25
    { "ROL",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ROL  d         */ }, //  0X26
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* AND  [d]       */ }, //  0X27
    { "PLP",   0,  base, NULL, NULL, NULL     , Implied                               /* PLP  s         */ }, //  0X28
    { "AND",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* AND  #         */ }, //  0X29
    { "ROL",   0,  base, NULL, NULL, NULL     , Implied                               /* ROL  A         */ }, //  0X2A
    { "PLD",   0,  base, NULL, NULL, NULL     , Implied                               /* PLD  s         */ }, //  0X2B
    { "BIT",   2,  base, NULL, NULL, READ_16  , Absolute                              /* BIT  a         */ }, //  0X2C
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute                              /* AND  a         */ }, //  0X2D
    { "ROL",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ROL  a         */ }, //  0X2E
    { "AND",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* AND  al        */ }, //  0X2F
    { "BMI",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BMI  R         */ }, //  0X30
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* AND  (d),y     */ }, //  0X31
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* AND  (d)       */ }, //  0X32
    { "AND",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* AND  (d,s),y   */ }, //  0X33
    { "BIT",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* BIT  d,x       */ }, //  0X34
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* AND  d,x       */ }, //  0X35
    { "ROL",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ROL  d,x       */ }, //  0X36
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* AND  [d],y     */ }, //  0X37
    { "SEC",   0,  base,  SEC, NULL, NULL     , Implied                               /* SEC  i         */ }, //  0X38
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* AND  a,y       */ }, //  0X39
    { "DEC",   0,  base, NULL, NULL, NULL     , Implied                               /* DEC  A         */ }, //  0X3A
    { "TSC",   0,  base, NULL, NULL, NULL     , Implied                               /* TSC  i         */ }, //  0X3B
    { "BIT",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* BIT  a,x       */ }, //  0X3C
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* AND  a,x       */ }, //  0X3D
    { "ROL",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ROL  a,x       */ }, //  0X3E
    { "AND",   3,  base, NULL, NULL, READ_24  , Absolute | IndirectLong | IndexedX    /* AND  al,x      */ }, //  0X3F
    { "RTI",   0,  base, NULL, NULL, NULL     , Implied                               /* RTI  s         */ }, //  0X40
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* EOR  (d,x)     */ }, //  0X41
    { "WDM",   1,  base, NULL, NULL, READ_8   , Implied                               /* WDM  i         */ }, //  0X42
    { "EOR",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* EOR  d,s       */ }, //  0X43
    { "MVP",   2,  base, NULL, NULL, READ_BMA , BlockMoveAddress                      /* MVP  src,dst   */ }, //  0X44
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* EOR  d         */ }, //  0X45
    { "LSR",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LSR  d         */ }, //  0X46
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* EOR  [d]       */ }, //  0X47
    { "PHA",   0,  base, NULL, NULL, NULL     , Implied                               /* PHA  s         */ }, //  0X48
    { "EOR",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* EOR  #         */ }, //  0X49
    { "LSR",   0,  base, NULL, NULL, NULL     , Implied                               /* LSR  A         */ }, //  0X4A
    { "PHK",   0,  base, NULL, NULL, NULL     , Implied                               /* PHK  s         */ }, //  0X4B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute                              /* JMP  a         */ }, //  0X4C
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute                              /* EOR  a         */ }, //  0X4D
    { "LSR",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LSR  a         */ }, //  0X4E
    { "EOR",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* EOR  al        */ }, //  0X4F
    { "BVC",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BVC  r         */ }, //  0X50
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* EOR  (d),y     */ }, //  0X51
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* EOR  (d)       */ }, //  0X52
    { "EOR",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* EOR  (d,s),y   */ }, //  0X53
    { "MVN",   2,  base, NULL, NULL, READ_BMA , BlockMoveAddress                      /* MVN  src,dst   */ }, //  0X54
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* EOR  d,x       */ }, //  0X55
    { "LSR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* LSR  d,x       */ }, //  0X56
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* EOR  [d],y     */ }, //  0X57
    { "CLI",   0,  base, NULL, NULL, NULL     , Implied                               /* CLI  i         */ }, //  0X58
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* EOR  a,y       */ }, //  0X59
    { "PHY",   0,  base, NULL, NULL, NULL     , Implied                               /* PHY  s         */ }, //  0X5A
    { "TCD",   0,  base, NULL, NULL, NULL     , Implied                               /* TCD  i         */ }, //  0X5B
    { "JMP",   3,  base, NULL,  JMP, READ_24  , AbsoluteLong                          /* JMP  al        */ }, //  0X5C
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* EOR  a,x       */ }, //  0X5D
    { "LSR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* LSR  a,x       */ }, //  0X5E
    { "EOR",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* EOR  al,x      */ }, //  0X5F
    { "RTS",   0,  base, NULL, NULL, NULL     , Implied                               /* RTS  s         */ }, //  0X60
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* ADC  (dp,X)    */ }, //  0X61
    { "PER",   2,  base, NULL, NULL, READ_16  , PCRelativeLong                        /* PER  s         */ }, //  0X62
    { "ADC",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* ADC  sr,S      */ }, //  0X63
    { "STZ",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STZ  d         */ }, //  0X64
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ADC  dp        */ }, //  0X65
    { "ROR",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* ROR  d         */ }, //  0X66
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* ADC  [dp]      */ }, //  0X67
    { "PLA",   0,  base, NULL, NULL, NULL     , Implied                               /* PLA  s         */ }, //  0X68
    { "ADC",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* ADC  #         */ }, //  0X69
    { "ROR",   0,  base, NULL, NULL, NULL     , Implied                               /* ROR  A         */ }, //  0X6A
    { "RTL",   0,  base, NULL, NULL, NULL     , Implied                               /* RTL  s         */ }, //  0X6B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | Indirect                   /* JMP  (a)       */ }, //  0X6C
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ADC  addr      */ }, //  0X6D
    { "ROR",   2,  base, NULL, NULL, READ_16  , Absolute                              /* ROR  a         */ }, //  0X6E
    { "ADC",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* ADC  al        */ }, //  0X6F
    { "BVS",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BVS  r         */ }, //  0X70
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* ADC  (d),y     */ }, //  0X71
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* ADC  (d)       */ }, //  0X72
    { "ADC",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* ADC  (sr,S),y  */ }, //  0X73
    { "STZ",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* STZ  d,x       */ }, //  0X74
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ADC  d,x       */ }, //  0X75
    { "ROR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* ROR  d,x       */ }, //  0X76
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* ADC  [d],y     */ }, //  0X77
    { "SEI",   0,  base, NULL, NULL, NULL     , Implied                               /* SEI  i         */ }, //  0X78
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* ADC  a,y       */ }, //  0X79
    { "PLY",   0,  base, NULL, NULL, NULL     , Implied                               /* PLY  s         */ }, //  0X7A
    { "TDC",   0,  base, NULL, NULL, NULL     , Implied                               /* TDC  i         */ }, //  0X7B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | Indirect | IndexedX        /* JMP  (a,x)     */ }, //  0X7C
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ADC  a,x       */ }, //  0X7D
    { "ROR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* ROR  a,x       */ }, //  0X7E
    { "ADC",   3,  base, NULL, NULL, READ_16  , AbsoluteLong | IndexedX               /* ADC  al,x      */ }, //  0X7F
    { "BRA",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BRA  r         */ }, //  0X80
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* STA  (d,x)     */ }, //  0X81
    { "BRL",   2,  base, NULL,  BRL, READ_16  , PCRelativeLong                        /* BRL  rl        */ }, //  0X82
    { "STA",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* STA  d,s       */ }, //  0X83
    { "STY",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STY  d         */ }, //  0X84
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STA  d         */ }, //  0X85
    { "STX",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* STX  d         */ }, //  0X86
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* STA  [d]       */ }, //  0X87
    { "DEY",   0,  base, NULL, NULL, NULL     , Implied                               /* DEY  i         */ }, //  0X88
    { "BIT",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* BIT  #         */ }, //  0X89
    { "TXA",   0,  base, NULL, NULL, NULL     , Implied                               /* TXA  i         */ }, //  0X8A
    { "PHB",   0,  base, NULL, NULL, NULL     , Implied                               /* PHB  s         */ }, //  0X8B
    { "STY",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STY  a         */ }, //  0X8C
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STA  a         */ }, //  0X8D
    { "STX",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STX  a         */ }, //  0X8E
    { "STA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* STA  al        */ }, //  0X8F
    { "BCC",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BCC  r         */ }, //  0X90
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* STA  (d),y     */ }, //  0X91
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* STA  (d)       */ }, //  0X92
    { "STA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* STA  (d,s),y   */ }, //  0X93
    { "STY",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* STY  d,x       */ }, //  0X94
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* STA  d,x       */ }, //  0X95
    { "STX",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedY                 /* STX  d,y       */ }, //  0X96
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* STA  [d],y     */ }, //  0X97
    { "TYA",   0,  base, NULL, NULL, NULL     , Implied                               /* TYA  i         */ }, //  0X98
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* STA  a,y       */ }, //  0X99
    { "TXS",   0,  base, NULL, NULL, NULL     , Implied                               /* TXS  i         */ }, //  0X9A
    { "TXY",   0,  base, NULL, NULL, NULL     , Implied                               /* TXY  i         */ }, //  0X9B
    { "STZ",   2,  base, NULL, NULL, READ_16  , Absolute                              /* STZ  a         */ }, //  0X9C
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* STA  a,x       */ }, //  0X9D
    { "STZ",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* STZ  a,x       */ }, //  0X9E
    { "STA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* STA  al,x      */ }, //  0X9F
    { "LDY",   1, x_set, NULL, NULL, READ_8_16, Immediate                             /* LDY  #         */ }, //  0XA0
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* LDA  (d,x)     */ }, //  0XA1
    { "LDX",   1, x_set, NULL, NULL, READ_8_16, Immediate                             /* LDX  #         */ }, //  0XA2
    { "LDA",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* LDA  d,s       */ }, //  0XA3
    { "LDY",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LDY  d         */ }, //  0XA4
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LDA  d         */ }, //  0XA5
    { "LDX",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* LDX  d         */ }, //  0XA6
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* LDA  [d]       */ }, //  0XA7
    { "TAY",   0,  base, NULL, NULL, NULL     , Implied                               /* TAY  i         */ }, //  0XA8
    { "LDA",   1, m_set, NULL, NULL, READ_8_16, Immediate                             /* LDA  #         */ }, //  0XA9
    { "TAX",   0,  base, NULL, NULL, NULL     , Implied                               /* TAX  i         */ }, //  0XAA
    { "PLB",   0,  base, NULL, NULL, NULL     , Implied                               /* PLB  s         */ }, //  0XAB
    { "LDY",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LDY  a         */ }, //  0XAC
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LDA  a         */ }, //  0XAD
    { "LDX",   2,  base, NULL, NULL, READ_16  , Absolute                              /* LDX  a         */ }, //  0XAE
    { "LDA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* LDA  al        */ }, //  0XAF
    { "BCS",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BCS  r         */ }, //  0XB0
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* LDA  (d),y     */ }, //  0XB1
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* LDA  (d)       */ }, //  0XB2
    { "LDA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* LDA  (d,s),y   */ }, //  0XB3
    { "LDY",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* LDY  d,x       */ }, //  0XB4
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* LDA  d,x       */ }, //  0XB5
    { "LDX",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedY                 /* LDX  d,y       */ }, //  0XB6
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* LDA  [d],y     */ }, //  0XB7
    { "CLV",   0,  base, NULL, NULL, NULL     , Implied                               /* CLV  i         */ }, //  0XB8
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* LDA  a,y       */ }, //  0XB9
    { "TSX",   0,  base, NULL, NULL, NULL     , Implied                               /* TSX  i         */ }, //  0XBA
    { "TYX",   0,  base, NULL, NULL, NULL     , Implied                               /* TYX  i         */ }, //  0XBB
    { "LDY",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* LDY  a,x       */ }, //  0XBC
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* LDA  a,x       */ }, //  0XBD
    { "LDX",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* LDX  a,y       */ }, //  0XBE
    { "LDA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* LDA  al,x      */ }, //  0XBF
    { "CPY",   1, x_set, NULL, NULL, READ_8   , Immediate                             /* CPY  #         */ }, //  0XC0
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* CMP  (d,x)     */ }, //  0XC1
    { "REP",   1,  base,  REP, NULL, READ_8   , Immediate                             /* REP  #         */ }, //  0XC2
    { "CMP",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* CMP  d,s       */ }, //  0XC3
    { "CPY",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* CPY  d         */ }, //  0XC4
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* CMP  d         */ }, //  0XC5
    { "DEC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* DEC  d         */ }, //  0XC6
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* CMP  [d]       */ }, //  0XC7
    { "INY",   0,  base, NULL, NULL, NULL     , Implied                               /* INY  i         */ }, //  0XC8
    { "CMP",   1, m_set, NULL, NULL, READ_8   , Immediate                             /* CMP  #         */ }, //  0XC9
    { "DEX",   0,  base, NULL, NULL, NULL     , Implied                               /* DEX  i         */ }, //  0XCA
    { "WAI",   0,  base, NULL, NULL, NULL     , Implied                               /* WAI  i         */ }, //  0XCB
    { "CPY",   2,  base, NULL, NULL, READ_16  , Absolute                              /* CPY  a         */ }, //  0XCC
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute                              /* CMP  a         */ }, //  0XCD
    { "DEC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* DEC  a         */ }, //  0XCE
    { "CMP",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* CMP  al        */ }, //  0XCF
    { "BNE",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BNE  r         */ }, //  0XD0
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* CMP  (d),y     */ }, //  0XD1
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* CMP  (d)       */ }, //  0XD2
    { "CMP",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* CMP  (d,s),y   */ }, //  0XD3
    { "PEI",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* PEI  (dp)      */ }, //  0XD4
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* CMP  d,x       */ }, //  0XD5
    { "DEC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* DEC  d,x       */ }, //  0XD6
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* CMP  [d],y     */ }, //  0XD7
    { "CLD",   0,  base, NULL, NULL, NULL     , Implied                               /* CLD  i         */ }, //  0XD8
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* CMP  a,y       */ }, //  0XD9
    { "PHX",   0,  base, NULL, NULL, NULL     , Implied                               /* PHX  s         */ }, //  0XDA
    { "STP",   0,  base, NULL, NULL, NULL     , Implied                               /* STP  i         */ }, //  0XDB
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | IndirectLong               /* JMP  [a]       */ }, //  0XDC
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* CMP  a,x       */ }, //  0XDD
    { "DEC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* DEC  a,x       */ }, //  0XDE
    { "CMP",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX               /* CMP  al,x      */ }, //  0XDF
    { "CPX",   1, x_set, NULL, NULL, READ_8   , Immediate                             /* CPX  #         */ }, //  0XE0
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX      /* SBC  (d,x)     */ }, //  0XE1
    { "SEP",   1,  base,  SEP, NULL, READ_8   , Immediate                             /* SEP  #         */ }, //  0XE2
    { "SBC",   1,  base, NULL, NULL, READ_8   , StackRelative                         /* SBC  d,s       */ }, //  0XE3
    { "CPX",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* CPX  d         */ }, //  0XE4
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* SBC  d         */ }, //  0XE5
    { "INC",   1,  base, NULL, NULL, READ_8   , DirectPage                            /* INC  d         */ }, //  0XE6
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong             /* SBC  [d]       */ }, //  0XE7
    { "INX",   0,  base, NULL, NULL, NULL     , Implied                               /* INX  i         */ }, //  0XE8
    { "SBC",   1, m_set, NULL, NULL, READ_8   , Immediate                             /* SBC  #         */ }, //  0XE9
    { "NOP",   0,  base, NULL, NULL, NULL     , Implied                               /* NOP  i         */ }, //  0XEA
    { "XBA",   0,  base, NULL, NULL, NULL     , Implied                               /* XBA  i         */ }, //  0XEB
    { "CPX",   2,  base, NULL, NULL, READ_16  , Absolute                              /* CPX  a         */ }, //  0XEC
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* SBC  a         */ }, //  0XED
    { "INC",   2,  base, NULL, NULL, READ_16  , Absolute                              /* INC  a         */ }, //  0XEE
    { "SBC",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                          /* SBC  al        */ }, //  0XEF
    { "BEQ",   1,  base, NULL,  BRA, READ_8   , PCRelative                            /* BEQ  r         */ }, //  0XF0
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY      /* SBC  (d),y     */ }, //  0XF1
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                 /* SBC  (d)       */ }, //  0XF2
    { "SBC",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY   /* SBC  (d,s),y   */ }, //  0XF3
    { "PEA",   2,  base, NULL, NULL, READ_16  , Absolute                              /* PEA  s         */ }, //  0XF4
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* SBC  d,x       */ }, //  0XF5
    { "INC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                 /* INC  d,x       */ }, //  0XF6
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY  /* SBC  [d],y     */ }, //  0XF7
    { "SED",   0,  base, NULL, NULL, NULL     , Implied                               /* SED  i         */ }, //  0XF8
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                   /* SBC  a,y       */ }, //  0XF9
    { "PLX",   0,  base, NULL, NULL, NULL     , Implied                               /* PLX  s         */ }, //  0XFA
    { "XCE",   0,  base,  XCE, NULL, NULL     , Implied                               /* XCE  i         */ }, //  0XFB
    { "JSR",   2,  base, NULL, NULL, READ_16  , Absolute | Indirect | IndexedX        /* JSR  (a,x)     */ }, //  0XFC
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* SBC  a,x       */ }, //  0XFD
    { "INC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                   /* INC  a,x       */ }, //  0XFE
    { "SBC",   3,  base, NULL, NULL, READ_16  , AbsoluteLong | IndexedX               /* SBC  al,x      */ }, //  0XFF
};
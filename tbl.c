#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "state.h"
#include "ops.h"
#include "processor.h"

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

Note: we can add a "physical start offset" to the disassembler state that can resolve this,
but what exists right now is "good enough"
*/
void JSR(uint32_t target_offset, uint32_t source_offset) {
    make_label(source_offset, target_offset, "SUBROUTINE");
}

void JSL(uint32_t target_offset, uint32_t source_offset) {
    make_label(source_offset, target_offset, "SUBROUTINE_LONG");
}

void JMP(uint32_t target_offset, uint32_t source_offset) {
    make_label(source_offset, target_offset, "JMP_LABEL");
}

// for PC Relative branching we actually start with PC being the next instruction
// past the branch, so adjust for that.
void BRL(uint32_t target_offset, uint32_t source_offset) {
    int32_t p = source_offset + (int8_t)target_offset;
    p += 3; // BRanch Long takes one byte for the instruction and 2 bytes for the
            // operand. Adjust to cover for that. 
    make_label(source_offset, p, "LOCAL_LONG");
}

void BRA(uint32_t target_offset, uint32_t source_offset) {
    int32_t p = source_offset + (int8_t)target_offset;
    p += 2; // Standard branches are 1 byte instruction, 1 byte PC Relative offset
            // adjust to account for the Program Counter being past the actual
            // branch instruction.
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
    { "BRK",   1,  base, NULL, NULL, READ_8   , Immediate                            , BRK           /* BRK  s         */ }, //   0X0
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , ORA_DP_I_IX   /* ORA  (d,x)     */ }, //   0X1
    { "COP",   1,  base, NULL, NULL, READ_8   , Immediate                            , COP           /* COP  #         */ }, //   0X2
    { "ORA",   1,  base, NULL, NULL, READ_8   , StackRelative                        , ORA_SR        /* ORA  d,s       */ }, //   0X3
    { "TSB",   1,  base, NULL, NULL, READ_8   , DirectPage                           , TSB_DP        /* TSB  d         */ }, //   0X4
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage                           , ORA_DP        /* ORA  d         */ }, //   0X5
    { "ASL",   1,  base, NULL, NULL, READ_8   , DirectPage                           , ASL_DP        /* ASL  d         */ }, //   0X6
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , ORA_DP_IL     /* ORA  [d]       */ }, //   0X7
    { "PHP",   0,  base, NULL, NULL, NULL     , Implied                              , PHP           /* PHP  s         */ }, //   0X8
    { "ORA",   1, m_set, NULL, NULL, READ_8_16, Immediate                            , ORA_IMM       /* ORA  #         */ }, //   0X9
    { "ASL",   0,  base, NULL, NULL, NULL     , Implied                              , ASL           /* ASL  A         */ }, //   0XA
    { "PHD",   0,  base, NULL, NULL, NULL     , Implied                              , PHD           /* PHD  s         */ }, //   0XB
    { "TSB",   2,  base, NULL, NULL, READ_16  , Absolute                             , TSB_ABS       /* TSB  a         */ }, //   0XC
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute                             , ORA_ABS       /* ORA  a         */ }, //   0XD
    { "ASL",   2,  base, NULL, NULL, READ_16  , Absolute                             , ASL_ABS       /* ASL  a         */ }, //   0XE
    { "ORA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , ORA_ABL       /* ORA  al        */ }, //   0XF
    { "BPL",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BPL_CB        /* BPL  r         */ }, //  0X10
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , ORA_DP_I_IY   /* ORA  (d),y     */ }, //  0X11
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , ORA_DP_I      /* ORA  (d)       */ }, //  0X12
    { "ORA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , ORA_SR_I_IY   /* ORA  (d,s),y   */ }, //  0X13
    { "TRB",   1,  base, NULL, NULL, READ_8   , DirectPage                           , TRB_DP        /* TRB  d         */ }, //  0X14
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , ORA_DP_IX     /* ORA  d,x       */ }, //  0X15
    { "ASL",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , ASL_DP_IX     /* ASL  d,x       */ }, //  0X16
    { "ORA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , ORA_DP_IL_IY  /* ORA  [d],y     */ }, //  0X17
    { "CLC",   0,  base,  CLC, NULL, NULL     , Implied                              , CLC_CB        /* CLC  i         */ }, //  0X18
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , ORA_ABS_IY    /* ORA  a,y       */ }, //  0X19
    { "INC",   0,  base, NULL, NULL, NULL     , Implied                              , INC           /* INC  A         */ }, //  0X1A
    { "TCS",   0,  base, NULL, NULL, NULL     , Implied                              , TCS           /* TCS  i         */ }, //  0X1B
    { "TRB",   2,  base, NULL, NULL, READ_16  , Absolute                             , TRB_ABS       /* TRB  a         */ }, //  0X1C
    { "ORA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , ORA_ABS_IX    /* ORA  a,x       */ }, //  0X1D
    { "ASL",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , ASL_ABS_IX    /* ASL  a,x       */ }, //  0X1E
    { "ORA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX              , ORA_ABL_IX    /* ORA  al,x      */ }, //  0X1F
    { "JSR",   2,  base, NULL,  JMP, READ_16  , Absolute                             , JSR_CB        /* JSR  a         */ }, //  0X20
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , AND_DP_IX     /* AND  (d,x)     */ }, //  0X21
    { "JSL",   3,  base, NULL,  JMP, READ_24  , AbsoluteLong                         , JSL_CB        /* JSL  al        */ }, //  0X22
    { "AND",   1,  base, NULL, NULL, READ_8   , StackRelative                        , AND_SR        /* AND  d,s       */ }, //  0X23
    { "BIT",   1,  base, NULL, NULL, READ_8   , DirectPage                           , BIT_DP        /* BIT  d         */ }, //  0X24
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage                           , AND_DP        /* AND  d         */ }, //  0X25
    { "ROL",   1,  base, NULL, NULL, READ_8   , DirectPage                           , ROL_DP        /* ROL  d         */ }, //  0X26
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , AND_DP_IL     /* AND  [d]       */ }, //  0X27
    { "PLP",   0,  base, NULL, NULL, NULL     , Implied                              , PLP           /* PLP  s         */ }, //  0X28
    { "AND",   1, m_set, NULL, NULL, READ_8_16, Immediate                            , AND_IMM       /* AND  #         */ }, //  0X29
    { "ROL",   0,  base, NULL, NULL, NULL     , Implied                              , ROL           /* ROL  A         */ }, //  0X2A
    { "PLD",   0,  base, NULL, NULL, NULL     , Implied                              , PLD           /* PLD  s         */ }, //  0X2B
    { "BIT",   2,  base, NULL, NULL, READ_16  , Absolute                             , BIT_ABS       /* BIT  a         */ }, //  0X2C
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute                             , AND_ABS       /* AND  a         */ }, //  0X2D
    { "ROL",   2,  base, NULL, NULL, READ_16  , Absolute                             , ROL_ABS       /* ROL  a         */ }, //  0X2E
    { "AND",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , AND_ABL       /* AND  al        */ }, //  0X2F
    { "BMI",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BMI_CB        /* BMI  R         */ }, //  0X30
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , AND_DP_I_IY   /* AND  (d),y     */ }, //  0X31
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , AND_DP_I      /* AND  (d)       */ }, //  0X32
    { "AND",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , AND_SR_I_IY   /* AND  (d,s),y   */ }, //  0X33
    { "BIT",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , BIT_DP_IX     /* BIT  d,x       */ }, //  0X34
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , AND_DP_IX     /* AND  d,x       */ }, //  0X35
    { "ROL",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , ROL_DP_IX     /* ROL  d,x       */ }, //  0X36
    { "AND",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , AND_DP_IL_IY  /* AND  [d],y     */ }, //  0X37
    { "SEC",   0,  base,  SEC, NULL, NULL     , Implied                              , SEC_CB        /* SEC  i         */ }, //  0X38
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , AND_ABS_IY    /* AND  a,y       */ }, //  0X39
    { "DEC",   0,  base, NULL, NULL, NULL     , Implied                              , DEC           /* DEC  A         */ }, //  0X3A
    { "TSC",   0,  base, NULL, NULL, NULL     , Implied                              , TSC           /* TSC  i         */ }, //  0X3B
    { "BIT",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , BIT_ABS_IX    /* BIT  a,x       */ }, //  0X3C
    { "AND",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , AND_ABS_IX    /* AND  a,x       */ }, //  0X3D
    { "ROL",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , ROL_ABS_IX    /* ROL  a,x       */ }, //  0X3E
    { "AND",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX              , AND_ABL_IX    /* AND  al,x      */ }, //  0X3F
    { "RTI",   0,  base, NULL, NULL, NULL     , Implied                              , RTI           /* RTI  s         */ }, //  0X40
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , EOR_DP_I_IX   /* EOR  (d,x)     */ }, //  0X41
    { "WDM",   1,  base, NULL, NULL, READ_8   , Implied                              , WDM           /* WDM  i         */ }, //  0X42
    { "EOR",   1,  base, NULL, NULL, READ_8   , StackRelative                        , EOR_SR        /* EOR  d,s       */ }, //  0X43
    { "MVP",   2,  base, NULL, NULL, READ_BMA , BlockMoveAddress                     , MVP           /* MVP  src,dst   */ }, //  0X44
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage                           , EOR_DP        /* EOR  d         */ }, //  0X45
    { "LSR",   1,  base, NULL, NULL, READ_8   , DirectPage                           , LSR_DP        /* LSR  d         */ }, //  0X46
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , EOR_DP_IL     /* EOR  [d]       */ }, //  0X47
    { "PHA",   0,  base, NULL, NULL, NULL     , Implied                              , PHA           /* PHA  s         */ }, //  0X48
    { "EOR",   1, m_set, NULL, NULL, READ_8_16, Immediate                            , EOR_IMM       /* EOR  #         */ }, //  0X49
    { "LSR",   0,  base, NULL, NULL, NULL     , Implied                              , LSR           /* LSR  A         */ }, //  0X4A
    { "PHK",   0,  base, NULL, NULL, NULL     , Implied                              , PHK           /* PHK  s         */ }, //  0X4B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute                             , JMP_CB        /* JMP  a         */ }, //  0X4C
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute                             , EOR_ABS       /* EOR  a         */ }, //  0X4D
    { "LSR",   2,  base, NULL, NULL, READ_16  , Absolute                             , LSR_ABS       /* LSR  a         */ }, //  0X4E
    { "EOR",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , EOR_ABL       /* EOR  al        */ }, //  0X4F
    { "BVC",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BVC_CB        /* BVC  r         */ }, //  0X50
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , EOR_DP_I_IY   /* EOR  (d),y     */ }, //  0X51
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , EOR_DP_I      /* EOR  (d)       */ }, //  0X52
    { "EOR",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , EOR_SR_I_IY   /* EOR  (d,s),y   */ }, //  0X53
    { "MVN",   2,  base, NULL, NULL, READ_BMA , BlockMoveAddress                     , MVN           /* MVN  src,dst   */ }, //  0X54
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , EOR_DP_IX     /* EOR  d,x       */ }, //  0X55
    { "LSR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , LSR_DP_IX     /* LSR  d,x       */ }, //  0X56
    { "EOR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , EOR_DP_IL_IY  /* EOR  [d],y     */ }, //  0X57
    { "CLI",   0,  base, NULL, NULL, NULL     , Implied                              , CLI           /* CLI  i         */ }, //  0X58
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , EOR_ABS_IY    /* EOR  a,y       */ }, //  0X59
    { "PHY",   0,  base, NULL, NULL, NULL     , Implied                              , PHY           /* PHY  s         */ }, //  0X5A
    { "TCD",   0,  base, NULL, NULL, NULL     , Implied                              , TCD           /* TCD  i         */ }, //  0X5B
    { "JMP",   3,  base, NULL,  JMP, READ_24  , AbsoluteLong                         , JMP_AL        /* JMP  al        */ }, //  0X5C
    { "EOR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , EOR_ABS_IX    /* EOR  a,x       */ }, //  0X5D
    { "LSR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , LSR_ABS_IX    /* LSR  a,x       */ }, //  0X5E
    { "EOR",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX              , EOR_AL_IX     /* EOR  al,x      */ }, //  0X5F
    { "RTS",   0,  base, NULL, NULL, NULL     , Implied                              , RTS           /* RTS  s         */ }, //  0X60
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , ADC_DP_I_IX   /* ADC  (dp,X)    */ }, //  0X61
    { "PER",   2,  base, NULL, NULL, READ_16  , PCRelativeLong                       , PER           /* PER  s         */ }, //  0X62
    { "ADC",   1,  base, NULL, NULL, READ_8   , StackRelative                        , ADC_SR        /* ADC  sr,S      */ }, //  0X63
    { "STZ",   1,  base, NULL, NULL, READ_8   , DirectPage                           , STZ           /* STZ  d         */ }, //  0X64
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage                           , ADC_DP        /* ADC  dp        */ }, //  0X65
    { "ROR",   1,  base, NULL, NULL, READ_8   , DirectPage                           , ROR_DP        /* ROR  d         */ }, //  0X66
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , ADC_DP_IL     /* ADC  [dp]      */ }, //  0X67
    { "PLA",   0,  base, NULL, NULL, NULL     , Implied                              , PLA           /* PLA  s         */ }, //  0X68
    { "ADC",   1, m_set, NULL, NULL, READ_8_16, Immediate                            , ADC_IMM       /* ADC  #         */ }, //  0X69
    { "ROR",   0,  base, NULL, NULL, NULL     , Implied                              , ROR           /* ROR  A         */ }, //  0X6A
    { "RTL",   0,  base, NULL, NULL, NULL     , Implied                              , RTL           /* RTL  s         */ }, //  0X6B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | Indirect                  , JMP_ABS_I     /* JMP  (a)       */ }, //  0X6C
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute                             , ADC_ABS       /* ADC  addr      */ }, //  0X6D
    { "ROR",   2,  base, NULL, NULL, READ_16  , Absolute                             , ROR_ABS       /* ROR  a         */ }, //  0X6E
    { "ADC",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , ADC_ABL       /* ADC  al        */ }, //  0X6F
    { "BVS",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BVS_PCR       /* BVS  r         */ }, //  0X70
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , ADC_DP_I_IY   /* ADC  (d),y     */ }, //  0X71
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , ADC_DP_I      /* ADC  (d)       */ }, //  0X72
    { "ADC",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , ADC_SR_I_IY   /* ADC  (sr,S),y  */ }, //  0X73
    { "STZ",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , STZ_DP_IX     /* STZ  d,x       */ }, //  0X74
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , ADC_DP_IX     /* ADC  d,x       */ }, //  0X75
    { "ROR",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , ROR_DP_IX     /* ROR  d,x       */ }, //  0X76
    { "ADC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , ADC_DP_IL_IY  /* ADC  [d],y     */ }, //  0X77
    { "SEI",   0,  base, NULL, NULL, NULL     , Implied                              , SEI           /* SEI  i         */ }, //  0X78
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , ADC_ABS_IY    /* ADC  a,y       */ }, //  0X79
    { "PLY",   0,  base, NULL, NULL, NULL     , Implied                              , PLY           /* PLY  s         */ }, //  0X7A
    { "TDC",   0,  base, NULL, NULL, NULL     , Implied                              , TDC           /* TDC  i         */ }, //  0X7B
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | Indirect | IndexedX       , JMP_ABS_I_IX  /* JMP  (a,x)     */ }, //  0X7C
    { "ADC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , ADC_ABS_IX    /* ADC  a,x       */ }, //  0X7D
    { "ROR",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , ROR_ABS_IX    /* ROR  a,x       */ }, //  0X7E
    { "ADC",   3,  base, NULL, NULL, READ_16  , AbsoluteLong | IndexedX              , ADC_AL_IX     /* ADC  al,x      */ }, //  0X7F
    { "BRA",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BRA_CB        /* BRA  r         */ }, //  0X80
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , STA_DP_I_IX   /* STA  (d,x)     */ }, //  0X81
    { "BRL",   2,  base, NULL,  BRL, READ_16  , PCRelativeLong                       , BRL_CB        /* BRL  rl        */ }, //  0X82
    { "STA",   1,  base, NULL, NULL, READ_8   , StackRelative                        , STA_SR        /* STA  d,s       */ }, //  0X83
    { "STY",   1,  base, NULL, NULL, READ_8   , DirectPage                           , STY_DP        /* STY  d         */ }, //  0X84
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage                           , STA_DP        /* STA  d         */ }, //  0X85
    { "STX",   1,  base, NULL, NULL, READ_8   , DirectPage                           , STX_DP        /* STX  d         */ }, //  0X86
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , STA_DP_IL     /* STA  [d]       */ }, //  0X87
    { "DEY",   0,  base, NULL, NULL, NULL     , Implied                              , DEY           /* DEY  i         */ }, //  0X88
    { "BIT",   1, m_set, NULL, NULL, READ_8_16, Immediate                            , BIT_IMM       /* BIT  #         */ }, //  0X89
    { "TXA",   0,  base, NULL, NULL, NULL     , Implied                              , TXA           /* TXA  i         */ }, //  0X8A
    { "PHB",   0,  base, NULL, NULL, NULL     , Implied                              , PHB           /* PHB  s         */ }, //  0X8B
    { "STY",   2,  base, NULL, NULL, READ_16  , Absolute                             , STY_ABS       /* STY  a         */ }, //  0X8C
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute                             , STA_ABS       /* STA  a         */ }, //  0X8D
    { "STX",   2,  base, NULL, NULL, READ_16  , Absolute                             , STX_ABS       /* STX  a         */ }, //  0X8E
    { "STA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , STA_ABL       /* STA  al        */ }, //  0X8F
    { "BCC",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BCC_CB        /* BCC  r         */ }, //  0X90
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , STA_DP_I_IY   /* STA  (d),y     */ }, //  0X91
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , STA_DP_I      /* STA  (d)       */ }, //  0X92
    { "STA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , STA_SR_I_IY   /* STA  (d,s),y   */ }, //  0X93
    { "STY",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , STY_DP_IX     /* STY  d,x       */ }, //  0X94
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , STA_DP_IX     /* STA  d,x       */ }, //  0X95
    { "STX",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedY                , STX_DP_IY     /* STX  d,y       */ }, //  0X96
    { "STA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , STA_DP_IL_IY  /* STA  [d],y     */ }, //  0X97
    { "TYA",   0,  base, NULL, NULL, NULL     , Implied                              , TYA           /* TYA  i         */ }, //  0X98
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , STA_ABS_IY    /* STA  a,y       */ }, //  0X99
    { "TXS",   0,  base, NULL, NULL, NULL     , Implied                              , TXS           /* TXS  i         */ }, //  0X9A
    { "TXY",   0,  base, NULL, NULL, NULL     , Implied                              , TXY           /* TXY  i         */ }, //  0X9B
    { "STZ",   2,  base, NULL, NULL, READ_16  , Absolute                             , STZ_ABS       /* STZ  a         */ }, //  0X9C
    { "STA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , STA_ABS_IX    /* STA  a,x       */ }, //  0X9D
    { "STZ",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , STZ_ABS_IX    /* STZ  a,x       */ }, //  0X9E
    { "STA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX              , STA_ABL_IX    /* STA  al,x      */ }, //  0X9F
    { "LDY",   1, x_set, NULL, NULL, READ_8_16, Immediate                            , LDY_IMM       /* LDY  #         */ }, //  0XA0
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , LDA_DP_I_IX   /* LDA  (d,x)     */ }, //  0XA1
    { "LDX",   1, x_set, NULL, NULL, READ_8_16, Immediate                            , LDX_IMM       /* LDX  #         */ }, //  0XA2
    { "LDA",   1,  base, NULL, NULL, READ_8   , StackRelative                        , LDA_SR        /* LDA  d,s       */ }, //  0XA3
    { "LDY",   1,  base, NULL, NULL, READ_8   , DirectPage                           , LDY_DP        /* LDY  d         */ }, //  0XA4
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage                           , LDA_DP        /* LDA  d         */ }, //  0XA5
    { "LDX",   1,  base, NULL, NULL, READ_8   , DirectPage                           , LDX_DP        /* LDX  d         */ }, //  0XA6
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , LDA_DP_IL     /* LDA  [d]       */ }, //  0XA7
    { "TAY",   0,  base, NULL, NULL, NULL     , Implied                              , TAY           /* TAY  i         */ }, //  0XA8
    { "LDA",   1, m_set, NULL, NULL, READ_8_16, Immediate                            , LDA_IMM       /* LDA  #         */ }, //  0XA9
    { "TAX",   0,  base, NULL, NULL, NULL     , Implied                              , TAX           /* TAX  i         */ }, //  0XAA
    { "PLB",   0,  base, NULL, NULL, NULL     , Implied                              , PLB           /* PLB  s         */ }, //  0XAB
    { "LDY",   2,  base, NULL, NULL, READ_16  , Absolute                             , LDY_ABS       /* LDY  a         */ }, //  0XAC
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute                             , LDA_ABS       /* LDA  a         */ }, //  0XAD
    { "LDX",   2,  base, NULL, NULL, READ_16  , Absolute                             , LDX_ABS       /* LDX  a         */ }, //  0XAE
    { "LDA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , LDA_ABL       /* LDA  al        */ }, //  0XAF
    { "BCS",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BCS_CB        /* BCS  r         */ }, //  0XB0
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , LDA_DP_I_IY   /* LDA  (d),y     */ }, //  0XB1
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , LDA_DP_I      /* LDA  (d)       */ }, //  0XB2
    { "LDA",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , LDA_SR_I_IY   /* LDA  (d,s),y   */ }, //  0XB3
    { "LDY",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , LDY_DP_IX     /* LDY  d,x       */ }, //  0XB4
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , LDA_DP_IX     /* LDA  d,x       */ }, //  0XB5
    { "LDX",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedY                , LDX_DP_IX     /* LDX  d,y       */ }, //  0XB6
    { "LDA",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , LDA_DP_IL_IY  /* LDA  [d],y     */ }, //  0XB7
    { "CLV",   0,  base, NULL, NULL, NULL     , Implied                              , CLV           /* CLV  i         */ }, //  0XB8
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , LDA_ABS_IY    /* LDA  a,y       */ }, //  0XB9
    { "TSX",   0,  base, NULL, NULL, NULL     , Implied                              , TSX           /* TSX  i         */ }, //  0XBA
    { "TYX",   0,  base, NULL, NULL, NULL     , Implied                              , TYX           /* TYX  i         */ }, //  0XBB
    { "LDY",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , LDY_ABS_IX    /* LDY  a,x       */ }, //  0XBC
    { "LDA",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , LDA_ABS_IX    /* LDA  a,x       */ }, //  0XBD
    { "LDX",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , LDX_ABS_IY    /* LDX  a,y       */ }, //  0XBE
    { "LDA",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX              , LDA_AL_IX     /* LDA  al,x      */ }, //  0XBF
    { "CPY",   1, x_set, NULL, NULL, READ_8   , Immediate                            , CPY_IMM       /* CPY  #         */ }, //  0XC0
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , CMP_DP_I_IX   /* CMP  (d,x)     */ }, //  0XC1
    { "REP",   1,  base,  REP, NULL, READ_8   , Immediate                            , REP_CB        /* REP  #         */ }, //  0XC2
    { "CMP",   1,  base, NULL, NULL, READ_8   , StackRelative                        , CMP_SR        /* CMP  d,s       */ }, //  0XC3
    { "CPY",   1,  base, NULL, NULL, READ_8   , DirectPage                           , CPY_DP        /* CPY  d         */ }, //  0XC4
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage                           , CMP_DP        /* CMP  d         */ }, //  0XC5
    { "DEC",   1,  base, NULL, NULL, READ_8   , DirectPage                           , DEC_DP        /* DEC  d         */ }, //  0XC6
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , CMP_DP_IL     /* CMP  [d]       */ }, //  0XC7
    { "INY",   0,  base, NULL, NULL, NULL     , Implied                              , INY           /* INY  i         */ }, //  0XC8
    { "CMP",   1, m_set, NULL, NULL, READ_8   , Immediate                            , CMP_IMM       /* CMP  #         */ }, //  0XC9
    { "DEX",   0,  base, NULL, NULL, NULL     , Implied                              , DEX           /* DEX  i         */ }, //  0XCA
    { "WAI",   0,  base, NULL, NULL, NULL     , Implied                              , WAI           /* WAI  i         */ }, //  0XCB
    { "CPY",   2,  base, NULL, NULL, READ_16  , Absolute                             , CPY_ABS       /* CPY  a         */ }, //  0XCC
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute                             , CMP_ABS       /* CMP  a         */ }, //  0XCD
    { "DEC",   2,  base, NULL, NULL, READ_16  , Absolute                             , DEC_ABS       /* DEC  a         */ }, //  0XCE
    { "CMP",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , CMP_ABL       /* CMP  al        */ }, //  0XCF
    { "BNE",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BNE_CB        /* BNE  r         */ }, //  0XD0
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , CMP_DP_I_IX   /* CMP  (d),y     */ }, //  0XD1
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , CMP_DP_I      /* CMP  (d)       */ }, //  0XD2
    { "CMP",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , CMP_SR_I_IY   /* CMP  (d,s),y   */ }, //  0XD3
    { "PEI",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , PEI_DP_I      /* PEI  (dp)      */ }, //  0XD4
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , CMP_DP_IX     /* CMP  d,x       */ }, //  0XD5
    { "DEC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , DEC_DP_IX     /* DEC  d,x       */ }, //  0XD6
    { "CMP",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , CMP_DP_IL_IY  /* CMP  [d],y     */ }, //  0XD7
    { "CLD",   0,  base, NULL, NULL, NULL     , Implied                              , CLD_CB        /* CLD  i         */ }, //  0XD8
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , CMP_ABS_IY    /* CMP  a,y       */ }, //  0XD9
    { "PHX",   0,  base, NULL, NULL, NULL     , Implied                              , PHX           /* PHX  s         */ }, //  0XDA
    { "STP",   0,  base, NULL, NULL, NULL     , Implied                              , STP           /* STP  i         */ }, //  0XDB
    { "JMP",   2,  base, NULL,  JMP, READ_16  , Absolute | IndirectLong              , JMP_ABS_IL    /* JMP  [a]       */ }, //  0XDC
    { "CMP",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , CMP_ABS_IX    /* CMP  a,x       */ }, //  0XDD
    { "DEC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , DEC_ABS_IX    /* DEC  a,x       */ }, //  0XDE
    { "CMP",   3,  base, NULL, NULL, READ_24  , AbsoluteLong | IndexedX              , CMP_ABL_IX    /* CMP  al,x      */ }, //  0XDF
    { "CPX",   1, x_set, NULL, NULL, READ_8   , Immediate                            , CPX_IMM       /* CPX  #         */ }, //  0XE0
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedX     , SBC_DP_I_IX   /* SBC  (d,x)     */ }, //  0XE1
    { "SEP",   1,  base,  SEP, NULL, READ_8   , Immediate                            , SEP_CB        /* SEP  #         */ }, //  0XE2
    { "SBC",   1,  base, NULL, NULL, READ_8   , StackRelative                        , SBC_SR        /* SBC  d,s       */ }, //  0XE3
    { "CPX",   1,  base, NULL, NULL, READ_8   , DirectPage                           , CPX_DP        /* CPX  d         */ }, //  0XE4
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage                           , SBC_DP        /* SBC  d         */ }, //  0XE5
    { "INC",   1,  base, NULL, NULL, READ_8   , DirectPage                           , INC_DP        /* INC  d         */ }, //  0XE6
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong            , SBC_DP_IL     /* SBC  [d]       */ }, //  0XE7
    { "INX",   0,  base, NULL, NULL, NULL     , Implied                              , INX           /* INX  i         */ }, //  0XE8
    { "SBC",   1, m_set, NULL, NULL, READ_8   , Immediate                            , SBC_IMM       /* SBC  #         */ }, //  0XE9
    { "NOP",   0,  base, NULL, NULL, NULL     , Implied                              , NOP           /* NOP  i         */ }, //  0XEA
    { "XBA",   0,  base, NULL, NULL, NULL     , Implied                              , XBA           /* XBA  i         */ }, //  0XEB
    { "CPX",   2,  base, NULL, NULL, READ_16  , Absolute                             , CPX_ABS       /* CPX  a         */ }, //  0XEC
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute                             , SBC_ABS       /* SBC  a         */ }, //  0XED
    { "INC",   2,  base, NULL, NULL, READ_16  , Absolute                             , INC_ABS       /* INC  a         */ }, //  0XEE
    { "SBC",   3,  base, NULL, NULL, READ_24  , AbsoluteLong                         , SBC_ABL       /* SBC  al        */ }, //  0XEF
    { "BEQ",   1,  base, NULL,  BRA, READ_8   , PCRelative                           , BEQ_CB        /* BEQ  r         */ }, //  0XF0
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect | IndexedY     , SBC_DP_I_IY   /* SBC  (d),y     */ }, //  0XF1
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | Indirect                , SBC_DP_I      /* SBC  (d)       */ }, //  0XF2
    { "SBC",   1,  base, NULL, NULL, READ_8   , StackRelative | Indirect | IndexedY  , SBC_SR_I_IY   /* SBC  (d,s),y   */ }, //  0XF3
    { "PEA",   2,  base, NULL, NULL, READ_16  , Absolute                             , PEA_ABS       /* PEA  s         */ }, //  0XF4
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , SBC_DP_IX     /* SBC  d,x       */ }, //  0XF5
    { "INC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndexedX                , INC_DP_IX     /* INC  d,x       */ }, //  0XF6
    { "SBC",   1,  base, NULL, NULL, READ_8   , DirectPage | IndirectLong | IndexedY , SBC_DP_IL_IY  /* SBC  [d],y     */ }, //  0XF7
    { "SED",   0,  base, NULL, NULL, NULL     , Implied                              , SED           /* SED  i         */ }, //  0XF8
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedY                  , SBC_ABS_IY    /* SBC  a,y       */ }, //  0XF9
    { "PLX",   0,  base, NULL, NULL, NULL     , Implied                              , PLX           /* PLX  s         */ }, //  0XFA
    { "XCE",   0,  base,  XCE, NULL, NULL     , Implied                              , XCE_CB        /* XCE  i         */ }, //  0XFB
    { "JSR",   2,  base, NULL, NULL, READ_16  , Absolute | Indirect | IndexedX       , JSR_ABS_I_IX  /* JSR  (a,x)     */ }, //  0XFC
    { "SBC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , SBC_ABS_IX    /* SBC  a,x       */ }, //  0XFD
    { "INC",   2,  base, NULL, NULL, READ_16  , Absolute | IndexedX                  , INC_ABS_IX    /* INC  a,x       */ }, //  0XFE
    { "SBC",   3,  base, NULL, NULL, READ_16  , AbsoluteLong | IndexedX              , SBC_ABL_IX    /* SBC  al,x      */ }, //  0XFF
};
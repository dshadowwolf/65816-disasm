#include "Opcode.hpp"
#include "Flags.hpp"
#include "State.hpp"

extern void JMP(unsigned int);
extern void BRA(unsigned int);
extern void BRL(unsigned int);
extern void JSL(unsigned int);
extern void JSR(unsigned int);

const Opcode opcodes[] = {
Opcode( "BRK",  1, Immediate                                         , READ_8   , nullptr, nullptr, false , false  ), // 0x00  -- BRK s
Opcode( "ORA",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0x01  -- ORA (d,x)
Opcode( "COP",  1, Immediate                                         , READ_8   , nullptr, nullptr, false , false  ), // 0x02  -- COP #
Opcode( "ORA",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0x03  -- ORA d,s
Opcode( "TSB",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x04  -- TSB d
Opcode( "ORA",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x05  -- ORA d
Opcode( "ASL",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x06  -- ASL d
Opcode( "ORA",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0x07  -- ORA [d]
Opcode( "PHP",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x08  -- PHP s
Opcode( "ORA",  1, Immediate                                         , READ_8_16, nullptr, nullptr, true  , false  ), // 0x09  -- ORA #
Opcode( "ASL",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x0A  -- ASL A
Opcode( "PHD",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x0B  -- PHD s
Opcode( "TSB",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x0C  -- TSB a
Opcode( "ORA",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x0D  -- ORA a
Opcode( "ASL",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x0E  -- ASL a
Opcode( "ORA",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0x0F  -- ORA al
Opcode( "BPL",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0x10  -- BPL r
Opcode( "ORA",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0x11  -- ORA (d),y
Opcode( "ORA",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0x12  -- ORA (d)
Opcode( "ORA",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0x13  -- ORA (d,s),y
Opcode( "TRB",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x14  -- TRB d
Opcode( "ORA",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x15  -- ORA d,x
Opcode( "ASL",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x16  -- ASL d,x
Opcode( "ORA",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0x17  -- ORA [d],y
Opcode( "CLC",  0, Implied                                           , NONE     , CLC    , nullptr, false , false  ), // 0x18  -- CLC i
Opcode( "ORA",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0x19  -- ORA a,y
Opcode( "INC",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x1A  -- INC A
Opcode( "TCS",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x1B  -- TCS i
Opcode( "TRB",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x1C  -- TRB a
Opcode( "ORA",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x1D  -- ORA a,x
Opcode( "ASL",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x1E  -- ASL a,x
Opcode( "ORA",  3, AbsoluteLong | IndexedX                           , READ_24  , nullptr, nullptr, false , false  ), // 0x1F  -- ORA al,x
Opcode( "JSR",  2, Absolute                                          , READ_16  , nullptr, JMP    , false , false  ), // 0x20  -- JSR a
Opcode( "AND",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x21  -- AND (d,x)
Opcode( "JSL",  3, AbsoluteLong                                      , READ_24  , nullptr, JMP    , false , false  ), // 0x22  -- JSL al
Opcode( "AND",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0x23  -- AND d,s
Opcode( "BIT",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x24  -- BIT d
Opcode( "AND",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x25  -- AND d
Opcode( "ROL",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x26  -- ROL d
Opcode( "AND",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0x27  -- AND [d]
Opcode( "PLP",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x28  -- PLP s
Opcode( "AND",  1, Immediate                                         , READ_8_16, nullptr, nullptr, true  , false  ), // 0x29  -- AND #
Opcode( "ROL",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x2A  -- ROL A
Opcode( "PLD",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x2B  -- PLD s
Opcode( "BIT",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x2C  -- BIT a
Opcode( "AND",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x2D  -- AND a
Opcode( "ROL",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x2E  -- ROL a
Opcode( "AND",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0x2F  -- AND al
Opcode( "BMI",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0x30  -- BMI R
Opcode( "AND",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0x31  -- AND (d),y
Opcode( "AND",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0x32  -- AND (d)
Opcode( "AND",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0x33  -- AND (d,s),y
Opcode( "BIT",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x34  -- BIT d,x
Opcode( "AND",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x35  -- AND d,x
Opcode( "ROL",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x36  -- ROL d,x
Opcode( "AND",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0x37  -- AND [d],y
Opcode( "SEC",  0, Implied                                           , NONE     , SEC    , nullptr, false , false  ), // 0x38  -- SEC i
Opcode( "AND",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0x39  -- AND a,y
Opcode( "DEC",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x3A  -- DEC A
Opcode( "TSC",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x3B  -- TSC i
Opcode( "BIT",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x3C  -- BIT a,x
Opcode( "AND",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x3D  -- AND a,x
Opcode( "ROL",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x3E  -- ROL a,x
Opcode( "AND",  3, Absolute | IndirectLong | IndexedX                , READ_24  , nullptr, nullptr, false , false  ), // 0x3F  -- AND al,x
Opcode( "RTI",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x40  -- RTI s
Opcode( "EOR",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0x41  -- EOR (d,x)
Opcode( "WDM",  1, Implied                                           , READ_8   , nullptr, nullptr, false , false  ), // 0x42  -- WDM i
Opcode( "EOR",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0x43  -- EOR d,s
Opcode( "MVP",  2, BlockMoveAddress                                  , READ_BMA , nullptr, nullptr, false , false  ), // 0x44  -- MVP src,dst
Opcode( "EOR",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x45  -- EOR d
Opcode( "LSR",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x46  -- LSR d
Opcode( "EOR",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0x47  -- EOR [d]
Opcode( "PHA",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x48  -- PHA s
Opcode( "EOR",  1, Immediate                                         , READ_8_16, nullptr, nullptr, true  , false  ), // 0x49  -- EOR #
Opcode( "LSR",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x4A  -- LSR A
Opcode( "PHK",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x4B  -- PHK s
Opcode( "JMP",  2, Absolute                                          , READ_16  , nullptr, JMP    , false , false  ), // 0x4C  -- JMP a
Opcode( "EOR",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x4D  -- EOR a
Opcode( "LSR",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x4E  -- LSR a
Opcode( "EOR",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0x4F  -- EOR al
Opcode( "BVC",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0x50  -- BVC r
Opcode( "EOR",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0x51  -- EOR (d),y
Opcode( "EOR",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0x52  -- EOR (d)
Opcode( "EOR",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0x53  -- EOR (d,s),y
Opcode( "MVN",  2, BlockMoveAddress                                  , READ_BMA , nullptr, nullptr, false , false  ), // 0x54  -- MVN src,dst
Opcode( "EOR",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x55  -- EOR d,x
Opcode( "LSR",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x56  -- LSR d,x
Opcode( "EOR",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0x57  -- EOR [d],y
Opcode( "CLI",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x58  -- CLI i
Opcode( "EOR",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0x59  -- EOR a,y
Opcode( "PHY",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x5A  -- PHY s
Opcode( "TCD",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x5B  -- TCD i
Opcode( "JMP",  3, AbsoluteLong                                      , READ_24  , nullptr, JMP    , false , false  ), // 0x5C  -- JMP al
Opcode( "EOR",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x5D  -- EOR a,x
Opcode( "LSR",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x5E  -- LSR a,x
Opcode( "EOR",  3, AbsoluteLong | IndexedX                           , READ_24  , nullptr, nullptr, false , false  ), // 0x5F  -- EOR al,x
Opcode( "RTS",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x60  -- RTS s
Opcode( "ADC",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0x61  -- ADC (dp,X)
Opcode( "PER",  2, PCRelativeLong                                    , READ_16  , nullptr, nullptr, false , false  ), // 0x62  -- PER s
Opcode( "ADC",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0x63  -- ADC sr,S
Opcode( "STZ",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x64  -- STZ d
Opcode( "ADC",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x65  -- ADC dp
Opcode( "ROR",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x66  -- ROR d
Opcode( "ADC",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0x67  -- ADC [dp]
Opcode( "PLA",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x68  -- PLA s
Opcode( "ADC",  1, Immediate                                         , READ_8_16, nullptr, nullptr, true  , false  ), // 0x69  -- ADC #
Opcode( "ROR",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x6A  -- ROR A
Opcode( "RTL",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x6B  -- RTL s
Opcode( "JMP",  2, Absolute | Indirect                               , READ_16  , nullptr, JMP    , false , false  ), // 0x6C  -- JMP (a)
Opcode( "ADC",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x6D  -- ADC addr
Opcode( "ROR",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x6E  -- ROR a
Opcode( "ADC",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0x6F  -- ADC al
Opcode( "BVS",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0x70  -- BVS r
Opcode( "ADC",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0x71  -- ADC (d),y
Opcode( "ADC",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0x72  -- ADC (d)
Opcode( "ADC",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0x73  -- ADC (sr,S),y
Opcode( "STZ",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x74  -- STZ d,x
Opcode( "ADC",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x75  -- ADC d,x
Opcode( "ROR",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x76  -- ROR d,x
Opcode( "ADC",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0x77  -- ADC [d],y
Opcode( "SEI",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x78  -- SEI i
Opcode( "ADC",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0x79  -- ADC a,y
Opcode( "PLY",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x7A  -- PLY s
Opcode( "TDC",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x7B  -- TDC i
Opcode( "JMP",  2, Absolute | Indirect | IndexedX                    , READ_16  , nullptr, JMP    , false , false  ), // 0x7C  -- JMP (a,x)
Opcode( "ADC",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x7D  -- ADC a,x
Opcode( "ROR",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x7E  -- ROR a,x
Opcode( "ADC",  3, AbsoluteLong | IndexedX                           , READ_16  , nullptr, nullptr, false , false  ), // 0x7F  -- ADC al,x
Opcode( "BRA",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0x80  -- BRA r
Opcode( "STA",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0x81  -- STA (d,x)
Opcode( "BRL",  2, PCRelativeLong                                    , READ_16  , nullptr, BRL    , false , false  ), // 0x82  -- BRL rl
Opcode( "STA",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0x83  -- STA d,s
Opcode( "STY",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x84  -- STY d
Opcode( "STA",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x85  -- STA d
Opcode( "STX",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0x86  -- STX d
Opcode( "STA",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0x87  -- STA [d]
Opcode( "DEY",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x88  -- DEY i
Opcode( "BIT",  1, Immediate                                         , READ_8_16, nullptr, nullptr, true  , false  ), // 0x89  -- BIT #
Opcode( "TXA",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x8A  -- TXA i
Opcode( "PHB",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x8B  -- PHB s
Opcode( "STY",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x8C  -- STY a
Opcode( "STA",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x8D  -- STA a
Opcode( "STX",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x8E  -- STX a
Opcode( "STA",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0x8F  -- STA al
Opcode( "BCC",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0x90  -- BCC r
Opcode( "STA",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0x91  -- STA (d),y
Opcode( "STA",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0x92  -- STA (d)
Opcode( "STA",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0x93  -- STA (d,s),y
Opcode( "STY",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x94  -- STY d,x
Opcode( "STA",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0x95  -- STA d,x
Opcode( "STX",  1, DirectPage | IndexedY                             , READ_8   , nullptr, nullptr, false , false  ), // 0x96  -- STX d,y
Opcode( "STA",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0x97  -- STA [d],y
Opcode( "TYA",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x98  -- TYA i
Opcode( "STA",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0x99  -- STA a,y
Opcode( "TXS",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x9A  -- TXS i
Opcode( "TXY",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0x9B  -- TXY i
Opcode( "STZ",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0x9C  -- STZ a
Opcode( "STA",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x9D  -- STA a,x
Opcode( "STZ",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0x9E  -- STZ a,x
Opcode( "STA",  3, AbsoluteLong | IndexedX                           , READ_24  , nullptr, nullptr, false , false  ), // 0x9F  -- STA al,x
Opcode( "LDY",  1, Immediate                                         , READ_8_16, nullptr, nullptr, false , true   ), // 0xA0  -- LDY #
Opcode( "LDA",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0xA1  -- LDA (d,x)
Opcode( "LDX",  1, Immediate                                         , READ_8_16, nullptr, nullptr, false , true   ), // 0xA2  -- LDX #
Opcode( "LDA",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0xA3  -- LDA d,s
Opcode( "LDY",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xA4  -- LDY d
Opcode( "LDA",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xA5  -- LDA d
Opcode( "LDX",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xA6  -- LDX d
Opcode( "LDA",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0xA7  -- LDA [d]
Opcode( "TAY",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xA8  -- TAY i
Opcode( "LDA",  1, Immediate                                         , READ_8_16, nullptr, nullptr, true  , false  ), // 0xA9  -- LDA #
Opcode( "TAX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xAA  -- TAX i
Opcode( "PLB",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xAB  -- PLB s
Opcode( "LDY",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xAC  -- LDY a
Opcode( "LDA",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xAD  -- LDA a
Opcode( "LDX",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xAE  -- LDX a
Opcode( "LDA",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0xAF  -- LDA al
Opcode( "BCS",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0xB0  -- BCS r
Opcode( "LDA",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0xB1  -- LDA (d),y
Opcode( "LDA",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0xB2  -- LDA (d)
Opcode( "LDA",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0xB3  -- LDA (d,s),y
Opcode( "LDY",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0xB4  -- LDY d,x
Opcode( "LDA",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0xB5  -- LDA d,x
Opcode( "LDX",  1, DirectPage | IndexedY                             , READ_8   , nullptr, nullptr, false , false  ), // 0xB6  -- LDX d,y
Opcode( "LDA",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0xB7  -- LDA [d],y
Opcode( "CLV",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xB8  -- CLV i
Opcode( "LDA",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0xB9  -- LDA a,y
Opcode( "TSX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xBA  -- TSX i
Opcode( "TYX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xBB  -- TYX i
Opcode( "LDY",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0xBC  -- LDY a,x
Opcode( "LDA",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0xBD  -- LDA a,x
Opcode( "LDX",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0xBE  -- LDX a,y
Opcode( "LDA",  3, AbsoluteLong | IndexedX                           , READ_24  , nullptr, nullptr, false , false  ), // 0xBF  -- LDA al,x
Opcode( "CPY",  1, Immediate                                         , READ_8   , nullptr, nullptr, false , true   ), // 0xC0  -- CPY #
Opcode( "CMP",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0xC1  -- CMP (d,x)
Opcode( "REP",  1, Immediate                                         , READ_8   , REP    , nullptr, false , false  ), // 0xC2  -- REP #
Opcode( "CMP",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0xC3  -- CMP d,s
Opcode( "CPY",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xC4  -- CPY d
Opcode( "CMP",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xC5  -- CMP d
Opcode( "DEC",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xC6  -- DEC d
Opcode( "CMP",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0xC7  -- CMP [d]
Opcode( "INY",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xC8  -- INY i
Opcode( "CMP",  1, Immediate                                         , READ_8   , nullptr, nullptr, true  , false  ), // 0xC9  -- CMP #
Opcode( "DEX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xCA  -- DEX i
Opcode( "WAI",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xCB  -- WAI i
Opcode( "CPY",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xCC  -- CPY a
Opcode( "CMP",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xCD  -- CMP a
Opcode( "DEC",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xCE  -- DEC a
Opcode( "CMP",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0xCF  -- CMP al
Opcode( "BNE",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0xD0  -- BNE r
Opcode( "CMP",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0xD1  -- CMP (d),y
Opcode( "CMP",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0xD2  -- CMP (d)
Opcode( "CMP",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0xD3  -- CMP (d,s),y
Opcode( "PEI",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0xD4  -- PEI (dp)
Opcode( "CMP",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0xD5  -- CMP d,x
Opcode( "DEC",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0xD6  -- DEC d,x
Opcode( "CMP",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0xD7  -- CMP [d],y
Opcode( "CLD",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xD8  -- CLD i
Opcode( "CMP",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0xD9  -- CMP a,y
Opcode( "PHX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xDA  -- PHX s
Opcode( "STP",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xDB  -- STP i
Opcode( "JMP",  2, Absolute | IndirectLong                           , READ_16  , nullptr, JMP    , false , false  ), // 0xDC  -- JMP [a]
Opcode( "CMP",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0xDD  -- CMP a,x
Opcode( "DEC",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0xDE  -- DEC a,x
Opcode( "CMP",  3, AbsoluteLong | IndexedX                           , READ_24  , nullptr, nullptr, false , false  ), // 0xDF  -- CMP al,x
Opcode( "CPX",  1, Immediate                                         , READ_8   , nullptr, nullptr, false , true   ), // 0xE0  -- CPX #
Opcode( "SBC",  1, DirectPage | Indirect | IndexedX                  , READ_8   , nullptr, nullptr, false , false  ), // 0xE1  -- SBC (d,x)
Opcode( "SEP",  1, Immediate                                         , READ_8   , SEP    , nullptr, false , false  ), // 0xE2  -- SEP #
Opcode( "SBC",  1, StackRelative                                     , READ_8   , nullptr, nullptr, false , false  ), // 0xE3  -- SBC d,s
Opcode( "CPX",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xE4  -- CPX d
Opcode( "SBC",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xE5  -- SBC d
Opcode( "INC",  1, DirectPage                                        , READ_8   , nullptr, nullptr, false , false  ), // 0xE6  -- INC d
Opcode( "SBC",  1, DirectPage | IndirectLong                         , READ_8   , nullptr, nullptr, false , false  ), // 0xE7  -- SBC [d]
Opcode( "INX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xE8  -- INX i
Opcode( "SBC",  1, Immediate                                         , READ_8   , nullptr, nullptr, true  , false  ), // 0xE9  -- SBC #
Opcode( "NOP",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xEA  -- NOP i
Opcode( "XBA",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xEB  -- XBA i
Opcode( "CPX",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xEC  -- CPX a
Opcode( "SBC",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xED  -- SBC a
Opcode( "INC",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xEE  -- INC a
Opcode( "SBC",  3, AbsoluteLong                                      , READ_24  , nullptr, nullptr, false , false  ), // 0xEF  -- SBC al
Opcode( "BEQ",  1, PCRelative                                        , READ_8   , nullptr, BRA    , false , false  ), // 0xF0  -- BEQ r
Opcode( "SBC",  1, DirectPage | Indirect | IndexedY                  , READ_8   , nullptr, nullptr, false , false  ), // 0xF1  -- SBC (d),y
Opcode( "SBC",  1, DirectPage | Indirect                             , READ_8   , nullptr, nullptr, false , false  ), // 0xF2  -- SBC (d)
Opcode( "SBC",  1, StackRelative | Indirect | IndexedY               , READ_8   , nullptr, nullptr, false , false  ), // 0xF3  -- SBC (d,s),y
Opcode( "PEA",  2, Absolute                                          , READ_16  , nullptr, nullptr, false , false  ), // 0xF4  -- PEA s
Opcode( "SBC",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0xF5  -- SBC d,x
Opcode( "INC",  1, DirectPage | IndexedX                             , READ_8   , nullptr, nullptr, false , false  ), // 0xF6  -- INC d,x
Opcode( "SBC",  1, DirectPage | IndirectLong | IndexedY              , READ_8   , nullptr, nullptr, false , false  ), // 0xF7  -- SBC [d],y
Opcode( "SED",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xF8  -- SED i
Opcode( "SBC",  2, Absolute | IndexedY                               , READ_16  , nullptr, nullptr, false , false  ), // 0xF9  -- SBC a,y
Opcode( "PLX",  0, Implied                                           , NONE     , nullptr, nullptr, false , false  ), // 0xFA  -- PLX s
Opcode( "XCE",  0, Implied                                           , NONE     , XCE    , nullptr, false , false  ), // 0xFB  -- XCE i
Opcode( "JSR",  2, Absolute | Indirect | IndexedX                    , READ_16  , nullptr, nullptr, false , false  ), // 0xFC  -- JSR (a,x)
Opcode( "SBC",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0xFD  -- SBC a,x
Opcode( "INC",  2, Absolute | IndexedX                               , READ_16  , nullptr, nullptr, false , false  ), // 0xFE  -- INC a,x
Opcode( "SBC",  3, AbsoluteLong | IndexedX                           , READ_16  , nullptr, nullptr, false , false  ), // 0xFF  -- SBC al,x
};

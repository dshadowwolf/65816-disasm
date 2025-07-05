#define X_FLAG 0x01  // index register size        -- SEP 0x10  -- if E == 1 then this is the `Break` bit
#define M_FLAG 0x02  // accumulator register size  -- SEP 0x20  -- if E == 1 then this is unused
#define E_FLAG 0x04  // 6502 emulation mode flag   -- XCE
#define CARRY  0x08  // Carry Flag -- needed for tracking what the XCE instruction does

void init();
bool isMSet();
bool isXSet();
bool isESet();
bool carrySet();
void REP(unsigned char x);
void SEP(unsigned char x);
void SEC(unsigned char unused);
void CLC(unsigned char unused);
void XCE(unsigned char unused);
void set_state(unsigned char x);
unsigned char get_state();
unsigned int get_start_offset();
void set_start_offset(unsigned int x);

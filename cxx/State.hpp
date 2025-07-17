#ifndef __STATE_HPP__
#define __STATE_HPP__

class State {
    public:
        State() : mFlag(false), xFlag(false), eFlag(false) {}
        bool isMSet() const { return mFlag&&eFlag; }
        bool isXSet() const { return xFlag&&eFlag; }
        bool isESet() const { return eFlag; }
        bool isCarrySet() const { return carryFlag; }
        void setMFlag(bool value) { mFlag = value; }
        void setXFlag(bool value) { xFlag = value; }
        void setEFlag(bool value) { eFlag = value; }
        void setCarryFlag(bool value) { carryFlag = value; }
        void setFlag(unsigned char flag) {
            switch(flag) {
                case 0x10: setXFlag(true); break; // X Flag
                case 0x20: setMFlag(true); break; // M Flag
                default: break; // Unknown flag, do nothing
            }
        }

        void clearFlag(unsigned char flag) {
            switch(flag) {
                case 0x10: setXFlag(false); break; // X Flag
                case 0x20: setMFlag(false); break; // M Flag
                default: break; // Unknown flag, do nothing
            }
        }

        protected:
            bool mFlag; // accuMulator Flag
            bool xFlag; // indeX Flag
            bool eFlag; // Emulation Flag
            bool carryFlag; // Carry Flag
};

static State state = State();

void SEP(unsigned char x) {
    state.clearFlag(x);
}

void REP(unsigned char x) {
    state.setFlag(x);
}

void CLC(unsigned char x) {
    state.setCarryFlag(false);
}

void SEC(unsigned char x) {
    state.setCarryFlag(true);
}

void XCE(unsigned char x) {
    if (state.isCarrySet() && !state.isESet()) {
        state.setEFlag(true);
    } else if (!state.isCarrySet() && state.isESet()) {
        state.setEFlag(false);
    }
}

#endif // __STATE_HPP__
#ifndef __OPCODE_HPP__
#define __OPCODE_HPP__  
#include <string>
#include "State.hpp"
#include "Flags.hpp"
#include "Data.hpp"

class Opcode {
public:
    Opcode(const char* name, int size, unsigned int flags, unsigned int read, void (*stateHandler)(unsigned char), void (*labelHandler)(unsigned int), bool mFlagUse, bool xFlagUse)
        : name(name), size(size), flags(flags), readval(read), stateHandler(stateHandler), labelHandler(labelHandler),
          mFlagUse(mFlagUse), xFlagUse(xFlagUse) {}

    int munge() {
        if (mFlagUse) {
            return state.isMSet()?size+1:size;
        }
    }

    signed int read(Data* data) {
        switch(readval) {
        case NONE:
            return 0;
        case READ_8:
            return *(signed char*)data->read_8();
        case READ_16:
            return *(unsigned int*)data->read_16();
        case READ_24:
            return *(unsigned int*)data->read_24();
        case READ_8_16:
            return *(signed int*)data->read_8_16(mFlagUse, xFlagUse);
        case READ_BMA:
            return *(unsigned short*)data->read_bma();
        default:
            return 0; // No read operation}
        }
    }
    const std::string& getName() const { return name; }
    const int getSize() { return munge(); }
    unsigned int getFlags() const { return flags; }
    unsigned char getReadVal() const { return readval; }
    void (*getStateHandler())(unsigned char) { return stateHandler; }
    void (*getLabelHandler())(unsigned int) { return labelHandler; }
    bool isMFlagUsed() const { return mFlagUse; }
    bool isXFlagUsed() const { return xFlagUse; }
    void setStateHandler(void (*handler)(unsigned char)) { stateHandler = handler; }
    void setLabelHandler(void (*handler)(unsigned int)) { labelHandler = handler; }
    void setReadVal(unsigned char read) { readval = read; }
    void setFlags(unsigned int newFlags) { flags = newFlags; }
    void setName(const std::string& newName) { name = newName; }
    void setSize(int newSize) { size = newSize; }
    void setMFlagUse(bool use) { mFlagUse = use; }
    void setXFlagUse(bool use) { xFlagUse = use; }
    bool isImplied() const { return flags & Implied; }
    bool isDirectPage() const { return flags & DirectPage; }
    bool isImmediate() const { return flags & Immediate; }
    bool isIndirect() const { return flags & Indirect; }
    bool isIndexedX() const { return flags & IndexedX; }
    bool isIndexedY() const { return flags & IndexedY; }
    bool isAbsolute() const { return flags & Absolute; }
    bool isAbsoluteLong() const { return flags & AbsoluteLong; }
    bool isIndexedLong() const { return flags & IndexedLong; }
    bool isPCRelative() const { return flags & PCRelative; }
    bool isStackRelative() const { return flags & StackRelative; }
    bool isPCRelativeLong() const { return flags & PCRelativeLong; }
    bool isBlockMoveAddress() const { return flags & BlockMoveAddress; }
    bool isIndirectLong() const { return flags & IndirectLong; }

protected:
    std::string name;
    int size;
    unsigned int flags;
    unsigned int readval;
    void (*stateHandler)(unsigned char);
    void (*labelHandler)(unsigned int);
    bool mFlagUse;
    bool xFlagUse;
};

#endif // __OPCODE_HPP__


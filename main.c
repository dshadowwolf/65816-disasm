#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "state.h"

extern void disasm(char *filename);

int main() {
    // this is a quick and dirty test to see if this thing works
    char *filename = "testfile.bin"; // replace with your test file
    set_state(0); // clear the state entirely
    set_start_offset(0xF818);
    disasm(filename);
    return 0;
}
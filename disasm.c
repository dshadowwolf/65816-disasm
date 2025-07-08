#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

//#include "ops.h"
#include "map.h"
#include "codetable.h"
#include "list.h"
#include "state.h"

extern const opcode_t opcodes[256];
typedef struct rval_s {
    uint32_t code;
    uint32_t handle;
    void* data;
    void* mark;
} rval_t;

rval_t* open_file(const char* filename) {
    rval_t* rets = malloc(sizeof(rval_t));
    int err = open(filename, O_EXCL|O_RDONLY);

    if ( err < 0 ) {
        perror("open_file");
        rets->code = errno;
        return rets;
    }

    rets->code = 0;
    rets->handle = err;

    return rets;
}

int64_t get_filesize(int filehandle) {
    off_t seeker = lseek(filehandle, 0, SEEK_END);

    if (lseek < 0) {
        perror("get_filesize");
        return -1;
    }

    return seeker;
}

rval_t* map_file(const char* filename) {
    rval_t *r = open_file(filename);

    if (r->code >= 0 && r->handle == 0) {
        return r; // error!
    }

    int64_t sz = get_filesize(r->handle);
    if (sz == -1) {
        perror("get_filesize");
        r->code = -1;
        close(r->handle);
        return r;
    }

    r->data = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE, r->handle, 0);
    if (r->data == NULL) {
        perror("MMAP");
        close(r->handle);
        r->code = -1;
    }
    return r;
}

static rval_t* input = NULL;

int open_and_map(const char* filename) {
    input = map_file(filename);
    if (input->code < 0) {
        fprintf(stderr, "Error opening or mapping file: %s\n", filename);
        return -1;
    }
    
    if (input->data == NULL) {
        fprintf(stderr, "Error mapping file data: %s\n", filename);
        close(input->handle);
        free(input);
        input = NULL;
        return -1;
    }
    input->mark = input->data; // set the mark to the start of the data
    return 0;
}

int unmap_and_close() {
    if (input == NULL || input->code < 0) {
        fprintf(stderr, "No valid input to unmap and close.\n");
        return -1;
    }

    if (munmap(input->mark, get_filesize(input->handle)) < 0) {
        perror("munmap");
        return -1;
    }

    if (close(input->handle) < 0) {
        perror("close");
        return -1;
    }

    free(input);
    input = NULL;
    return 0;
}

int READ_8(bool unused) {
    if (input == NULL || input->data == NULL) {
        fprintf(stderr, "No valid input data to read from.\n");
        return -1;
    }

    uint8_t value = *((uint8_t*)input->data);
    input->data = (void*)((uint8_t*)input->data + 1); // move the pointer forward

    return value;
}

int READ_8_16(bool read16) {
    uint16_t value;
    if (input == NULL || input->data == NULL) {
        fprintf(stderr, "No valid input data to read from.\n");
        return -1;
    }

    if (read16) {
        value = *((uint16_t*)input->data);
        input->data = (void*)((uint8_t*)input->data + 2); // move the pointer forward
    } else {
        value = *((uint8_t*)input->data);
        input->data = (void*)((uint8_t*)input->data + 1); // move the pointer forward
    }

    return value;
}

int READ_16(bool unused) {
    if (input == NULL || input->data == NULL) {
        fprintf(stderr, "No valid input data to read from.\n");
        return -1;
    }

    uint16_t value = *((uint16_t*)input->data);
    input->data = (void*)((uint8_t*)input->data + 2); // move the pointer forward
    return value;
}

int READ_BMA(bool unused) {
    if (input == NULL || input->data == NULL) {
        fprintf(stderr, "No valid input data to read from.\n");
        return -1;
    }

    uint8_t value1 = *((uint8_t*)input->data);
    input->data = (void*)((uint8_t*)input->data + 1); // move the pointer forward
    uint8_t value2 = *((uint8_t*)input->data);
    input->data = (void*)((uint8_t*)input->data + 1); // move the pointer forward

    // thanks to how we're doing shit here, we have to get funky with this...
    return (value1 << 16) | value2; // combine the two bytes into a single value
}

int READ_24(bool unused) {
    if (input == NULL || input->data == NULL) {
        fprintf(stderr, "No valid input data to read from.\n");
        return -1;
    }

    uint16_t low_word = *((uint8_t*)input->data);
    input->data = (void*)((uint8_t*)input->data + 2); // move the pointer forward
    uint8_t high_byte = *((uint8_t*)input->data);
    input->data = (void*)((uint8_t*)input->data + 1); // move the pointer forward

    // expected to return a 24bit value, do so...
    return (high_byte << 16) | low_word;
}

extern char* format_opcode_and_operands(codeentry_t*, ...);


listent_t* disasm_raw(char* filename) {
    if (open_and_map(filename) < 0) {
        fprintf(stderr, "Failed to open and map file: %s\n", filename);
        return NULL;
    }


    // Disassembly logic goes here...
    // This could use some refactoring, but we do the map+later list for book-keeping
    // purposes. It'd be O(n) for any label if we just went with a raw list instead
    // of using an intermediate hash-map. Might be worth it -- max address space for
    // the 65C816 is 16M but this comes in 64k segments, which I don't think we're
    // ever going to see code using more than one of except for things like data
    // storage, so an O(n) linked list might be fine, performance-wise...
    uint32_t len = get_filesize(input->handle);
    while ((input->data - input->mark) < len) {
        uint32_t offset = (input->data - input->mark) + get_start_offset();
        uint8_t opcode = READ_8(false);
        const opcode_t* code = &opcodes[opcode];
        bool size_check = code->munge(code->psize) > code->psize;
        uint32_t params = code->reader?code->reader(size_check):0;

        if (code->state) {
            // if the opcode has a state function, call it
            // Used for tracking the state of the CPU for the E, M and X flags
            code->state((uint8_t)params);
        }
        if (code->flags & BlockMoveAddress) {
            // take apart the uint32_t -- one byte is & 0xFF the other is >> 16 & 0xFF
            uint8_t param1 = (params >> 16) & 0xFF;
            uint8_t param2 = params & 0xFF;
            // MVN and MVP have two parameters -- the source page and destination page
            // stub this (for now) -- should be entering into the disassembly map
            add_entry(offset, make_line(offset, opcode, param1, param2));
        } else {
            // for all other opcodes, we just need the opcode and the single parameter
            // again, somewhat stubbed as we should be entering into the disassembly map
            add_entry(offset, make_line(offset, opcode, params));
        }
        // we _KNOW_ we have an entry now, so we can process for labels
        if (code->extra) { 
            // if the opcode has an extra function, call it
            // this is used for JSR, JSL, JMP, BRL and BRA to create labels
            code->extra((signed)params, offset);
        }  
    }

    // create the output...
    listent_t* retval;
    for(uint32_t i = 0+get_start_offset(); i < len+get_start_offset(); i++) {
        codeentry_t* code = find_node(i);

        if (code != NULL) {
            // we never allocate the start, just init it here if needed
            if (retval == NULL) retval = init_node(code);
            else append_node(retval, init_node(code));
        }
    }

    unmap_and_close();
    return retval;
}

/*
    This is finally in a coherent state, with an internal helper to get the raw data.
*/
void disasm_dump(char *filename) {
    listent_t* disasm_data = disasm_raw(filename); // get the data
    listent_t* work = disasm_data;                 // working copy to preserve the pointer

    if (disasm_data == NULL) {
        printf("ERROR: disassembly returned NULL\n");
        return;
    }

    // loop the data for output
    while(work != NULL) {
        codeentry_t* entry = work->data;
        printf("0x%06X: %s\n", entry->offset, format_opcode_and_operands(entry, entry->params[0], entry->params[1])); // OUTPUT! (no, Johnny 5, not Input)
        work = work->child;
    }

    // and clean up
    delete_list(disasm_data);
}

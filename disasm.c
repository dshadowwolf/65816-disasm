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

#include "ops.h"

extern const opcode_t opcodes[256];
typedef struct rval_s {
    uint32_t code;
    uint32_t handle;
    void* data;
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


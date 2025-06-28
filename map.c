#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "list.h"

static listent_t* mapbase[512];
typedef struct mapent_s {
    uint32_t key;
    void* val;
} mapent_t;

// this is too raw... need to mod this so we can get exact values out...
// perhaps... hrm...
void add_entry(uint32_t key, void* val) {
    uint32_t k = key%512; // bucket
    mapent_t* ent = malloc(sizeof(mapent_t));
    ent->key = key;
    ent->val = val;

    if (mapbase[k] == NULL) {
        mapbase[k] = init_node(ent);
    } else {
        append_node(mapbase[k], init_node(ent));
    }
}

void* find_node(uint32_t key) {
    uint32_t k = key%512; // bucket
    listent_t* work = mapbase[k];

    while(work != NULL) {
        mapent_t* ent = (mapent_t*)work->data;
        if (ent->key == key) {
            return ent->val; // found it!
        }
        work = work->child;
    }
    return NULL; // not found
}

void delete_entry(uint32_t key) {
    uint32_t k = key%512; // bucket
    listent_t* work = mapbase[k];

    while(work != NULL) {
        mapent_t* ent = (mapent_t*)work->data;
        if (ent->key == key) {
            delete_node(work);
            return;
        }
        work = work->child;
    }
}

void delete_map() {
    for (int i = 0; i < 512; i++) {
        if (mapbase[i] != NULL) {
            delete_list((listent_t*)mapbase[i]);
            mapbase[i] = NULL;
        }
    }
}

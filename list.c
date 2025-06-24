#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "list.h"

listent_t* init_node(void* nodeval) {
    listent_t* rv = malloc(sizeof(listent_t));
    rv = memset(rv, 0, sizeof(listent_t));
    rv->data = nodeval;
    return rv;
}

void append_node(listent_t* list, listent_t* node) {
    listent_t* work;

    work = list;
    while(work->child != NULL) work = work->child;
    work->child = node;
    node->parent = work;
}

void append(listent_t* list, void* val) {
    listent_t* node = init_node(val);
    if (list == NULL) {
        list = node;
    } else {
        append_node(list, node);
    }
}

void delete_node(listent_t* list) {
    listent_t *work = list->parent, *hold = list->child;
    hold->parent = work;
    work->child = hold;
    free(list->data);
    free(list);
}

void delete_list(listent_t* list) {
    listent_t* work = list;

    while(work->child != NULL) work = work->child;
    while(work->parent != NULL) {
        listent_t* hold = work;
        work = work->parent;
        free(hold->data);
        free(hold);
    }
    free(work->data);
    free(work);
}
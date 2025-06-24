#include <stdint.h>

void add_entry(uint32_t key, void* val);
void* find_node(uint32_t key);
void delete_entry(uint32_t key);
void delete_map();

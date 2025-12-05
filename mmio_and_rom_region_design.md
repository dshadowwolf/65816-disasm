### Idea
Basic idea is to have a table of defined memory regions that each map to a callback or set of callbacks for IO. The `get_memory_bank` should return a callback for accessing this table, which, of necessity and by the design of the 65C816 be different for each Program Bank and Data Bank.

### Design Ideas
```c
typedef struct mem_table_s {
    uint8_t bank;
    uint16_t max_addr;
    uint16_t min_addr;
    // write operations are permitted to NOP
    void write_word(uint8_t *data, uint16_t address, uint16_t value); 
    void write_byte(uint8_t *data, uint16_t address, uint8_t value);
    // READ operations, on uninitialized areas that are ROM
    // should return 0xFF and on RAM 0x00
    uint8_t read_byte(uint8_t *data, uint16_t address);
    uint16_t read_word(uint8_t *data, uint16_t address);
    uint8_t *region_data; // may be NULL for MMIO regions
} mem_table_t;
```

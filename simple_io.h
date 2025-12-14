/*
 * Simple IO Interface for Board FIFO
 * 
 * This provides a simple API for reading/writing bytes through the
 * VIA-connected FIFO to USB, useful for testing CPU code that does IO.
 */

#ifndef SIMPLE_IO_H
#define SIMPLE_IO_H

#include <stdint.h>
#include <stdbool.h>
#include "board_fifo.h"

// Initialize the IO system
// Returns: fifo handle, or NULL on failure
fifo_t *io_init(void);

// Check if data is available to read from USB
bool io_data_available(fifo_t *fifo);

// Check if space is available to write to USB
bool io_space_available(fifo_t *fifo);

// Read a byte from USB/FIFO (blocking)
uint8_t io_read_byte(fifo_t *fifo);

// Write a byte to USB/FIFO (blocking)
void io_write_byte(fifo_t *fifo, uint8_t data);

// Write a string to USB/FIFO
void io_write_string(fifo_t *fifo, const char *str);

#endif // SIMPLE_IO_H

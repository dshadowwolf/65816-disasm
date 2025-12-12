/*
 * Simple IO Interactive Test
 * 
 * This program creates an interactive session where:
 * - Data sent by the CPU is displayed to stdout
 * - Data typed into stdin is sent to the CPU
 * 
 * Use Ctrl+D to exit.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "simple_io.h"
#include "board_fifo.h"

static struct termios orig_termios;
static bool raw_mode = false;

// Enable raw mode for immediate character input
void enable_raw_mode(void) {
    struct termios raw;
    
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        return;
    }
    
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // Disable echo and canonical mode
    raw.c_cc[VMIN] = 0;   // Non-blocking read
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1) {
        raw_mode = true;
    }
}

// Restore original terminal settings
void disable_raw_mode(void) {
    if (raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode = false;
    }
}

// CPU echo program - reads from FIFO and echoes back
void cpu_echo_program(fifo_t *fifo) {
    while (1) {
        // Check if data available from USB
        if (io_data_available(fifo)) {
            uint8_t byte = io_read_byte(fifo);
            
            // Exit on Ctrl+D (EOF, 0x04)
            if (byte == 0x04) {
                break;
            }
            
            // Echo the byte back
            io_write_byte(fifo, byte);
        }
    }
}

// CPU greeting program - sends a greeting then echoes
void cpu_greeting_program(fifo_t *fifo) {
    const char *greeting = "Hello! I'm the 65816 CPU. Type something and I'll echo it back.\r\n";
    const char *prompt = "CPU> ";
    
    io_write_string(fifo, greeting);
    io_write_string(fifo, prompt);
    
    while (1) {
        if (io_data_available(fifo)) {
            uint8_t byte = io_read_byte(fifo);
            
            // Exit on Ctrl+D
            if (byte == 0x04) {
                io_write_string(fifo, "\r\nGoodbye!\r\n");
                break;
            }
            
            // Echo the character
            io_write_byte(fifo, byte);
            
            // On newline, send prompt
            if (byte == '\r' || byte == '\n') {
                io_write_string(fifo, prompt);
            }
        }
    }
}

// CPU line-based program - processes complete lines
void cpu_line_program(fifo_t *fifo) {
    char line_buffer[256];
    int line_pos = 0;
    
    io_write_string(fifo, "65816 Line Processor Ready\r\n");
    io_write_string(fifo, "Commands: HELLO, ECHO <text>, ADD <a> <b>, QUIT\r\n");
    io_write_string(fifo, "> ");
    
    while (1) {
        if (io_data_available(fifo)) {
            uint8_t byte = io_read_byte(fifo);
            
            // Exit on Ctrl+D
            if (byte == 0x04) {
                io_write_string(fifo, "\r\nBye!\r\n");
                break;
            }
            
            // Echo character
            io_write_byte(fifo, byte);
            
            // Handle backspace
            if (byte == 0x7F || byte == 0x08) {
                if (line_pos > 0) {
                    line_pos--;
                }
                continue;
            }
            
            // Process on newline
            if (byte == '\r' || byte == '\n') {
                line_buffer[line_pos] = '\0';
                
                // Process command
                if (line_pos > 0) {
                    if (strcmp(line_buffer, "HELLO") == 0) {
                        io_write_string(fifo, "Hi there!\r\n");
                    } else if (strncmp(line_buffer, "ECHO ", 5) == 0) {
                        io_write_string(fifo, line_buffer + 5);
                        io_write_string(fifo, "\r\n");
                    } else if (strncmp(line_buffer, "ADD ", 4) == 0) {
                        int a, b;
                        if (sscanf(line_buffer + 4, "%d %d", &a, &b) == 2) {
                            char result[64];
                            snprintf(result, sizeof(result), "Result: %d\r\n", a + b);
                            io_write_string(fifo, result);
                        } else {
                            io_write_string(fifo, "Error: Usage: ADD <num1> <num2>\r\n");
                        }
                    } else if (strcmp(line_buffer, "QUIT") == 0) {
                        io_write_string(fifo, "Goodbye!\r\n");
                        break;
                    } else {
                        io_write_string(fifo, "Unknown command\r\n");
                    }
                }
                
                line_pos = 0;
                io_write_string(fifo, "> ");
            } else if (line_pos < 255) {
                line_buffer[line_pos++] = byte;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║   Simple IO Interactive Test           ║\n");
    printf("║   CPU ↔ Terminal via Board FIFO        ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Initialize IO
    fifo_t *fifo = io_init();
    if (!fifo) {
        fprintf(stderr, "ERROR: Failed to initialize IO\n");
        return 1;
    }
    
    // Determine which program to run
    int mode = 2; // Default to line processor
    if (argc > 1) {
        mode = atoi(argv[1]);
    }
    
    printf("Mode: ");
    switch (mode) {
        case 0:
            printf("Simple Echo\n");
            break;
        case 1:
            printf("Greeting Echo\n");
            break;
        case 2:
            printf("Line Processor\n");
            break;
        default:
            printf("Unknown (using Line Processor)\n");
            mode = 2;
    }
    
    printf("\nPress Ctrl+D to exit\n");
    printf("════════════════════════════════════════\n\n");
    
    // Enable raw mode for character-by-character input
    enable_raw_mode();
    
    // Make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    // Start CPU program in the background (simulated)
    bool cpu_running = true;
    bool cpu_started = false;
    
    // Main loop: shuttle data between stdin/stdout and FIFO
    while (cpu_running) {
        // Start CPU program on first iteration
        if (!cpu_started) {
            cpu_started = true;
            // Note: In a real system, CPU would be running independently
            // For this simulation, we'll interleave CPU execution with IO
        }
        
        // Read from stdin and send to CPU via USB→FIFO
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            // Send character to CPU
            board_fifo_usb_send_to_cpu(fifo, ch);
        }
        
        // Let CPU process (simulate CPU executing instructions)
        switch (mode) {
            case 0:
                // Simple echo: if data available, read and write back
                if (io_data_available(fifo)) {
                    uint8_t byte = io_read_byte(fifo);
                    if (byte == 0x04) {
                        cpu_running = false;
                    } else {
                        io_write_byte(fifo, byte);
                    }
                }
                break;
                
            case 1:
                // Greeting echo
                if (!cpu_started) {
                    const char *greeting = "Hello! I'm the 65816 CPU. Type something and I'll echo it back.\r\n";
                    const char *prompt = "CPU> ";
                    io_write_string(fifo, greeting);
                    io_write_string(fifo, prompt);
                }
                if (io_data_available(fifo)) {
                    uint8_t byte = io_read_byte(fifo);
                    if (byte == 0x04) {
                        io_write_string(fifo, "\r\nGoodbye!\r\n");
                        cpu_running = false;
                    } else {
                        io_write_byte(fifo, byte);
                        if (byte == '\r' || byte == '\n') {
                            io_write_string(fifo, "CPU> ");
                        }
                    }
                }
                break;
                
            case 2:
                // Line processor
                {
                    static char line_buffer[256];
                    static int line_pos = 0;
                    static bool prompt_sent = false;
                    
                    if (!prompt_sent) {
                        io_write_string(fifo, "65816 Line Processor Ready\r\n");
                        io_write_string(fifo, "Commands: HELLO, ECHO <text>, ADD <a> <b>, QUIT\r\n");
                        io_write_string(fifo, "> ");
                        prompt_sent = true;
                    }
                    
                    if (io_data_available(fifo)) {
                        uint8_t byte = io_read_byte(fifo);
                        
                        if (byte == 0x04) {
                            io_write_string(fifo, "\r\nBye!\r\n");
                            cpu_running = false;
                        } else {
                            // Echo character
                            io_write_byte(fifo, byte);
                            
                            // Handle backspace
                            if (byte == 0x7F || byte == 0x08) {
                                if (line_pos > 0) {
                                    line_pos--;
                                }
                            } else if (byte == '\r' || byte == '\n') {
                                line_buffer[line_pos] = '\0';
                                
                                if (line_pos > 0) {
                                    if (strcmp(line_buffer, "HELLO") == 0) {
                                        io_write_string(fifo, "Hi there!\r\n");
                                    } else if (strncmp(line_buffer, "ECHO ", 5) == 0) {
                                        io_write_string(fifo, line_buffer + 5);
                                        io_write_string(fifo, "\r\n");
                                    } else if (strncmp(line_buffer, "ADD ", 4) == 0) {
                                        int a, b;
                                        if (sscanf(line_buffer + 4, "%d %d", &a, &b) == 2) {
                                            char result[64];
                                            snprintf(result, sizeof(result), "Result: %d\r\n", a + b);
                                            io_write_string(fifo, result);
                                        } else {
                                            io_write_string(fifo, "Error: Usage: ADD <num1> <num2>\r\n");
                                        }
                                    } else if (strcmp(line_buffer, "QUIT") == 0) {
                                        io_write_string(fifo, "Goodbye!\r\n");
                                        cpu_running = false;
                                    } else {
                                        io_write_string(fifo, "Unknown command\r\n");
                                    }
                                }
                                
                                line_pos = 0;
                                io_write_string(fifo, "> ");
                            } else if (line_pos < 255) {
                                line_buffer[line_pos++] = byte;
                            }
                        }
                    }
                }
                break;
        }
        
        // Read from FIFO→USB and display to stdout
        uint8_t output_byte;
        if (board_fifo_usb_receive_from_cpu(fifo, &output_byte)) {
            write(STDOUT_FILENO, &output_byte, 1);
        }
        
        // Small delay to avoid spinning too fast
        usleep(1000);
    }
    
    // Cleanup
    disable_raw_mode();
    free_board_fifo(fifo);
    
    printf("\n\n╔════════════════════════════════════════╗\n");
    printf("║   Session ended                        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}

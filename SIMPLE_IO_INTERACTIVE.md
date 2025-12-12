# Simple IO Interactive Test

This program creates an interactive terminal session where you can communicate with simulated 65816 CPU code through the board_fifo interface.

## Building

```bash
make simple_io_interactive
```

## Running

```bash
./simple_io_interactive [mode]
```

Where `mode` is:
- `0` - Simple Echo (CPU echoes everything you type)
- `1` - Greeting Echo (CPU shows greeting, then echoes with prompts)
- `2` - Line Processor (default - CPU processes commands)

## Usage Examples

### Mode 0: Simple Echo
```bash
./simple_io_interactive 0
```
Everything you type will be echoed back by the CPU.

### Mode 1: Greeting Echo
```bash
./simple_io_interactive 1
```
CPU shows a greeting, then echoes with prompts.

### Mode 2: Line Processor (Default)
```bash
./simple_io_interactive
# or
./simple_io_interactive 2
```

Available commands:
- `HELLO` - CPU responds with a greeting
- `ECHO <text>` - CPU echoes back the text
- `ADD <num1> <num2>` - CPU adds two numbers and returns result
- `QUIT` - Exit the program

Example session:
```
> HELLO
Hi there!
> ECHO Testing 123
Testing 123
> ADD 5 7
Result: 12
> QUIT
Goodbye!
```

## How It Works

The program simulates a 65816 CPU running code that:
1. Reads bytes from the board_fifo (via VIA 6522)
2. Processes the input
3. Writes responses back to the board_fifo

Your terminal:
- **stdin** → USB → board_fifo → CPU reads
- **CPU writes** → board_fifo → USB → **stdout**

The CPU execution is simulated within the main loop, interleaving CPU instruction execution with terminal I/O.

## Exiting

Press `Ctrl+D` to exit the program.

## Notes

- The terminal is placed in raw mode for character-by-character input
- Characters are echoed by the CPU, not the terminal
- Use `Ctrl+D` (not `Ctrl+C`) to exit cleanly

# Simple Computer Simulator

This repository contains a small compiler/simulator project for a simple instruction set. It reads a program file, loads initial data, executes the program, and writes the final state.

## Requirements

Make sure you have:
- GCC (or any C compiler)
- make

## Build the simulator

Open the project folder and run:

```bash
cd Lab1
make
```

This will build the executable named `simulator`.

On Windows, the compiled file may appear as `simulator.exe`.

## How to run the simulator

### 1. Run with default files

If you run it without arguments, it uses:
- input file: `input.txt`
- data file: `data.byte`

```bash
./simulator
```

### 2. Run with custom files

You can provide your own input and data files as arguments:

```bash
./simulator my_input.txt my_data.byte
```

### 3. Run with the sample test files included in the repository

The repository already contains example files such as:
- `test1.txt` with `data1.byte`
- `test2.txt` with `data2.byte`
- `test3.txt` with `data3.byte`

Example:

```bash
./simulator test1.txt data1.byte
./simulator test2.txt data2.byte
./simulator test3.txt data3.byte
```

## Notes

- The simulator expects the first argument to be the program/input file and the second argument to be the data file.
- If the wrong number of arguments is provided, the program will print usage instructions and exit.
- The executable name is `simulator` on Linux/WSL and `simulator.exe` on Windows.

## Extension features

- Processes use saved CPU contexts and execute in time slices.
- Ready-queue scheduling uses priority with aging; equal-priority processes remain round-robin.
- Page-table entries are stored in reserved physical memory and include read, write, and execute permissions.
- A small TLB and direct-mapped data cache record hit/miss statistics and add simulated access delay.
- The shared buffer is mapped at logical data address `SHARED_MEMORY_ADDRESS` in `memory.h`.
- MMIO addresses are `IO_CONSOLE_OUT`, `IO_CONSOLE_IN`, and `IO_PROCESS_ID`.

While the simulator is running, useful shell commands include:

```text
stats
memmap <process-id>
cache <process-id>
tlb <process-id>
priority <process-id> <priority>
```

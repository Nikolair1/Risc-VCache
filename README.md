# Risc-V Cache
This project implements a memory hierarchy in C++ with a focus on designing a memory controller. The memory hierarchy consists of L1 and L2 caches, main memory, and a victim cache. The L1 cache is direct-mapped, the L2 cache is 8-way set-associative, and a victim cache is fully-associative. The code, written exclusively in C++, follows a modular approach with well-defined functions, adhering to an exclusive/write-no-allocate/write-through cache design. The system processes load (LW) and store (SW) instructions from a trace file, providing miss rates for L1 and L2, along with the average access time (AAT). 
# Compilation
The code is compiled using "g++ *.cpp -o memory_driver" and results are formatted as "(L1,L2,AAT)" in the terminal.
# Running a trace file
Each line in the trace file represents an instruction with four values:
MemR (Memory Read): 0 or 1, indicating if it's a LOAD instruction.
MemW (Memory Write): 0 or 1, indicating if it's a STORE instruction.
adr (Address): A number between 0 and 4095.
data: A signed integer value.
For example, a line might look like: 1 0 1024 42, representing a STORE instruction at address 1024 with data value 42. The program reads these lines, processes the instructions, and updates the memory hierarchy accordingly.

After compiling run:
./memory_driver <inputfile.txt>
# Diagram
![3-FigureD 1-1](https://github.com/Nikolair1/RiscVCache/assets/93243326/23d8a035-7cd4-46e6-b4ac-ca253ecaa4f1)

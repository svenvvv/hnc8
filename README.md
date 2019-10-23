# hnc8
CHIP-8 emulator with a debug server

# Building

The only dependency is `glfw`.  
Currently only tested under GNU/Linux.  

To build a debug build:  
`make debug`

To build a release build:  
`make release`

# Usage

## Emulator mode

`./hnc8 rom.ch8`  

## Disassembler mode

`./hnc8 -md rom.ch8`  

## Debug server mode

`./hnc8 -ms`

# Command line arguments

### -m
Select mode of operation.  
Valid modes are "emu", "server" and "disasm".  

### -h
Display help text.  

### -v
Output version information.  

## Disassembler arguments

### -a
Display memory addresses before assembly.  

### -i
Display raw instruction bytes before assembly.  

## Emulator arguments

### -f integer
Set clock multiplier for the emulator core.  
Timers are still ticked at 60hz.  

### -s double
Set graphical output scale.

## Debug server arguments

### -p integer
Set debug server listen port.

# Emulator keys

### 1-4, Q-R, A-F, Z-V
Chip8 keypad.  

### F5 
Reset emulator.  

### ESC
Quit emulator.  

### TAB
Turbo mode.  

# Debug server

## Usage

Launch the emulator in debug server mode:  
`./hnc8 -ms`

Use netcat to connect to it:  
`nc localhost 8888`

To disconnect send the command `shutdown` and disconnct from the server.

## Commands

### help

Display help.

### shutdown

Shut down the server after client disconnect.

### load filename / l filename

Load a file into the emulator core.

### break [address] / b [address]

Set a breakpoint on specified address or current PC.

### lsbreak / lb

List all breakpoints.

### rmbreak [index] / rb [index]

Remove breakpoint at index or remove most recently added breakpoint.

### continue / c

Continue execution after a breakpoint.

### backtrace / bt

Display the stack values.

### stepi [count] / si [count]

Execute count number of instructions from current PC.  
If count is none then a single instruction is executed.

### examine address [count] / x address [count]

Examine memory contents at address. Displays count bytes.

### commands

List all commands

### registers [register] [value] / r [register] [value]

No arguments:  
Display contents of all registers.

Register argument set:  
Display contents of specified register.

Register and value arguments set:  
Set contents of register to value.

### setkey keynum / sk keynum

Toggle state of key keynum.

### keys / k

Display keypad state.

### disassemble [count] [address] / da [count] [address]

Disassemble count opcodes starting at address.  
If address is not specified then from current PC.

### screen / scr

Display VM screen contents.

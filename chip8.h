/*
 * hnc8
 * Copyright (C) 2019 hundinui
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>


#define VM_EXEC_START_ADDR  0x200
#define VM_RAM_SIZE         4096
#define VM_SCREEN_WIDTH     64
#define VM_SCREEN_HEIGHT    32
#define VM_STACK_SIZE       16
#define VM_KEY_COUNT        16
#define VM_FONT_H           5

typedef struct {
    /* Registers */
    uint8_t v[16];
    uint16_t i;
    uint16_t pc;
    uint8_t sp;
    uint16_t stack[VM_STACK_SIZE];
    uint8_t keys[VM_KEY_COUNT];
    /* Timers */
    uint8_t tim_delay;
    uint8_t tim_sound;
    /* Memory */
    bool vram_updated;
    uint8_t vram[VM_SCREEN_WIDTH * VM_SCREEN_HEIGHT];
    uint8_t ram[VM_RAM_SIZE];
} ch8_t;

/*
 * Initialize the VM core
 */
void ch8_init(ch8_t *vm);

/*
 * Return the current opcode at PC
 */
uint16_t ch8_get_op(ch8_t *vm);

/*
 * Reinitialize the VM core and load rom into VM memory.
 *
 * Params:
 *  rom     - pointer to file contents,
 *  rom_sz  - amount of bytes to read.
 */
void ch8_load(ch8_t *vm, const uint16_t *rom, uint16_t rom_sz);

/*
 * Execute a single instruction
 */
void ch8_tick(ch8_t *vm);

/*
 * Increment timer registers
 *
 * NOTE: this function MUST be called at a frequency of 60hz
 */
void ch8_tick_timers(ch8_t *vm);

/*
 * Disassemble opcode into mnemonics and operands.
 *
 * Params
 *  opcode  - 2 byte opcode to disassemble.
 *
 * Returns
 *  pointer to zero terminated string.
 * NOTE: the string pointed to is only guaranteed to be valid until the
 * next call to ch8_disassemble, at which point it may be overwritten.
 */
const char *ch8_disassemble(uint16_t opcode);

/*
 * Execute a single instruction in VM
 *
 * Params
 *  opcode  - 2 byte opcode to execute.
 */
void ch8_exec(ch8_t *vm, uint16_t opcode);

#endif // CHIP8_H

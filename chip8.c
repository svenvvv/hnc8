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

#include "chip8.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define FONT_SZ 16 * VM_FONT_H

const static uint8_t builtin_font[FONT_SZ] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void ch8_init(ch8_t *vm)
{
    assert(vm != NULL);

    memset(vm, 0, sizeof(*vm));

    /* copy fonts to RAM */
    memcpy(vm->ram, builtin_font, FONT_SZ);

    vm->pc = VM_EXEC_START_ADDR;
    vm->vram_updated = false;

    /* Seed rand() for RND opcode */
    srand(time(NULL));
}

inline uint16_t ch8_get_op(ch8_t *vm)
{
    return (vm->ram[vm->pc] << 8) | vm->ram[vm->pc + 1];
}

void ch8_load(ch8_t *vm, const uint16_t *rom, uint16_t rom_sz)
{
    assert(vm != NULL);
    assert(rom != NULL);

    ch8_init(vm);

    memcpy(vm->ram + VM_EXEC_START_ADDR, rom, rom_sz);
}

void ch8_tick(ch8_t *vm)
{
    assert(vm != NULL);

    uint16_t opcode = ch8_get_op(vm);
    ch8_exec(vm, opcode);

    vm->pc += 2;
}

void ch8_tick_timers(ch8_t *vm)
{
    assert(vm != NULL);

    if(vm->tim_delay > 0) {
        vm->tim_delay -= 1;
    }
    if(vm->tim_sound > 0) {
        vm->tim_sound -= 1;
    }
}


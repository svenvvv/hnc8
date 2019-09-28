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

void ch8_load(ch8_t* vm, const uint16_t* rom, uint16_t rom_sz)
{
    assert(vm != NULL);
    assert(rom != NULL);

    memset(vm, 0, sizeof(*vm));
    memcpy(vm->ram + VM_EXEC_START_ADDR, rom, rom_sz);

    vm->pc = VM_EXEC_START_ADDR;

    /* Seed rand() for RND opcode */
    srand(time(NULL));
}

void ch8_tick(ch8_t* vm)
{
    assert(vm != NULL);

    uint16_t opcode = (vm->ram[vm->pc] << 8) | vm->ram[vm->pc + 1];
    ch8_exec(vm, opcode);
}

void ch8_tick_timers(ch8_t* vm)
{
    assert(vm != NULL);

    if(vm->tim_delay > 0) {
        vm->tim_delay -= 1;
    }
    if(vm->tim_sound > 0) {
        vm->tim_sound -= 1;
    }
}


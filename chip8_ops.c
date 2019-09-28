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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "chip8.h"

#define SETTXT(...)
#define UNKNOWN_OP(opcode)  SETTXT("UNKNOWN"); LOG_ERROR("UNKNOWN OPCODE %04X", opcode)

#define SETVF vm->v[0xF] = 1
#define CLRVF vm->v[0xF] = 0

static void ops_x0(ch8_t *vm, uint16_t opcode)
{
    switch((opcode & 0xF0) >> 4) {
        case 0xE:
            switch(opcode & 0x000F) {
                case 0x0:
                    memset(vm->vram, 0, VM_SCREEN_WIDTH * VM_SCREEN_HEIGHT);
                    break;
                case 0xE:
                    vm->pc = vm->stack[--vm->sp];
                    break;
            }
            break;
        case 0x0:
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void jp(ch8_t *vm, uint16_t opcode)
{
    uint16_t addr = opcode & 0x0FFF;
    vm->pc = addr;
}

static void call(ch8_t *vm, uint16_t opcode)
{
    uint16_t addr = opcode & 0x0FFF;
    vm->stack[vm->sp++] = vm->pc;
    vm->pc = addr;
}

static void se_vi(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    if(vm->v[reg] == imm) {
        vm->pc += 2;
    }
}

static void sne_vi(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    if(vm->v[reg] != imm) {
        vm->pc += 2;
    }
}

static void se_vv(ch8_t *vm, uint16_t opcode)
{
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    if(vm->v[rega] == vm->v[regb]) {
        vm->pc += 2;
    }
}

static void ld_vi(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    vm->v[reg] = imm;
}

static void add(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    vm->v[reg] += imm;
}

static void ops_x8(ch8_t *vm, uint16_t opcode)
{
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    uint16_t result;
    switch(opcode & 0x000F) {
        case 0:
            vm->v[rega] = vm->v[regb];
            break;
        case 1:
            vm->v[rega] |= vm->v[regb];
            break;
        case 2:
            vm->v[rega] &= vm->v[regb];
            break;
        case 3:
            vm->v[rega] ^= vm->v[regb];
            break;
        case 4:
            result = vm->v[rega] + vm->v[regb];
            if(result > 0xFF) {
                SETVF;
            } else {
                CLRVF;
            }
            vm->v[rega] = result;
            break;
        case 5:
            if(vm->v[rega] > vm->v[regb]) {
                SETVF;
            } else {
                CLRVF;
            }
            vm->v[rega] -= vm->v[regb];
            break;
        case 6:
            if(vm->v[rega] & 1) {
                SETVF;
            } else {
                CLRVF;
            }
            vm->v[rega] = vm->v[rega] >> 1;
            break;
        case 7:
            if(vm->v[regb] > vm->v[rega]) {
                SETVF;
            } else {
                CLRVF;
            }
            vm->v[rega] = vm->v[regb] - vm->v[rega];
            break;
        case 0xE:
            if(vm->v[rega] & 0x80) {
                SETVF;
            } else {
                CLRVF;
            }
            vm->v[rega] = vm->v[rega] << 1;
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void sne_vv(ch8_t *vm, uint16_t opcode)
{
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    if(vm->v[rega] != vm->v[regb]) {
        vm->pc += 2;
    }
}

static void ld_i(ch8_t *vm, uint16_t opcode)
{
    uint16_t addr = opcode & 0x0FFF;
    vm->i = addr;
}

static void jp_v(ch8_t *vm, uint16_t opcode)
{
    uint16_t addr = opcode & 0x0FFF;
    vm->pc = vm->v[0] + addr;
}

static void rnd(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    vm->v[reg] = rand() & imm;
}

static void drw(ch8_t *vm, uint16_t opcode)
{
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    uint8_t bytes = (opcode & 0x000F);
    SETTXT("DRW V%X, V%X, %u", rega, regb, bytes);
}

static void skip(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t key = vm->v[reg];
    switch(opcode & 0xFF) {
        case 0x9E:
            if(vm->keys[key]) {
                vm->pc += 2;
            }
            break;
        case 0xA1:
            if(vm->keys[key] == 0) {
                vm->pc += 2;
            }
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void ops_xF(ch8_t *vm, uint16_t opcode)
{
    uint8_t reg = (opcode & 0x0F00) >> 8;
    switch(opcode & 0xFF) {
        case 0x07:
            vm->v[reg] = vm->tim_delay;
            break;
        case 0x0A:
            /*
             * decrement PC so we'll loop, doing it here
             * for simplicity. if a key is found then it'll
             * be incremented to break out of the loop.
             */
            vm->pc -= 2;
            for(uint8_t i = 0; i < VM_KEY_COUNT; ++i) {
                if(vm->keys[i] != 0) {
                    vm->v[reg] = i;
                    vm->pc += 2;
                    break;
                }
            }
            break;
        case 0x15:
            vm->tim_delay = vm->v[reg];
            break;
        case 0x18:
            vm->tim_sound = vm->v[reg];
            break;
        case 0x1E:
            vm->i += vm->v[reg];
            break;
        case 0x29:
            vm->i = vm->v[reg] * VM_FONT_H;
            break;
        case 0x33:
            vm->ram[vm->i] = vm->v[reg] / 100;
            vm->ram[vm->i + 1] = (vm->v[reg] / 10) % 10;
            vm->ram[vm->i + 2] = (vm->v[reg] % 100) % 10;
            break;
        case 0x55:
            memcpy(vm->ram + vm->i, vm->v, reg);
            break;
        case 0x65:
            memcpy(vm->v, vm->ram + vm->i, reg);
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void (*ch8_opcode_lut[16])(ch8_t *vm, uint16_t opcode) = {
    ops_x0,       // 0x0xxx
    jp,           // 0x1xxx
    call,         // 0x2xxx
    se_vi,        // 0x3xxx
    sne_vi,       // 0x4xxx
    se_vv,        // 0x5xxx
    ld_vi,        // 0x6xxx
    add,          // 0x7xxx
    ops_x8,       // 0x8xxx
    sne_vv,       // 0x9xxx
    ld_i,         // 0xAxxx
    jp_v,         // 0xBxxx
    rnd,          // 0xCxxx
    drw,          // 0xDxxx
    skip,         // 0xExxx
    ops_xF        // 0xFxxx
};

void ch8_exec(ch8_t *vm, uint16_t opcode)
{
    uint8_t op_index = opcode >> 12;
    (*ch8_opcode_lut[op_index])(vm, opcode);
}

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
#include "log.h"

#define DISASM_BUF_SZ 32

#define SETTXT(...)         snprintf(g_disasm_buf, DISASM_BUF_SZ, __VA_ARGS__)
#define UNKNOWN_OP(opcode)  SETTXT("UNKNOWN"); LOG_ERROR("UNKNOWN OPCODE %04X", opcode)

static char g_disasm_buf[DISASM_BUF_SZ];

static void ops_x0(uint16_t opcode)
{
    switch((opcode & 0xF0) >> 4) {
        case 0xE:
            switch(opcode & 0x000F) {
                case 0x0:
                    SETTXT("CLS");
                    break;
                case 0xE:
                    SETTXT("RET");
                    break;
            }
            break;
        case 0x0:
            SETTXT("NOP");
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void jp(uint16_t opcode) {
    uint16_t addr = opcode & 0x0FFF;
    SETTXT("JP 0x%04X", addr);
}

static void call(uint16_t opcode) {
    uint16_t addr = opcode & 0x0FFF;
    SETTXT("CALL 0x%04X", addr);
}

static void se_vi(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    SETTXT("SE V%X, %u", reg, imm);
}

static void sne_vi(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    SETTXT("SNE V%X, %u", reg, imm);
}

static void se_vv(uint16_t opcode) {
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    SETTXT("SE V%X, V%X", rega, regb);
}

static void ld_vi(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    SETTXT("LD V%X, %u", reg, imm);
}

static void add(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    SETTXT("ADD V%X, %u", reg, imm);
}

static void ops_x8(uint16_t opcode) {
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    switch(opcode & 0x000F) {
        case 0:
            SETTXT("LD V%X, V%X", rega, regb);
            break;
        case 1:
            SETTXT("OR V%X, V%X", rega, regb);
            break;
        case 2:
            SETTXT("AND V%X, V%X", rega, regb);
            break;
        case 3:
            SETTXT("XOR V%X, V%X", rega, regb);
            break;
        case 4:
            SETTXT("ADD V%X, V%X", rega, regb);
            break;
        case 5:
            SETTXT("SUB V%X, V%X", rega, regb);
            break;
        case 6:
            SETTXT("SHR V%X, V%X", rega, regb);
            break;
        case 7:
            SETTXT("SUBN V%X, V%X", rega, regb);
            break;
        case 0xE:
            SETTXT("SHL V%X, V%X", rega, regb);
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void sne_vv(uint16_t opcode) {
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    SETTXT("SNE V%X, V%X", rega, regb);
}

static void ld_i(uint16_t opcode) {
    uint16_t addr = opcode & 0x0FFF;
    SETTXT("LD I, 0x%04X", addr);
}

static void jp_v(uint16_t opcode) {
    uint16_t addr = opcode & 0x0FFF;
    SETTXT("JP V0, %04X", addr);
}

static void rnd(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    uint8_t imm = opcode & 0x00FF;
    SETTXT("RND V%X, %u", reg, imm);
}

static void drw(uint16_t opcode) {
    uint8_t rega = (opcode & 0x0F00) >> 8;
    uint8_t regb = (opcode & 0x00F0) >> 4;
    uint8_t bytes = (opcode & 0x000F);
    SETTXT("DRW V%X, V%X, %u", rega, regb, bytes);
}

static void skip(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    switch(opcode & 0xFF) {
        case 0x9E:
            SETTXT("SKP V%X", reg);
            break;
        case 0xA1:
            SETTXT("SKNP V%X", reg);
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void ops_xF(uint16_t opcode) {
    uint8_t reg = (opcode & 0x0F00) >> 8;
    switch(opcode & 0xFF) {
        case 0x07:
            SETTXT("LD V%X, DT", reg);
            break;
        case 0x0A:
            SETTXT("LD V%X, K", reg);
            break;
        case 0x15:
            SETTXT("LD DT, V%X", reg);
            break;
        case 0x18:
            SETTXT("LD ST, V%X", reg);
            break;
        case 0x1E:
            SETTXT("ADD I, V%X", reg);
            break;
        case 0x29:
            SETTXT("LD F, V%X", reg);
            break;
        case 0x33:
            SETTXT("LD B, V%X", reg);
            break;
        case 0x55:
            SETTXT("LD [I], V%X", reg);
            break;
        case 0x65:
            SETTXT("LD V%X, [I]", reg);
            break;
        default:
            UNKNOWN_OP(opcode);
            break;
    }
}

static void (*ch8_opcode_lut[16])(uint16_t opcode) = {
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

const char *ch8_disassemble(uint16_t opcode)
{
    uint8_t op_index = opcode >> 12;
    (*ch8_opcode_lut[op_index])(opcode);
    return g_disasm_buf;
}

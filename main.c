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
#include <stdlib.h>
#include <getopt.h>     // getopt()
#include <stdbool.h>

#include "chip8.h"
#include "log.h"
#include "chip8_dbg_server.h"
#include "chip8_emu.h"
#include "file.h"

const char *usage_general = "\
Usage: %s [OPTION]... FILE\n\n\
Options:\n\
\t-m MODE\t\tselect operation mode\n\t\t\t  valid modes are \"emu\", \"server\" and \"disasm\"\n\
\t-h\t\toutput this help message and exit\n\
\t-v\t\toutput version information and exit\n\
\n";

const char *usage_disasm = "\
Disassembler options:\n\
\t-a\t\toutput addresses with disassembly\n\
\t-i\t\toutput raw instructions with disassembly\n\
\n";

const char *usage_emu = "\
Emulator options:\n\
\n";

const char *usage_server = "\
Server options:\n\
\t-p\t\tlisten port (default: 8888)\n\
\n";

const char *version_text = "\
hnc8 %s\n\
Copyright (C) 2019 hundinui.\n\
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
";

static void print_usage(const char *exe_name)
{
    printf(usage_general, exe_name);
    printf("%s", usage_disasm);
    printf("%s", usage_emu);
    printf("%s", usage_server);
}

static void print_version(void)
{
    printf(version_text, HNC8_VERSION);
}

typedef enum {
    MODE_DISASM,
    MODE_EMULATOR,
    MODE_DEBUG
} mode_e;

int main(int argc, char **argv)
{
    int opt;
    mode_e mode = MODE_EMULATOR;
    bool opt_da_addr = false;
    bool opt_da_instr = false;
    int opt_dbg_port = 8888;
    double opt_emu_scale = 10.0;

    while((opt = getopt(argc, argv, "hvm:aip:s:")) != -1) {
        switch(opt) {
            /* General options */
            case ':':
                print_usage(argv[0]);
                LOG_ERROR("Missing argument value\n");
                return 1;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'm': /* Mode option */
                switch(optarg[0]) {
                    case 'e': /* emulator mode */
                        mode = MODE_EMULATOR;
                        LOG_DEBUG("Emulator mode\n");
                        break;
                    case 'd': /* disassembler mode */
                        mode = MODE_DISASM;
                        LOG_DEBUG("Disassembler mode\n");
                        break;
                    case 's':
                        mode = MODE_DEBUG;
                        LOG_DEBUG("Debug Server mode\n");
                        break;
                    default:
                        print_usage(argv[0]);
                        LOG_ERROR("Invalid mode: %s\n", optarg);
                        return 1;
                }
                break;
            /* Disassembler specific options */
            case 'a':
                LOG_DEBUG("Outputting addresses in disasm\n");
                opt_da_addr = true;
                break;
            case 'i':
                LOG_DEBUG("Outputting instructions in disasm\n");
                opt_da_instr = true;
                break;
            /* Debug server options */
            case 'p':
                opt_dbg_port = (uint16_t)strtol(optarg, NULL, 10);
                break;
            /* Emulator specific options */
            case 's':
                opt_emu_scale = strtod(optarg, NULL);
                LOG_DEBUG("Scale set to %f\n", opt_emu_scale);
                break;
        }
    }

    /* break out to debug mode here because we don't want to load a file yet */
    if(mode == MODE_DEBUG) {
        dbg_server_loop(opt_dbg_port);
        return 0;
    }

    uint8_t extra_args = argc - optind;
    if(extra_args != 1) {
        print_usage(argv[0]);
        LOG_ERROR("Please specify one input file on the command line\n");
        return 1;
    }

    size_t input_sz;
    uint16_t *input_mem;
    if(load_file(argv[optind], &input_mem, &input_sz) != 0) {
        return 1;
    }

    LOG_DEBUG("Loaded ROM %s, size %hu bytes\n", argv[optind], input_sz);

    switch(mode) {
        case MODE_DISASM:
            for(uint16_t i = 0; i < input_sz; ++i) {
                uint8_t op_h = input_mem[i] & 0xFF;
                uint8_t op_l = (input_mem[i] & 0xFF00) >> 8;
                uint16_t opcode = (op_h << 8) | op_l;

                if(opt_da_addr) printf("%04X:    ", i + VM_EXEC_START_ADDR);
                if(opt_da_instr) printf("%02X %02X    ", op_h, op_l);
                printf("%s\n", ch8_disassemble(opcode));
            }
            break;
        case MODE_EMULATOR:
            emu_loop(input_mem, input_sz, opt_emu_scale);
            break;
        case MODE_DEBUG:
            LOG_ERROR("this should not happen\n");
            break;
    }

    unload_file(input_mem, input_sz);

    return 0;
}

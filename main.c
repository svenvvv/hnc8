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
#include <getopt.h>     // getopt()
#include <sys/mman.h>   // mmap()
#include <sys/stat.h>   // fstat()
#include <fcntl.h>      // open()
#include <unistd.h>     // close()

#include "chip8.h"
#include "log.h"

static void print_usage(const char *exe_name)
{
    printf("\
Usage: %s [OPTION]... FILE\n\n\
Options:\n\
\t-h\toutput this help message and exit\n\
\t-v\toutput version information and exit\n\
", exe_name);
}

static void print_version(void)
{
    printf("\
hnc8 %s\n\
Copyright (C) 2019 hundinui.\n\
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
", HNC8_VERSION);
}

int main(int argc, char **argv)
{
    int opt;

    while((opt = getopt(argc, argv, "hv")) != -1) {
        switch(opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
        }
    }

    uint8_t extra_args = argc - optind;
    if(extra_args != 1) {
        print_usage(argv[0]);
        LOG_ERROR("Please specify one input file on the command line\n");
        return 1;
    }

    int input_fd = open(argv[optind], O_RDONLY);
    if(input_fd == -1) {
        LOG_ERROR("Error opening input file\n");
        return 1;
    }

    struct stat st;
    fstat(input_fd, &st);
    uint16_t input_sz = st.st_size;

    uint16_t *input_mem = mmap(0, input_sz, PROT_READ, MAP_PRIVATE, input_fd, 0);
    if(input_mem == MAP_FAILED) {
        fprintf(stderr, "Error reading input file\n");
        return 1;
    }

    LOG_DEBUG("Loaded ROM %s, size %hu bytes\n", argv[optind], input_sz);

    for(uint16_t i = 0; i < input_sz; ++i) {
        const uint16_t opcode = ((input_mem[i] & 0xFF00) >> 8) | ((input_mem[i] & 0xFF) << 8);
        printf("%04X:%04X %s\n", i, opcode, ch8_disassemble(opcode));
    }

    munmap(input_mem, input_sz);
    close(input_fd);

    return 0;
}

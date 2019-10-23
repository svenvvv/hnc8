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

#include "file.h"
#include <stdio.h>
#include "log.h"

#ifdef __linux__

#include <sys/mman.h>   // mmap()
#include <sys/stat.h>   // fstat()
#include <fcntl.h>      // open()
#include <unistd.h>     // close()

int load_file(const char *filename, uint16_t **ptr, size_t *size)
{
    int input_fd = open(filename, O_RDONLY);
    if(input_fd == -1) {
        LOG_ERROR("Error opening input file\n");
        return 1;
    }

    struct stat st;
    fstat(input_fd, &st);
    *size = st.st_size;

    *ptr = mmap(0, *size, PROT_READ, MAP_PRIVATE, input_fd, 0);
    if(*ptr == MAP_FAILED) {
        LOG_ERROR("Error reading input file\n");
        return 1;
    }

    close(input_fd);

    return 0;
}

void unload_file(uint16_t *ptr, size_t sz)
{
    munmap(ptr, sz);
}

#elif defined(_WIN32)

#include <stdlib.h>

int load_file(const char *filename, uint16_t **ptr, size_t *size)
{
    FILE *f = fopen(filename, "rb");
    if(f == NULL) {
        LOG_ERROR("Error reading input file\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    *ptr = malloc(*size + 1);
    if(*ptr == NULL) {
        LOG_ERROR("Error allocating memory\n");
        return 1;
    }

    fread(*ptr, 1, *size, f);
    fclose(f);

    return 0;
}

void unload_file(uint16_t *ptr, size_t sz)
{
    free(ptr);
}

#endif

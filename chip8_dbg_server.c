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

#include "chip8_dbg_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#ifdef __linux__
#   include <netinet/in.h>
#elif defined(_WIN32)
#   include <winsock2.h>
#endif

#include "log.h"
#include "chip8.h"
#include "file.h"

#define MAX_PACKET_SZ 512
#define MAX_TOKENS    16
#define MAX_STRARG_SZ 64
#define MAX_BPOINTS   16

#define EXAMINE_BYTES_PER_ROW 16

#define MSG_CURSOR      ">"
#define MSG_HELLO       "hnc8 debug server " HNC8_VERSION "\nType \"help\" for help or \"commands\" for a listing of commands.\n"
#define MSG_HELP        "TODO :)\n"
#define MSG_SHUTDOWN    "The server will shut down after client disconnect.\n"

#define MSG_ERR_FN              "Error executing function\n"
#define MSG_ERR_NO_FILE         "No file has been loaded.\nUse command \"load filename\" to load a program.\n"
#define MSG_ERR_ARGS_INVALID    "Invalid arguments for function\n"
#define MSG_ERR_ARGS_MISSING    "Too few arguments to call function\n"

#define tx_msg(msg) write(sockfd, msg, sizeof(msg))

typedef struct {
    uint8_t len;
    char *str;
} lex_t;

typedef struct {
    const char *cmd;
    const uint8_t cmd_len;
    const char *cmd_short;
    const uint8_t cmd_short_len;
    int (*fn)(int, lex_t *, int);
    const char *help_text;
} command_t;

static ch8_t g_vm;
static bool g_running = true;

static uint16_t g_bpoints[MAX_BPOINTS] = { 0 };
static uint8_t g_bpoints_count = 0;

static uint16_t *g_file = NULL;
static size_t g_file_sz = 0;

static void tx_printf(int sockfd, const char *fmt, ...)
{
    char buf[MAX_PACKET_SZ];
    size_t len = 0;
    va_list arg;

    va_start(arg, fmt);
    len = vsprintf(buf, fmt, arg);
    va_end(arg);

    write(sockfd, buf, len);
}

/* --- COMMANDS --- */

static int cmd_help(int sockfd, lex_t *argv, int argc)
{
    tx_msg(MSG_HELP);
    return 0;
}

static int cmd_shutdown(int sockfd, lex_t *argv, int argc)
{
    tx_msg(MSG_SHUTDOWN);
    g_running = false;
    return 0;
}

static int cmd_load(int sockfd, lex_t *argv, int argc)
{
    if(argc < 2) {
        tx_msg(MSG_ERR_ARGS_MISSING);
        return -1;
    }

    if(g_file != NULL) {
        unload_file(g_file, g_file_sz);
        g_file = NULL;
        g_file_sz = 0;
    }

    if(load_file(argv[1].str, &g_file, &g_file_sz) != 0) {
        tx_printf(sockfd, "Could not load file \"%.*s\"\n", argv[1].len, argv[1].str);
        return -1;
    }

    ch8_load(&g_vm, g_file, g_file_sz);

    tx_printf(sockfd, "Loaded \"%s\".\n", argv[1].str);

    return 0;
}

static int cmd_break(int sockfd, lex_t *argv, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return -1;
    }

    if(g_bpoints_count < MAX_BPOINTS) {
        int br_addr = 0;

        if(argc == 1) {
            br_addr = g_vm.pc;
        } else {
            char *endptr = NULL;
            br_addr = strtol(argv[1].str, &endptr, 0);
        }

        g_bpoints[g_bpoints_count++] = br_addr;

        tx_printf(sockfd, "Set breakpoint %i on 0x%x\n", g_bpoints_count-1, br_addr);

        return 0;
    } else {
        tx_printf(sockfd, "Maximum number of breakpoints reached (%d)\n", MAX_BPOINTS);

        return -1;
    }
}

static int cmd_lsbreak(int sockfd, lex_t *argv, int argc)
{
    if(g_bpoints_count == 0) {
        tx_printf(sockfd, "No breakpoints\n");
        return 0;
    }
    for(uint8_t i = 0; i < g_bpoints_count; ++i) {
        tx_printf(sockfd, "%i - 0x%x\n", i, g_bpoints[i]);
    }
    return 0;
}

static int cmd_rmbreak(int sockfd, lex_t *argv, int argc)
{
    if(g_bpoints_count == 0) {
        tx_printf(sockfd, "No breakpoints\n");
        return 0;
    }
    int num = 0;
    if(argc == 1) {
        num = --g_bpoints_count;
        g_bpoints[num] = 0;
    } else {
        char *endptr = NULL;
        num = strtol(argv[1].str, &endptr, 0);
        if(num > g_bpoints_count || num < 0) {
          tx_msg(MSG_ERR_ARGS_INVALID);
          return -1;
        }
        g_bpoints[num] = g_bpoints[--g_bpoints_count];
    }

    tx_printf(sockfd, "Removed breakpoint %i\n", num);

    return 0;
}

static int cmd_continue(int sockfd, lex_t *argv, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return -1;
    }

    for(;;) {
        ch8_tick(&g_vm);

        for(uint8_t i = 0; i < g_bpoints_count; ++i) {
            if(g_vm.pc == g_bpoints[i]) {
                tx_printf(sockfd, "Breakpoint %i hit at 0x%x\n", i, g_bpoints[i]);
                return 0;
            }
        }
    }

    return 0;
}

static int cmd_backtrace(int sockfd, lex_t *argv, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return -1;
    }

    uint8_t sp = g_vm.sp;

    if(sp) {
        tx_printf(sockfd, "Stack trace:\n");

        while(sp--) {
            tx_printf(sockfd, "0: 0x%04X\n", g_vm.stack[sp]);
        }
    } else {
        tx_printf(sockfd, "No addresses on stack\n");
    }

    return 0;
}

static int cmd_stepi(int sockfd, lex_t *argv, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return -1;
    }

    uint16_t opcode = ch8_get_op(&g_vm);
    ch8_exec(&g_vm, opcode);
    tx_printf(sockfd, "%s\n", ch8_disassemble(opcode));

    g_vm.pc += 2;

    return 0;
}

static int cmd_examine(int sockfd, lex_t *argv, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return -1;
    }
    if(argc < 2) {
        tx_msg(MSG_ERR_ARGS_MISSING);
        return -1;
    }

    char *endptr = NULL;
    uint16_t addr = strtol(argv[1].str, &endptr, 0);
    if(errno != 0 || endptr == argv[1].str) {
        return -1;
    }
    uint8_t count = EXAMINE_BYTES_PER_ROW;
    uint8_t rows = 1;
    if(argc == 3) {
        endptr = NULL;
        count = strtol(argv[2].str, &endptr, 0);
        if(errno != 0 || endptr == argv[1].str) {
            return -1;
        }

        rows = count / EXAMINE_BYTES_PER_ROW;
        if(count % EXAMINE_BYTES_PER_ROW != 0) {
            rows += 1;
        }
    }

    for(uint8_t x = 0; x < rows; ++x) {
        tx_printf(sockfd, "%04x: ", addr + (x * EXAMINE_BYTES_PER_ROW));
        for(uint8_t y = 0;
            y < EXAMINE_BYTES_PER_ROW &&
            y + (x * EXAMINE_BYTES_PER_ROW) < count;
            ++y) {
            tx_printf(sockfd, "%02x ",
                g_vm.ram[addr + (x * EXAMINE_BYTES_PER_ROW) + y]);
        }
        tx_printf(sockfd, "\n");
    }

    return 0;
}

static int cmd_registers(int sockfd, lex_t *argv, int argc)
{
    const char *v_reg_lut[] = {
        "v0", "v1", "v2", "v3", "v4",
        "v5", "v6", "v7", "v8", "v9",
        "v10", "v11", "v12", "v13", "v14",
        "v15"
    };
    const char *fmt_str = "%s\t0x%04x\t%i\n";
    const char *fmt_str_v = "%s\t0x%02x\t%i\n";

    if(argc == 1) { /* display registers */
        for(uint8_t i = 0; i < 16; ++i) {
            tx_printf(sockfd, fmt_str_v, v_reg_lut[i], g_vm.v[i], g_vm.v[i]);
        }
        tx_printf(sockfd, fmt_str, "i", g_vm.i, g_vm.i);
        tx_printf(sockfd, fmt_str, "pc", g_vm.pc, g_vm.pc);
        tx_printf(sockfd, fmt_str_v, "sp", g_vm.sp, g_vm.sp);
        tx_printf(sockfd, fmt_str_v, "dt", g_vm.tim_delay, g_vm.tim_delay);
        tx_printf(sockfd, fmt_str_v, "st", g_vm.tim_sound, g_vm.tim_sound);
        return 0;
    }

    if(argc >= 2) { /* display specific register */
        const char *name = NULL;
        const char *fmt = fmt_str;
        uint16_t val = 0;
        char *endptr = NULL;
        int vreg = 0;
        if(argc == 3) {
            val = strtol(argv[2].str, &endptr, 0);
            if(errno != 0 || endptr == argv[1].str) {
                tx_msg("Invalid value\n");
                return -1;
            }
        }

        switch(argv[1].str[0]) {
            case 'v':
            case 'V':
                endptr = NULL;
                vreg = strtol(argv[1].str + 1, &endptr, 0);
                if(errno != 0 || endptr == argv[1].str || vreg > 15 || vreg < 0) {
                    tx_msg("Invalid v register index\n");
                    return -1;
                }
                name = v_reg_lut[vreg];
                fmt = fmt_str_v;
                if(argc == 3) {
                    val = val > 0xFF ? 0xFF : val;
                    g_vm.v[vreg] = val;
                } else {
                    val = g_vm.v[vreg];
                }
                break;
            case 'i':
            case 'I':
                name = "i";
                if(argc == 3) {
                    g_vm.i = val;
                } else {
                    val = g_vm.i;
                }
                break;
            case 'p':
            case 'P':
                name = "pc";
                if(argc == 3) {
                    g_vm.pc = val;
                } else {
                    val = g_vm.pc;
                }
                break;
            case 's':
            case 'S':
                if(argv[1].str[1] == 'p') {
                    name = "sp";
                    if(argc == 3) {
                        val = val > 0xFF ? 0xFF : val;
                        g_vm.sp = val;
                    } else {
                        val = g_vm.sp;
                    }
                } else if(argv[1].str[1] == 't') {
                    name = "st";
                    if(argc == 3) {
                        val = val > 0xFF ? 0xFF : val;
                        g_vm.tim_sound = val;
                    } else {
                        val = g_vm.tim_sound;
                    }
                } else {
                    tx_msg("Invalid register name\n");
                    return -1;
                }
                break;
            case 'd':
            case 'D':
                name = "dt";
                if(argc == 3) {
                    val = val > 0xFF ? 0xFF : val;
                    g_vm.tim_delay = val;
                } else {
                    val = g_vm.tim_delay;
                }
                break;
            default:
                tx_msg(MSG_ERR_ARGS_INVALID);
                return -1;
        }

        if(argc == 3) {
            tx_printf(sockfd, "Set %s to 0x%04x (%u)\n", name, val, val);
        } else {
            tx_printf(sockfd, fmt, name, val, val);
        }

        return 0;
    }

    return -1;
}

static int cmd_setkey(int sockfd, lex_t *argv, int argc)
{
    if(argc < 2) {
        return -1;
    }
    char *endptr = NULL;
    int keyvalue = strtol(argv[1].str, &endptr, 0);
    if(keyvalue > 15 || keyvalue < 0) {
        return -1;
    }
    g_vm.keys[keyvalue] = !g_vm.keys[keyvalue];
    return 0;
}

static int cmd_keys(int sockfd, lex_t *argv, int argc)
{
    uint8_t *k = g_vm.keys;

    for(uint8_t i = 0; i < 4; ++i) {
        tx_printf(sockfd, "%i %i %i %i\n", k[4*i], k[4*i+1], k[4*i+2], k[4*i+3]);
    }

    return 0;
}

static int cmd_disassemble(int sockfd, lex_t *argv, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return -1;
    }

    /*
     * lazy hack, save old PC and then increment PC and use
     * ch8_get_op to handle the opcode decoding
     */
    uint16_t old_pc = g_vm.pc;
    uint16_t count = 6;

    if(argc == 2) {
        char *endptr = NULL;
        count = strtol(argv[1].str, &endptr, 0);
    }
    if(argc == 3) {
        char *endptr = NULL;
        g_vm.pc = strtol(argv[2].str, &endptr, 0);
    }

    for(uint16_t i = 0; i < count; ++i) {
        uint16_t op = ch8_get_op(&g_vm);
        const char *disstr = ch8_disassemble(op);
        tx_printf(sockfd, "%X %s\n", g_vm.pc, disstr);

        g_vm.pc += 2;
    }

    g_vm.pc = old_pc;

    return 0;
}

static int cmd_screen(int sockfd, lex_t *argv, int argc)
{
    const char *border = "════════════════════════════════════════════════════════════════";
    for(uint8_t y = 0; y < VM_SCREEN_HEIGHT; ++y) {
        if(y == 0) {
            tx_printf(sockfd, "╔%s╗\n", border);
        }
        tx_printf(sockfd, "║");


        for(uint8_t x = 0; x < VM_SCREEN_WIDTH; ++x) {
            uint8_t value = g_vm.vram[x + (y * VM_SCREEN_WIDTH)];
            char val = value > 0 ? 'X' : ' ';
            tx_printf(sockfd, "%c", val);
        }

        tx_printf(sockfd, "║\n");
    }
    tx_printf(sockfd, "╚%s╝\n", border);

    return 0;
}

/*
 * only one prototyped because we need to read the list we are pointing
 * to this from :)
 */
static int cmd_commands(int sockfd, lex_t *argv, int argc);

#define DEF_CMD(cmd, shortcmd, fn, help_text) \
{ cmd, sizeof(cmd) - 1, shortcmd, shortcmd == NULL ? 0: sizeof(shortcmd) - 1, fn, help_text }

static const command_t commands[] = {
    /*       NAME        SHORT     FUNC               HELP */
    DEF_CMD("help",         "h",    cmd_help,         "- Display help message"),
    DEF_CMD("shutdown",     NULL,   cmd_shutdown,     "- Shut down the server"),
    DEF_CMD("load",         "l",    cmd_load,         "filename - Load ROM into VM"),
    DEF_CMD("break",        "b",    cmd_break,        "[address] - Add a breakpoint"),
    DEF_CMD("lsbreak",      "lb",   cmd_lsbreak,      "- List breakpoints"),
    DEF_CMD("rmbreak",      "rb",   cmd_rmbreak,      "[index] - Remove breakpoint at address, or latest"),
    DEF_CMD("continue",     "c",    cmd_continue,     "- Continue execution until breakpoint"),
    DEF_CMD("backtrace",    "bt",   cmd_backtrace,    "- Display the stack trace"),
    DEF_CMD("stepi",        "si",   cmd_stepi,        "[count] - Step forward"),
    DEF_CMD("examine",      "x",    cmd_examine,      "address [count] - Examine memory"),
    DEF_CMD("commands",     NULL,   cmd_commands,     "- Display this info about commands"),
    DEF_CMD("registers",    "r",    cmd_registers,    "[register] [value] - Display and edit VM registers"),
    DEF_CMD("setkey",       "sk",   cmd_setkey,       "keynum - Toggle a keypad key state"),
    DEF_CMD("keys",         "k",    cmd_keys,         "- Display keypad state"),
    DEF_CMD("disassemble",  "da",   cmd_disassemble,  "[count] [address] - Disassemble opcodes"),
    DEF_CMD("screen",       "scr",  cmd_screen,       "- Display screen contents")
};
#define commands_count (sizeof(commands) / sizeof(commands[0]))

static int cmd_commands(int sockfd, lex_t *argv, int argc)
{
    tx_printf(sockfd, "Available commands:\n");
    for(uint8_t i = 0; i < commands_count; ++i) {
        tx_printf(sockfd, "  %s %s\n", commands[i].cmd, commands[i].help_text);
    }
    return 0;
}

static inline void decode_msg(int sockfd, char *msg, size_t len)
{
    uint8_t lex_i = 0;
    lex_t lex[MAX_TOKENS] = { 0 };

    /* clean the newline */
    if(msg[len - 1] == '\n') {
        msg[len - 1] = '\0';
        len -= 1;
    }

    /* split the message we got into lexemes */
    char *start = msg;
    char *end;

    while((end = memchr(start, ' ', len)) != NULL && end < msg + len) {
        uint8_t word_len = end - start;
        if(word_len == 0) {
            start = end + 1;
            continue;
        }
        lex[lex_i].len = word_len;
        lex[lex_i].str = start;
        lex_i += 1;
        start = end + 1;
    }
    lex[lex_i].len = (msg + len) - start;
    lex[lex_i].str = start;
    lex_i += 1;

#ifdef DEBUG
    for(int i = 0; i < lex_i; ++i) {
        printf("%i - %.*s\n", lex[i].len, lex[i].len, lex[i].str);
    }
#endif

    const lex_t *net_cmd = &lex[0];
    for(uint8_t i = 0; i < commands_count; ++i) {
        const command_t *cmd = &commands[i];
        bool cmd_match = net_cmd->len == cmd->cmd_len;
        bool short_match = cmd->cmd_short_len > 0 ?
            net_cmd->len == cmd->cmd_short_len : false;
        if(
        (cmd_match && strncmp(net_cmd->str, cmd->cmd, cmd->cmd_len) == 0) ||
        (short_match && strncmp(net_cmd->str, cmd->cmd_short, cmd->cmd_short_len) == 0)
        ) {
            if((*cmd->fn)(sockfd, lex, lex_i) < 0) {
                tx_msg(MSG_ERR_FN);
            }
            return;
        }
    }

    tx_printf(sockfd, "Unknown command \"%s\"\n", net_cmd->str);
}

static void client_handler(int sockfd)
{
    char buf[MAX_PACKET_SZ];

    tx_msg(MSG_HELLO);

    for(;;) {
        tx_msg(MSG_CURSOR);

        memset(buf, 0, MAX_PACKET_SZ);

        size_t sz = read(sockfd, buf, MAX_PACKET_SZ);
        if(sz == 0) break;

        printf("Got: %s", buf);

        decode_msg(sockfd, buf, sz);
    }
}

void dbg_server_loop(uint16_t port)
{
    int sockfd;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        LOG_ERROR("Error creating socket\n");
        return;
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        LOG_ERROR("Error binding to socket\n");
        return;
    }

    if((listen(sockfd, 5)) != 0) {
        LOG_ERROR("Error listening on socket\n");
        return;
    }

    LOG("Debug server listening on localhost:%i\n", port);

#ifdef __linux__
    unsigned int len = sizeof(cli);
#elif defined(_WIN32)
    int len = sizeof(cli);
#endif

    /* reset emu */
    ch8_init(&g_vm);

    while(g_running) {
        int connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
        if(connfd < 0) {
            LOG_ERROR("Error accepting client\n");
            return;
        } else {
            LOG("Client connected\n");
        }

        client_handler(connfd);

        close(connfd);

        LOG("Client disconnected\n");
    }

    LOG("Shutting down\n");

    close(sockfd);

    return;
}

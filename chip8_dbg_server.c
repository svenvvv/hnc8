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
#include <netinet/in.h>
#include <unistd.h>

#include "log.h"
#include "chip8.h"
#include "file.h"

#define MAX_PACKET_SZ 512
#define MAX_TOKENS    16

#define MSG_CURSOR      ">"
#define MSG_HELLO       "hnc8 debug server " HNC8_VERSION "\nType \"help\" for help.\n"
#define MSG_HELP        "TODO :)\n"
#define MSG_SHUTDOWN    "The server will shut down after client disconnect.\n"
#define MSG_OK          "OK\n"

#define MSG_ERR_NO_FILE      "No file has been loaded.\nUse command \"load filename\" to load a program.\n"

#define tx_msg(msg) write(sockfd, msg, sizeof(msg))

typedef enum {
    TYPE_INT,
    TYPE_STRING
} arg_type_e;

typedef struct {
    arg_type_e type;
    union {
        int16_t intval;
        char strval[255];
    };
} arg_t;

typedef struct {
    uint8_t len;
    char *str;
} lex_t;

typedef struct {
    const char *cmd;
    const uint8_t cmd_len;
    const char *cmd_short;
    const uint8_t cmd_short_len;
    const char *args_type;
    void (*fn)(int, arg_t *, int);
} command_t;

ch8_t g_vm;
bool g_running = true;

uint16_t *g_file = NULL;
size_t g_file_sz = 0;

/* --- COMMANDS --- */

static void cmd_help(int sockfd, arg_t *args, int argc)
{
    tx_msg(MSG_HELP);
}

static void cmd_shutdown(int sockfd, arg_t *args, int argc)
{
    tx_msg(MSG_OK);
    tx_msg(MSG_SHUTDOWN);
    g_running = false;
}

static void cmd_load(int sockfd, arg_t *args, int argc)
{
    tx_msg(MSG_OK);
}

static void cmd_break(int sockfd, arg_t *args, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return;
    }
    tx_msg(MSG_OK);
}

static void cmd_continue(int sockfd, arg_t *args, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return;
    }
    tx_msg(MSG_OK);
}

static void cmd_backtrace(int sockfd, arg_t *args, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return;
    }
    tx_msg(MSG_OK);
}

static void cmd_stepi(int sockfd, arg_t *args, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return;
    }
    tx_msg(MSG_OK);
}

static void cmd_examine(int sockfd, arg_t *args, int argc)
{
    if(g_file == NULL) {
        tx_msg(MSG_ERR_NO_FILE);
        return;
    }
    tx_msg(MSG_OK);
}

#define DEF_CMD(cmd, shortcmd, args, fn) { cmd, sizeof(cmd) - 1, shortcmd, sizeof(shortcmd) - 1, args, fn }

static const command_t commands[] = {
    /*       NAME        SHORT    ARGS    FUNC */
    DEF_CMD("help",       "h",    NULL,   cmd_help),
    DEF_CMD("shutdown",   NULL,   NULL,   cmd_shutdown),
    DEF_CMD("load",       "l",    "s",    cmd_load),
    DEF_CMD("break",      "b",    "i",    cmd_break),
    DEF_CMD("continue",   "c",    NULL,   cmd_continue),
    DEF_CMD("backtrace",  "bt",   NULL,   cmd_backtrace),
    DEF_CMD("stepi",      "si",   "i?",   cmd_stepi),
    DEF_CMD("examine",    "x",    "i",    cmd_examine)
};
#define commands_count (sizeof(commands) / sizeof(commands[0]))

static inline void tx_printf(int sockfd, const char *fmt, ...)
{
    char buf[MAX_PACKET_SZ];
    size_t len = 0;
    va_list arg;

    va_start(arg, fmt);
    len = vsprintf(buf, fmt, arg);
    va_end(arg);

    write(sockfd, buf, len);
}

static int parse_args(const command_t *cmd, arg_t *argv, const lex_t *lexemes)
{
    const char *ptr = cmd->args_type;
    uint8_t i = 0;
    printf("parsing tokens: ");
    while(*ptr != '\0') {
        printf("%c, ", *ptr);
        ++ptr;
        ++i;
    }
    printf("\n");
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
        uint8_t len = end - start;
        if(len == 0) {
            start = end + 1;
            continue;
        }
        lex[lex_i].len = len;
        lex[lex_i].str = start;
        lex_i += 1;
        start = end + 1;
    }
    lex[lex_i].len = (msg + len) - start;
    lex[lex_i].str = start;
    lex_i += 1;

    for(int i = 0; i < lex_i; ++i) {
        printf("%i - %.*s\n", lex[i].len, lex[i].len, lex[i].str);
    }

    const lex_t *net_cmd = &lex[0];
    for(uint8_t i = 0; i < commands_count; ++i) {
        const command_t *cmd = &commands[i];
        bool cmd_match = net_cmd->len == cmd->cmd_len;
        bool short_match = net_cmd->len == cmd->cmd_short_len;
        if(
        (cmd_match && strncmp(net_cmd->str, cmd->cmd, cmd->cmd_len) == 0) ||
        (short_match && strncmp(net_cmd->str, cmd->cmd_short, cmd->cmd_short_len) == 0)
        ) {
            if(cmd->args_type == NULL) {
                (*cmd->fn)(sockfd, NULL, 0);
                return;
            } else {
                arg_t argv[MAX_TOKENS];
                uint8_t c = parse_args(cmd, argv, lex);
                (*cmd->fn)(sockfd, argv, c);
                return;
            }
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
    int connfd;
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

    unsigned int len = sizeof(cli);

    while(g_running) {
        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
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

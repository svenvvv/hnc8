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

#include "chip8_emu.h"
#include <stdbool.h>
#include <stdio.h>
#define __USE_MISC
#include <unistd.h> // usleep()
#undef __USE_MISC
#include <GLFW/glfw3.h>

#include "chip8.h"
#include "log.h"

#define FPS 60
#define FPS_FRAMETIME (1000 / FPS)

static GLFWwindow *g_win;
static GLuint g_fb_id;
static uint16_t g_w, g_h;
static ch8_t g_vm;

static void set_key(uint8_t i, uint8_t state)
{
    g_vm.keys[i] = state;
}

static void win_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    uint8_t state = action == GLFW_RELEASE ? 0 : 1;
    switch(key) {
        case GLFW_KEY_1:
            set_key(1, state);
            break;
        case GLFW_KEY_2:
            set_key(2, state);
            break;
        case GLFW_KEY_3:
            set_key(3, state);
            break;
        case GLFW_KEY_4:
            set_key(0xC, state);
            break;

        case GLFW_KEY_Q:
            set_key(4, state);
            break;
        case GLFW_KEY_W:
            set_key(5, state);
            break;
        case GLFW_KEY_E:
            set_key(6, state);
            break;
        case GLFW_KEY_R:
            set_key(0xD, state);
            break;

        case GLFW_KEY_A:
            set_key(7, state);
            break;
        case GLFW_KEY_S:
            set_key(8, state);
            break;
        case GLFW_KEY_D:
            set_key(9, state);
            break;
        case GLFW_KEY_F:
            set_key(0xE, state);
            break;

        case GLFW_KEY_Z:
            set_key(0xA, state);
            break;
        case GLFW_KEY_X:
            set_key(0, state);
            break;
        case GLFW_KEY_C:
            set_key(0xB, state);
            break;
        case GLFW_KEY_V:
            set_key(0xF, state);
            break;
        default:
            break;
    }
}

static void win_init(uint16_t w, uint16_t h)
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to init glfw\n");
        return;
    }

    g_win = glfwCreateWindow(w, h, "hnc8", NULL, NULL);
    if (!g_win) {
        LOG_ERROR("Failed to create window\n");
        glfwTerminate();
        return;
    }

    glfwSetKeyCallback(g_win, win_key_callback);
    glfwMakeContextCurrent(g_win);

    /* Generate framebuffer texture for output */
    glGenTextures(1, &g_fb_id);
    glBindTexture(GL_TEXTURE_2D, g_fb_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, VM_SCREEN_WIDTH,
                 VM_SCREEN_HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glClearColor(0xFF, 0xFF, 0xFF, 0xFF);
}

static void win_destroy(void)
{
    glDeleteTextures(1, &g_fb_id);
    glfwDestroyWindow(g_win);
    glfwTerminate();
}

static void win_render(void)
{
    glViewport(0, 0, g_w, g_h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, g_w, 0, g_h, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, g_fb_id);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 1); glVertex2i(0, 0);
    glTexCoord2i(0, 0); glVertex2i(0, g_h);
    glTexCoord2i(1, 0); glVertex2i(g_w, g_h);
    glTexCoord2i(1, 1); glVertex2i(g_w, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    glfwSwapBuffers(g_win);
}

void emu_loop(const uint16_t *rom, uint16_t rom_sz, double scale)
{
    g_w = VM_SCREEN_WIDTH * scale;
    g_h = VM_SCREEN_HEIGHT * scale;

    win_init(g_w, g_h);

    ch8_load(&g_vm, rom, rom_sz);

    double t_d = 0.0;
    double t_start = glfwGetTime() * 1000000.0;
    double t_end = 0.0;
    do {
        t_d = t_end - t_start;

        glfwPollEvents();

        uint16_t op = ch8_get_op(&g_vm);
        printf("%s\n", ch8_disassemble(op));

        ch8_tick(&g_vm);
        ch8_tick_timers(&g_vm);

        if(g_vm.vram_updated) {
            /* update framebuffer */
            glBindTexture(GL_TEXTURE_2D, g_fb_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VM_SCREEN_WIDTH, VM_SCREEN_HEIGHT,
                             GL_LUMINANCE, GL_UNSIGNED_BYTE, g_vm.vram);
            glBindTexture(GL_TEXTURE_2D, 0);

            g_vm.vram_updated = false;
        }

        win_render();

        if(t_d < FPS_FRAMETIME) {
            usleep(FPS_FRAMETIME - t_d);
        }

        t_start = t_end;
        t_end = glfwGetTime() * 1000000.0;
    } while(!glfwWindowShouldClose(g_win));

    win_destroy();
}

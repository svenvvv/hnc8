#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../chip8.h"

#define COL_RST "\033[0m"
#define COL_RED "\033[1;31m"
#define COL_GRN "\033[1;32m"

/*
 * Testbed boilerplate code
 */
#define TEST(...) \
{ \
    const char *name; \
    bool test = true; \
    ch8_t vm = {{ 0 }}; \
    __VA_ARGS__ \
    printf("  %s\t\t\t%s\n", name, test ? COL_GRN "✓ PASS" COL_RST : COL_RED "✗ FAIL" COL_RST); \
}

#define TESTGROUP(name) printf("%s\n", name);

#define EXPECT(...) \
if(!(__VA_ARGS__)) { test = false; }

static const uint8_t ram_zero[VM_RAM_SIZE] = { 0 };

int main(void)
{
    printf("Running hnc8 instruction set tests...\n\n");

    { /* 0x0xxx */
        TESTGROUP("0x0xxx");
        TEST(
            name = "CLS";

            const size_t vr_sz = VM_SCREEN_WIDTH * VM_SCREEN_HEIGHT;
            memset(vm.vram, 0xFF, vr_sz);
            ch8_exec(&vm, 0x00E0);

            EXPECT(memcmp(vm.vram, ram_zero, vr_sz) == 0);
        );
        TEST(
            name = "RET";

            vm.sp = 1;
            vm.stack[0] = 0xBEEF;

            ch8_exec(&vm, 0x00EE);

            EXPECT(vm.sp == 0);
            EXPECT(vm.pc == 0xBEEF);
        );
    }

    { /* 0x1xxx */
        TESTGROUP("0x1xxx");
        TEST(
            name = "JP";
            ch8_exec(&vm, 0x1123);

            EXPECT(vm.pc == 0x123);
        );
    }

    { /* 0x2xxx */
        TESTGROUP("0x2xxx");
        TEST(
            name = "CALL";

            vm.pc = 0xBEEF;
            ch8_exec(&vm, 0x2FFF);

            EXPECT(vm.sp == 1);
            EXPECT(vm.stack[0] == 0xBEEF);
            EXPECT(vm.pc == 0xFFF);
        );
    }

    { /* 0x3xxx */
        TESTGROUP("0x3xxx");
        TEST(
            name = "SE imm skip";

            vm.v[0] = 0xBE;
            ch8_exec(&vm, 0x30BE);

            EXPECT(vm.pc == 2);
        );

        TEST(
            name = "SE imm noskip";

            vm.v[0] = 0xEE;
            ch8_exec(&vm, 0x30BE);

            EXPECT(vm.pc == 0);
        );
    }

    { /* 0x4xxx */
        TESTGROUP("0x4xxx");
        TEST(
            name = "SNE imm skip";

            vm.v[0] = 0xBB;
            ch8_exec(&vm, 0x40BE);

            EXPECT(vm.pc == 2);
        );

        TEST(
            name = "SNE imm noskip";

            vm.v[0] = 0xBE;
            ch8_exec(&vm, 0x40BE);

            EXPECT(vm.pc == 0);
        );
    }

    { /* 0x5xxx */
        TESTGROUP("0x5xxx");
        TEST(
            name = "SE reg skip";

            vm.v[0] = 0xBB;
            vm.v[1] = 0xBB;
            ch8_exec(&vm, 0x5010);

            EXPECT(vm.pc == 2);
        );

        TEST(
            name = "SE reg noskip";

            vm.v[0] = 0xBE;
            vm.v[1] = 0xBB;
            ch8_exec(&vm, 0x5010);

            EXPECT(vm.pc == 0);
        );
    }

    { /* 0x6xxx */
        TESTGROUP("0x6xxx");
        TEST(
            name = "LD imm";

            ch8_exec(&vm, 0x60BB);

            EXPECT(vm.v[0] == 0xBB);
        );
    }

    { /* 0x7xxx */
        TESTGROUP("0x7xxx");
        TEST(
            name = "ADD imm";

            vm.v[0] = 10;
            ch8_exec(&vm, 0x700A);

            EXPECT(vm.v[0] == 20);
        );
    }

    { /* 0x8xxx */
        TESTGROUP("0x8xxx");
        TEST(
            name = "LD reg";

            vm.v[1] = 10;
            ch8_exec(&vm, 0x8010);

            EXPECT(vm.v[0] == 10);
        );

        TEST(
            name = "OR";

            vm.v[0] = 0x0F;
            vm.v[1] = 0xF0;
            ch8_exec(&vm, 0x8011);

            EXPECT(vm.v[0] == 0xFF);
        );

        TEST(
            name = "AND";

            vm.v[0] = 0xFF;
            vm.v[1] = 0xF0;
            ch8_exec(&vm, 0x8012);

            EXPECT(vm.v[0] == 0x0F);
        );

        TEST(
            name = "XOR";

            vm.v[0] = 0x11;
            vm.v[1] = 0x22;
            ch8_exec(&vm, 0x8013);

            EXPECT(vm.v[0] == 0x33);
        );

        TEST(
            name = "ADD reg";

            vm.v[0] = 0xFF;
            vm.v[1] = 10;
            ch8_exec(&vm, 0x8014);

            EXPECT(vm.v[0] == 10);
            EXPECT(vm.v[0xF] == 1);
        );

        TEST(
            name = "SUB";

            vm.v[0] = 0xFF;
            vm.v[1] = 10;
            ch8_exec(&vm, 0x8015);

            EXPECT(vm.v[0] == (0xFF - 10));
            EXPECT(vm.v[0xF] == 1);
        );

        TEST(
            name = "SHR";

            vm.v[0] = 0x03;
            ch8_exec(&vm, 0x8016);

            EXPECT(vm.v[0] == 0x01);
            EXPECT(vm.v[0xF] == 1);
        );

        TEST(
            name = "SUBN";

            vm.v[0] = 1;
            vm.v[1] = 3;
            ch8_exec(&vm, 0x8017);

            EXPECT(vm.v[0] == 2);
            EXPECT(vm.v[0xF] == 1);
        );

        TEST(
            name = "SHL";

            vm.v[0] = 0x81;
            vm.v[1] = 3;
            ch8_exec(&vm, 0x801E);

            EXPECT(vm.v[0] == 2);
            EXPECT(vm.v[0xF] == 1);
        );
    }

    {
        TESTGROUP("0x9xxx");
        TEST(
            name = "SNE reg skip";

            vm.v[0] = 0x00;
            vm.v[1] = 0xFF;
            ch8_exec(&vm, 0x9010);

            EXPECT(vm.pc == 2);
        );

        TEST(
            name = "SNE reg noskip";

            vm.v[0] = 0xFF;
            vm.v[1] = 0xFF;
            ch8_exec(&vm, 0x9010);

            EXPECT(vm.pc == 0);
        );
    }

    {
        TESTGROUP("0xAxxx");
        TEST(
            name = "LD I";

            ch8_exec(&vm, 0xABEF);

            EXPECT(vm.i == 0xBEF);
        );
    }

    {
        TESTGROUP("0xBxxx");
        TEST(
            name = "JP V0";

            vm.v[0] = 0xFF;
            ch8_exec(&vm, 0xBB00);

            EXPECT(vm.pc == 0xBFF);
        );
    }

    {
        TESTGROUP("0xCxxx");
        TEST(
            name = "JP V0";

            vm.v[0] = 0x00;
            ch8_exec(&vm, 0xC0FF);

            EXPECT(vm.v[0] != 0);
        );
    }

    {
        TESTGROUP("0xDxxx");
        TEST(
            name = "DRW";

            vm.v[0] = 0x00;
            ch8_exec(&vm, 0xD0FF);

            EXPECT(0);
        );
    }

    {
        TESTGROUP("0xExxx");
        TEST(
            name = "SKP";

            vm.v[0] = 0;
            vm.keys[0] = 0xFF;
            ch8_exec(&vm, 0xE09E);

            EXPECT(vm.pc == 2);
        );

        TEST(
            name = "SKNP";

            vm.v[0] = 0;
            vm.keys[0] = 0;
            ch8_exec(&vm, 0xE09E);

            EXPECT(vm.pc == 2);
        );
    }

    {
        TESTGROUP("0xFxxx");
        TEST(
            name = "LD Vx, DT";

            vm.tim_delay = 0xBA;
            ch8_exec(&vm, 0xF007);

            EXPECT(vm.v[0] == 0xBA);
        );

        /* TODO: LD Vx, K not tested FIXME? */

        TEST(
            name = "LD DT, Vx";

            vm.v[0] = 0xBA;
            ch8_exec(&vm, 0xF015);

            EXPECT(vm.tim_delay == 0xBA);
        );

        TEST(
            name = "LD ST, Vx";

            vm.v[0] = 0xBA;
            ch8_exec(&vm, 0xF018);

            EXPECT(vm.tim_sound == 0xBA);
        );

        TEST(
            name = "ADD I, Vx";

            vm.i = 10;
            vm.v[0] = 10;
            ch8_exec(&vm, 0xF01E);

            EXPECT(vm.i == 20);
        );

        /* TODO: LD F, Vx not tested FIXME? */

        /* TODO: LD B, Vx not tested FIXME? */

        TEST(
            name = "LD [I], Vx";

            vm.v[0] = 'h';
            vm.v[1] = 'e';
            vm.v[2] = 'l';
            vm.v[3] = 'l';
            vm.v[4] = 'o';
            vm.v[5] = ' ';
            vm.v[6] = 'c';
            vm.v[7] = 'h';
            vm.v[8] = 'i';
            vm.v[9] = 'p';
            vm.v[10] = '8';
            vm.v[11] = 0;
            ch8_exec(&vm, 0xFC55);

            EXPECT(strncmp((char*)vm.ram, "hello chip8", 12) == 0);
        );

        TEST(
            name = "LD [I], Vx";

            strncpy(vm.ram, "hello chip8", 12);
            ch8_exec(&vm, 0xFC65);

            EXPECT(vm.v[0] == 'h');
            EXPECT(vm.v[1] == 'e');
            EXPECT(vm.v[2] == 'l');
            EXPECT(vm.v[3] == 'l');
            EXPECT(vm.v[4] == 'o');
            EXPECT(vm.v[5] == ' ');
            EXPECT(vm.v[6] == 'c');
            EXPECT(vm.v[7] == 'h');
            EXPECT(vm.v[8] == 'i');
            EXPECT(vm.v[9] == 'p');
            EXPECT(vm.v[10] == '8');
            EXPECT(vm.v[11] == 0);
        );
    }

    return 0;
}

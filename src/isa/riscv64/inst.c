#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/ifetch.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum
{
    TYPE_R,
    TYPE_I,
    TYPE_S,
    TYPE_B,
    TYPE_U,
    TYPE_J,
    TYPE_N, // none
};

#define src1R()         \
    do                  \
    {                   \
        *src1 = R(rs1); \
    } while (0)
#define src2R()         \
    do                  \
    {                   \
        *src2 = R(rs2); \
    } while (0)
#define immI()                            \
    do                                    \
    {                                     \
        *imm = SEXT(BITS(i, 31, 20), 12); \
    } while (0)
#define immS()                                                   \
    do                                                           \
    {                                                            \
        *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
    } while (0)
#define immB()                                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        *imm = SEXT(BITS(i, 31, 31), 1) << 11 | BITS(i, 7, 7) << 10 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8); \
    } while (0)
#define immU()                                  \
    do                                          \
    {                                           \
        *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
    } while (0)
#define immJ()                                                                                                   \
    do                                                                                                           \
    {                                                                                                            \
        *imm = SEXT(BITS(i, 31, 31), 1) << 19 | BITS(i, 19, 12) << 11 | BITS(i, 20, 20) << 10 | BITS(i, 30, 21); \
    } while (0)

static void decode_operand(Decode *s, int *dest, word_t *src1, word_t *src2, word_t *imm, int type)
{
    uint32_t i = s->isa.inst.val;
    int rd = BITS(i, 11, 7);
    int rs1 = BITS(i, 19, 15);
    int rs2 = BITS(i, 24, 20);
    *dest = rd;
    switch (type)
    {
    case TYPE_R:
        src1R();
        src2R();
        break;
    case TYPE_I:
        src1R();
        immI();
        break;
    case TYPE_S:
        src1R();
        src2R();
        immS();
        break;
    case TYPE_B:
        src1R();
        src2R();
        immB();
        break;
    case TYPE_U:
        immU();
        break;
    case TYPE_J:
        immJ();
        break;
    }
}

static int decode_exec(Decode *s)
{
    int dest = 0;
    word_t src1 = 0, src2 = 0, imm = 0;
    s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)               \
    {                                                                      \
        decode_operand(s, &dest, &src1, &src2, &imm, concat(TYPE_, type)); \
        __VA_ARGS__;                                                       \
    }

#define JMP() s->dnpc = s->pc + imm * 2;
#define GES(_a, _b) ((int64_t)_a >= (int64_t)_b)
#define LTS(_a, _b) ((int64_t)_a < (int64_t)_b)

    INSTPAT_START();
    INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(dest) = src1 + src2);
    INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(dest) = src1 + imm);
    INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw, I, R(dest) = SEXT(BITS(src1 + imm, 31, 0), 32));
    INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw, R, R(dest) = SEXT(BITS(src1 + src2, 31, 0), 32));
    INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(dest) = s->pc + imm);
    INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, if (src1 == src2) JMP());
    INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if (GES(src1, src2)) JMP());
    INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (src1 >= src2) JMP());
    INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, if (LTS(src1, src2)) JMP());
    INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, if (src1 < src2) JMP());
    INSTPAT("??????? ????? ????? 001 ????? 11000 11", bnq, B, if (src1 != src2) JMP());
    INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R(dest) = s->snpc; JMP());
    INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, R(dest) = s->snpc; s->dnpc = (src1 + imm) & ~1;);
    INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(dest) = SEXT(Mr(src1 + imm, 1), 8));
    INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(dest) = Mr(src1 + imm, 1));
    INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, R(dest) = Mr(src1 + imm, 8));
    INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(dest) = SEXT(Mr(src1 + imm, 2), 16));
    INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(dest) = Mr(src1 + imm, 2));
    INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(dest) = imm);
    INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(dest) = SEXT(Mr(src1 + imm, 4), 32));
    INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu, I, R(dest) = Mr(src1 + imm, 4));
    INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd, S, Mw(src1 + imm, 8, src2));
    INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R(dest) = LTS(src1, src2));
    INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R(dest) = LTS(src1, imm));
    INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(dest) = src1 < imm);
    INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R(dest) = src1 < src2);
    INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(dest) = src1 - src2);
    INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw, R, R(dest) = SEXT(BITS(src1 - src2, 31, 0), 32));
    INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, BITS(src2, 31, 0)));

    INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
    INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));
    INSTPAT_END();

    R(0) = 0; // reset $zero to 0

    return 0;
}

int isa_exec_once(Decode *s)
{
    s->isa.inst.val = inst_fetch(&s->snpc, 4);
    return decode_exec(s);
}

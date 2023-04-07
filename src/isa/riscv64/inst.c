#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/ifetch.h>
#include <ftrace.h>
#include <stdint.h>

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
#define immB()                                                                                                    \
    do                                                                                                            \
    {                                                                                                             \
        *imm = SEXT(BITS(i, 31, 31), 1) << 12 | BITS(i, 7, 7) << 11 | BITS(i, 30, 25) << 5 | BITS(i, 11, 8) << 1; \
    } while (0)
#define immU()                                  \
    do                                          \
    {                                           \
        *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
    } while (0)
#define immJ()                                                                                                        \
    do                                                                                                                \
    {                                                                                                                 \
        *imm = SEXT(BITS(i, 31, 31), 1) << 20 | BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i, 30, 21) << 1; \
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
        decode_operand(s, &dest, &src1, &src2, &imm, CONCAT(TYPE_, type)); \
        __VA_ARGS__;                                                       \
    }

#ifdef __PORTABLE__
#define _SIGN(_src, _len) SEXT(BITS(_src, _len - 1, _len - 1), 1)
#define _BITS(_src, _len) (_len >= 64 ? _src : BITS(_src, _len - 1, 0)) // BITS(x, 63, 0) is UB
#define SRA(_a, _b, _len) (_BITS((_SIGN(_a, _len) ^ _a), _len) >> _b ^ _SIGN(_a, _len))
#define _RS(_x) SRA(_x, 32, 64)
#define u_RS(_x) (_x >> 32)
#define _MULH(_ua, _ub, _a, _b)                                  \
    ({                                                           \
        _ua##int64_t a = _a;                                     \
        _ub##int64_t b = _b;                                     \
        int64_t a_lo = (uint32_t)a;                              \
        int64_t b_lo = (uint32_t)b;                              \
        int64_t a_hi = _ua##_RS(a);                              \
        int64_t b_hi = _ub##_RS(b);                              \
        int64_t mid_1 = a_hi * b_lo + (a_lo * b_lo >> 32);       \
        int64_t mid_2 = a_lo * b_hi + (uint32_t)mid_1;           \
        (uint64_t)(a_hi * b_hi + (mid_1 >> 32) + (mid_2 >> 32)); \
    })
#endif
#ifndef __PORTABLE__
#define SRA(_a, _b, _len) ((int##_len##_t)_a >> _b)
#define _MULH(_ua, _ub, _a, _b) (uint64_t)(((__int128)(_ua##int64_t)_a * (_ub##int64_t)_b) >> 64)
#endif
#define WORD(_x) BITS(_x, 31, 0)
#define JMP() ({ s->dnpc = s->pc + imm; })
#define GES(_a, _b) ((int64_t)_a >= (int64_t)_b)
#define LTS(_a, _b) ((int64_t)_a < (int64_t)_b)
#define MULH(_a, _b) _MULH(, , _a, _b)
#define MULHSU(_a, _b) _MULH(, u, _a, _b)
#define MULHU(_a, _b) _MULH(u, u, _a, _b)
#define DIVOF(_len) (src1 == INT##_len##_MIN && src2 == -1) ? // division overflow
#define DIVBZ (src2 == 0) ?                                   // division by zero

    INSTPAT_START();
    INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(dest) = src1 + src2);
    INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(dest) = src1 + imm);
    INSTPAT("??????? ????? ????? 000 ????? 00110 11", addiw, I, R(dest) = SEXT(WORD(src1 + imm), 32));
    INSTPAT("0000000 ????? ????? 000 ????? 01110 11", addw, R, R(dest) = SEXT(WORD(src1 + src2), 32));
    INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R(dest) = src1 & src2);
    INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(dest) = src1 & imm);
    INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(dest) = s->pc + imm);
    INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, if (src1 == src2) JMP());
    INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if (GES(src1, src2)) JMP());
    INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (src1 >= src2) JMP());
    INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, if (LTS(src1, src2)) JMP());
    INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, if (src1 < src2) JMP());
    INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, if (src1 != src2) JMP());
    INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, R, R(dest) = DIVOF(64)(INT64_MIN)
            : DIVBZ(-1)
            : (int64_t)src1 / (int64_t)src2);
    INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, R, R(dest) = DIVBZ(-1) : src1 / src2);
    INSTPAT("0000001 ????? ????? 101 ????? 01110 11", divuw, R, R(dest) = DIVBZ(-1) : WORD(src1) / WORD(src2));
    INSTPAT("0000001 ????? ????? 100 ????? 01110 11", divw, R, R(dest) = DIVOF(32)(INT32_MIN)
            : DIVBZ(-1)
            : (int64_t)WORD(src1) / (int64_t)WORD(src2));
    INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R(dest) = s->snpc; JMP();
            IFDEF(CONFIG_FTRACE, check_call(s->dnpc)));
    INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, R(dest) = s->snpc; s->dnpc = (src1 + imm) & ~1;
            IFDEF(CONFIG_FTRACE, check_call(s->dnpc); check_ret(INSTPAT_INST(s))));
    INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(dest) = SEXT(Mr(src1 + imm, 1), 8));
    INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(dest) = Mr(src1 + imm, 1));
    INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, R(dest) = Mr(src1 + imm, 8));
    INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(dest) = SEXT(Mr(src1 + imm, 2), 16));
    INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(dest) = Mr(src1 + imm, 2));
    INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(dest) = imm);
    INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(dest) = SEXT(Mr(src1 + imm, 4), 32));
    INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu, I, R(dest) = Mr(src1 + imm, 4));
    INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, R(dest) = src1 * src2);
    INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh, R, R(dest) = MULH(src1, src2));
    INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu, R, R(dest) = MULHSU(src1, src2));
    INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu, R, R(dest) = MULHU(src1, src2));
    INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw, R, R(dest) = SEXT(WORD(src1 * src2), 32));
    INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R(dest) = src1 | src2);
    INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(dest) = src1 | imm);
    INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, R(dest) = DIVOF(64)(0)
            : DIVBZ(src1)
            : (int64_t)src1 % (int64_t)src2);
    INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, R, R(dest) = DIVBZ(src1) : src1 % src2);
    INSTPAT("0000001 ????? ????? 111 ????? 01110 11", remuw, R, R(dest) = DIVBZ(src1) : WORD(src1) % WORD(src2));
    INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw, R, R(dest) = DIVOF(32)(0)
            : DIVBZ(src1)
            : (int64_t)WORD(src1) % (int64_t)WORD(src2));
    INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, BITS(src2, 7, 0)));
    INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd, S, Mw(src1 + imm, 8, src2));
    INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, BITS(src2, 15, 0)));
    INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, R(dest) = src1 << BITS(src2, 5, 0));
    INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli, I, R(dest) = src1 << imm);
    INSTPAT("0000000 ????? ????? 001 ????? 00110 11", slliw, I, R(dest) = SEXT(WORD(src1 << imm), 32));
    INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw, R, R(dest) = SEXT(WORD(src1 << BITS(src2, 4, 0)), 32));
    INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R(dest) = LTS(src1, src2));
    INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R(dest) = LTS(src1, imm));
    INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(dest) = src1 < imm);
    INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R(dest) = src1 < src2);
    INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra, R, R(dest) = SRA(src1, BITS(src2, 5, 0), 64));
    INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai, I, R(dest) = SRA(src1, imm, 64));
    INSTPAT("0100000 ????? ????? 101 ????? 00110 11", sraiw, I, R(dest) = SRA(src1, imm, 32));
    INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw, R, R(dest) = SRA(src1, BITS(src2, 4, 0), 32));
    INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, R(dest) = src1 >> BITS(src2, 5, 0));
    INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli, I, R(dest) = src1 >> imm);
    INSTPAT("0000000 ????? ????? 101 ????? 00110 11", srliw, I, R(dest) = SEXT(WORD(src1) >> imm, 32));
    INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw, R, R(dest) = SEXT(WORD(src1) >> BITS(src2, 4, 0), 32));
    INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(dest) = src1 - src2);
    INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw, R, R(dest) = SEXT(WORD(src1 - src2), 32));
    INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, WORD(src2)));
    INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R(dest) = src1 ^ src2);
    INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(dest) = src1 ^ imm);

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

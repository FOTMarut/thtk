/*
 * Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain this list
 *    of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this
 *    list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#include <config.h>
#include <stddef.h>
#include "thecl.h"
#include "ecsparse.h"
#include "expr.h"

typedef struct {
    int symbol;
    int symbols[2];
} alternative_t;

static const alternative_t
th10_alternatives[] = {
    { ADD,      { ADDI, ADDF } },
    { SUBTRACT, { SUBTRACTI, SUBTRACTF } },
    { MULTIPLY, { MULTIPLYI, MULTIPLYF } },
    { DIVIDE,   { DIVIDEI, DIVIDEF } },
    { EQUAL,    { EQUALI, EQUALF } },
    { INEQUAL,  { INEQUALI, INEQUALF } },
    { LT,       { LTI, LTF } },
    { LTEQ,     { LTEQI, LTEQF } },
    { GT,       { GTI, GTF } },
    { GTEQ,     { GTEQI, GTEQF } },
    { NEG,      { NEGI, NEGF } },
    { 0,        { 0, 0 } }
};

static const expr_t
th10_expressions[] = {
    /* The program checks against the number of params, as well as the
     * requested stack depth, and does the replacements. */
    /* p0 is the first param, p1 the second ... */
    /* s0 is the previous instruction, s1 the one previous to s0 ... */

    /*SYM         ID  RET     P  A    S   DISP                      NB */
    { GOTO,       12,   0, "ot", 0,  "S",           "goto p0 @ p1", 0 },
    { UNLESS,     13,   0, "ot", 1,  "S", "unless s0 goto p0 @ p1", 0 },
    { IF,         14,   0, "ot", 1,  "S",     "if s0 goto p0 @ p1", 0 },

    { LOADI,      42, 'S',  "S", 0, NULL, "p0", 0 },
    { ASSIGNI,    43,   0,  "S", 1,  "S", "p0 = s0", 1 },
    { LOADF,      44, 'f',  "f", 0, NULL, "p0", 0 },
    { ASSIGNF,    45,   0,  "f", 1,  "f", "p0 = s0", 1 },

    { ADDI,       50, 'S', NULL, 2, "SS", "s1 + s0", 0 },
    { ADDF,       51, 'f', NULL, 2, "ff", "s1 + s0", 0 },
    { SUBTRACTI,  52, 'S', NULL, 2, "SS", "s1 - s0", 0 },
    { SUBTRACTF,  53, 'f', NULL, 2, "ff", "s1 - s0", 0 },
    { MULTIPLYI,  54, 'S', NULL, 2, "SS", "s1 * s0", 0 },
    { MULTIPLYF,  55, 'f', NULL, 2, "ff", "s1 * s0", 0 },
    { DIVIDEI,    56, 'S', NULL, 2, "SS", "s1 / s0", 0 },
    { DIVIDEF,    57, 'f', NULL, 2, "ff", "s1 / s0", 0 },
    { MODULO,     58, 'S', NULL, 2, "SS", "s1 % s0", 0 },
    { EQUALI,     59, 'S', NULL, 2, "SS", "s1 == s0", 0 },
    { EQUALF,     60, 'S', NULL, 2, "ff", "s1 == s0", 0 },
    { INEQUALI,   61, 'S', NULL, 2, "SS", "s1 != s0", 0 },
    { INEQUALF,   62, 'S', NULL, 2, "ff", "s1 != s0", 0 },
    { LTI,        63, 'S', NULL, 2, "SS", "s1 < s0", 0 },
    { LTF,        64, 'S', NULL, 2, "ff", "s1 < s0", 0 },
    { LTEQI,      65, 'S', NULL, 2, "SS", "s1 <= s0", 0 },
    { LTEQF,      66, 'S', NULL, 2, "ff", "s1 <= s0", 0 },
    { GTI,        67, 'S', NULL, 2, "SS", "s1 > s0", 0 },
    { GTF,        68, 'S', NULL, 2, "ff", "s1 > s0", 0 },
    { GTEQI,      69, 'S', NULL, 2, "SS", "s1 >= s0", 0 },
    { GTEQF,      70, 'S', NULL, 2, "ff", "s1 >= s0", 0 },
    { NOT,        71, 'S', NULL, 1,  "S", "!s0", 0 },
/*  { XXX,        72,   0, NULL, 0, NULL, NULL },*/
    { OR,         73, 'S', NULL, 2, "SS", "s1 || s0", 0 },
    { AND,        74, 'S', NULL, 2, "SS", "s1 && s0", 0 },
    { XOR,        75, 'S', NULL, 2, "SS", "s1 ^ s0", 0 },
    { B_OR,       76, 'S', NULL, 2, "SS", "s1 | s0", 0 },
    { B_AND,      77, 'S', NULL, 2, "SS", "s1 & s0", 0 },
    { DEC,        78, 'S',  "S", 0, NULL, "p0--", 0 },
    { SIN,        79, 'f', NULL, 1, "f", "sin(s0)", 1 },
    { COS,        80, 'f', NULL, 1, "f", "cos(s0)", 1 },
    { NEGI,       84, 'S', NULL, 1, "S", "-s0", 0 },
    { NEGF,       85, 'f', NULL, 1, "f", "-s0", 0 },
    { SQRT,       88, 'f', NULL, 1, "f", "sqrt(s0)", 1 },
    { 0,           0,   0, NULL, 0, NULL, NULL, 0 }
};

static const expr_t
th13_expressions[] = {
    /*SYM         ID  RET     P  A    S   DISP */
    { NEGI,       83, 'S', NULL, 1, "S", "-s0", 0 },
    { NEGF,       84, 'f', NULL, 1, "f", "-s0", 0 },
    { 0,           0,   0, NULL, 0, NULL, NULL, 0 }
};

static const expr_t*
expr_get_by_symbol_from_table(
    expr_t* table,
    int symbol)
{
    while (table->symbol) {
        if (table->symbol == symbol)
            return table;
        ++table;
    }

    return NULL;
}

const expr_t*
expr_get_by_symbol(
    unsigned int version,
    int symbol)
{
    expr_t* ret = NULL;

    if (!ret && is_post_th13(version)) ret = expr_get_by_symbol_from_table(th13_expressions, symbol);
    if (!ret && is_post_th10(version)) ret = expr_get_by_symbol_from_table(th10_expressions, symbol);

    return ret;
}

static const expr_t*
expr_get_by_id_from_table(
    expr_t* table,
    int id)
{
    while (table->symbol) {
        if (table->id == id)
            return table;
        ++table;
    }

    return NULL;
}

const expr_t*
expr_get_by_id(
    unsigned int version,
    int id)
{
    expr_t* ret = NULL;

    if (!ret && is_post_th13(version)) ret = expr_get_by_id_from_table(th13_expressions, id);
    if (!ret && is_post_th10(version)) ret = expr_get_by_id_from_table(th10_expressions, id);

    return ret;
}

int
expr_is_leaf(
    unsigned int version,
    int id)
{
    const expr_t* expr = expr_get_by_id(version, id);

    if (!expr)
        return 0;

    return expr->stack_arity == 0 &&
           expr->return_type != 0;
}

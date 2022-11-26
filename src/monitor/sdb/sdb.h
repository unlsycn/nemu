#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

void sdb_error(char *loc, char *format, ...);

// expr   -> cmd args
// args   -> cmd args
//         | m_expr
//         | reg
//         | ε
// m_expr -> m_expr op m_expr
//         | (m_expr)
//         | number
// op     -> +
//         | -
//         | *
//         | /
// reg    -> $reg_name

typedef enum
{
    TK_CMD,
    TK_NUM,   // DEC | HEX
    TK_PUNCT, // + | - | * | / | ( | )
    TK_REG,   // &reg
    TK_EOL
} TokenKind;

typedef struct Token Token;
struct Token
{
    TokenKind kind;
    Token *next;
    word_t value;
    char *loc;
    int length;
};

Token *tokenize(char *str);

bool token_equal(Token *token, char *str);

#endif

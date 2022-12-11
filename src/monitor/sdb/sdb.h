#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>
#include <setjmp.h>

// stmt -> cmd args
// args -> subcmd
//       | expr
//       | ε
// expr -> expr bop expr
//       | op expr
//       | (expr)
//       | number
//       | reg
// bop  -> +
//       | -
//       | *
//       | /
//       | ==
//       | !=
//       | >
//       | >=
//       | <
//       | <=
//       | &&
// uop   | +
//       | -
//       | *
// reg  -> $reg_name

/* sdb */

extern void *sdb_calloc(size_t __nmemb, size_t __size);

void sdb_error(char *loc, char *format, ...);

/* interrupt current statement if assert fails */
#define sdb_assert(cond, loc, format, ...)     \
    if (!(cond))                               \
    {                                          \
        sdb_error(loc, format, ##__VA_ARGS__); \
    }

typedef enum
{
    SDB_RUN,
    SDB_QUIT,
    SDB_WP,
} SdbRet;

/* tokenizer */

typedef enum
{
    TK_CMD,
    TK_NUM, // DEC | HEX
    TK_PUNCT,
    TK_REG, // &reg
    TK_EOL
} TokenType;

typedef struct Token Token;
struct Token
{
    TokenType type;
    Token *next;
    word_t value;
    char *loc;
    int length;
};

extern void free_tokens(Token *tokens);

extern bool token_equal(Token *token, const char *str);

Token *tokenize(char *str);

/* parser */

typedef enum
{
    NON = -1,
    // cmd
    AST_CMD_C,
    AST_CMD_Q,
    // cmd subcmd
    AST_CMD_HELP,
    AST_CMD_INFO,
    // cmd expr
    AST_CMD_SI,
    AST_CMD_P,
    AST_CMD_W,
    AST_CMD_D,
    // cmd n expr
    AST_CMD_X,
    // subcmd
    AST_SUBCMD,
    // ||
    AST_LOGI_OR,
    // &&
    AST_LOGI_AND,
    // |
    AST_BIT_OR,
    // ^
    AST_BIT_XOR,
    // &
    AST_BIT_AND,
    // == !=
    AST_EQ,
    AST_NEQ,
    // <= < >= >
    AST_LE,
    AST_LT,
    AST_GE,
    AST_GT,
    // << >>
    AST_LS,
    AST_RS,
    // binary + =
    AST_ADD,
    AST_SUB,
    // * / %
    AST_MUL,
    AST_DIV,
    AST_MOD,
    // ! ~
    AST_LOGI_NOT,
    AST_BIT_NOT,
    // unary + -
    AST_POS,
    AST_NEG,
    // deref *
    AST_DEREF,
    // number reg
    AST_NUMBER,
    AST_REG
} ASTNodeType;

typedef union ASTValue ASTValue;
union ASTValue {
    word_t i;
    char *str;
};

typedef struct ASTNode ASTNode;

typedef ASTValue (*Handler)(ASTNode *this);

struct ASTNode
{
    ASTNodeType type;
    ASTNode *left_child;
    ASTNode *right_child;
    ASTValue value;
    Handler handler;
};

typedef struct Operator Operator;
struct Operator
{
    const char *str;
    const ASTNodeType type;
    Handler handler;
};

typedef struct OperatorPrec OperatorPrec;
struct OperatorPrec
{
    const int n;
    OperatorPrec *next;
    const Operator list[];
};

bool is_double_punct(char *str);

#define ret_val return this->value;

void delete_AST_node(ASTNode *node);

extern void free_AST(ASTNode *node);

ASTNode *parse(Token *tokens);

/* command */

typedef struct Cmd Cmd;
struct Cmd
{
    const char *name;
    const char *description;
    const ASTNodeType type;
    Handler handler;
};

ASTNodeType type_cmd(Token *token);

Handler handler_cmd(Token *token);

/* watchpoint */

extern int new_wp(ASTNode *node);
void remedy_expr(Token *expr);

extern void free_wp(int no);

extern void print_wp();

#endif

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

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

void sdb_error(char *loc, char *format, ...);

typedef enum
{
    TK_CMD,
    TK_NUM, // DEC | HEX
    TK_PUNCT,
    TK_REG, // &reg
    TK_EOL
} TokenType;

typedef struct Token Token;

Token *tokenize(char *str);

bool token_equal(Token *token, const char *str);

struct Token
{
    TokenType type;
    Token *next;
    word_t value;
    char *loc;
    int length;
};

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
    //&&
    AST_AND,
    // == !=
    AST_EQ,
    AST_NEQ,
    // >= > <= <
    AST_GE,
    AST_GT,
    AST_LE,
    AST_LT,
    // binary + =
    AST_ADD,
    AST_SUB,
    // * /
    AST_MUL,
    AST_DIV,
    // !
    AST_NOT,
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

typedef struct ASTNode ASTNode;

typedef struct Cmd Cmd;

typedef ASTValue (*Handler)(ASTNode *this);

ASTNodeType type_cmd(Token *token);

Handler handler_cmd(Token *token);

ASTValue cmd_c(ASTNode *this);
ASTValue cmd_q(ASTNode *this);
ASTValue cmd_help(ASTNode *this);
ASTValue cmd_info(ASTNode *this);
ASTValue cmd_si(ASTNode *this);
ASTValue cmd_p(ASTNode *this);
ASTValue cmd_w(ASTNode *this);
ASTValue cmd_d(ASTNode *this);
ASTValue cmd_x(ASTNode *this);

ASTValue subcmd_handler(ASTNode *this);
ASTValue and_handler(ASTNode *this);
ASTValue eq_handler(ASTNode *this);
ASTValue neq_handler(ASTNode *this);
ASTValue ge_handler(ASTNode *this);
ASTValue gt_handler(ASTNode *this);
ASTValue le_handler(ASTNode *this);
ASTValue lt_handler(ASTNode *this);
ASTValue add_handler(ASTNode *this);
ASTValue sub_handler(ASTNode *this);
ASTValue mul_handler(ASTNode *this);
ASTValue div_handler(ASTNode *this);
ASTValue not_handler(ASTNode *this);
ASTValue pos_handler(ASTNode *this);
ASTValue neg_handler(ASTNode *this);
ASTValue deref_handler(ASTNode *this);
ASTValue reg_handler(ASTNode *this);
ASTValue number_handler(ASTNode *this);

#define ret_val return this->value;

union ASTValue {
    word_t i;
    char *ch;
};
struct Cmd
{
    const char *name;
    const char *description;
    const ASTNodeType type;
    Handler handler;
};

struct ASTNode
{
    ASTNodeType type;
    ASTNode *left_child;
    ASTNode *right_child;
    ASTValue value;
    Handler handler;
};

ASTNode *parse(Token *tokens);

void traverse_AST(ASTNode *ast);

#endif

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

void sdb_error(char *loc, char *format, ...);

extern void *sdb_calloc(size_t __nmemb, size_t __size);

typedef enum
{
    SDB_RUN,
    SDB_QUIT,
    SDB_WP,
} SdbRet;

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

extern void free_tokens(Token *tokens);

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

typedef struct ASTNode ASTNode;

typedef struct Cmd Cmd;

typedef struct Operator Operator;

typedef struct OperatorPrec OperatorPrec;

typedef ASTValue (*Handler)(ASTNode *this);

void delete_AST_node(ASTNode *node);

extern void free_AST(ASTNode *node);

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
ASTValue logi_or_handler(ASTNode *this);
ASTValue logi_and_handler(ASTNode *this);
ASTValue bit_or_handler(ASTNode *this);
ASTValue bit_xor_handler(ASTNode *this);
ASTValue bit_and_handler(ASTNode *this);
ASTValue eq_handler(ASTNode *this);
ASTValue neq_handler(ASTNode *this);
ASTValue le_handler(ASTNode *this);
ASTValue lt_handler(ASTNode *this);
ASTValue ge_handler(ASTNode *this);
ASTValue gt_handler(ASTNode *this);
ASTValue ls_handler(ASTNode *this);
ASTValue rs_handler(ASTNode *this);
ASTValue add_handler(ASTNode *this);
ASTValue sub_handler(ASTNode *this);
ASTValue mul_handler(ASTNode *this);
ASTValue div_handler(ASTNode *this);
ASTValue mod_handler(ASTNode *this);
ASTValue logi_not_handler(ASTNode *this);
ASTValue bit_not_handler(ASTNode *this);
ASTValue pos_handler(ASTNode *this);
ASTValue neg_handler(ASTNode *this);
ASTValue deref_handler(ASTNode *this);
ASTValue reg_handler(ASTNode *this);
ASTValue number_handler(ASTNode *this);

#define ret_val return this->value;

union ASTValue {
    word_t i;
    char *str;
};

struct Cmd
{
    const char *name;
    const char *description;
    const ASTNodeType type;
    Handler handler;
};

struct Operator
{
    const char *str;
    const ASTNodeType type;
    Handler handler;
};

struct OperatorPrec
{
    const int n;
    OperatorPrec *next;
    const Operator list[];
};

bool is_double_punct(char *str);

struct ASTNode
{
    ASTNodeType type;
    ASTNode *left_child;
    ASTNode *right_child;
    ASTValue value;
    Handler handler;
};

ASTNode *parse(Token *tokens);

extern int new_wp(ASTNode *node);
void remedy_expr(Token *expr);

extern void free_wp(int no);

extern void print_wp();

#endif

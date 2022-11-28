#include "sdb.h"
#include <isa.h>
#include <memory/vaddr.h>

#define LHS this->left_child->handler(this->left_child).i
#define RHS this->right_child->handler(this->right_child).i

#pragma region handlers

ASTValue subcmd_handler(ASTNode *this)
{
    ret_val;
}
ASTValue and_handler(ASTNode *this)
{
    this->value.i = LHS && RHS;
    ret_val;
}
ASTValue eq_handler(ASTNode *this)
{
    this->value.i = LHS == RHS;
    ret_val;
}
ASTValue neq_handler(ASTNode *this)
{
    this->value.i = LHS != RHS;
    ret_val;
}
ASTValue ge_handler(ASTNode *this)
{
    this->value.i = LHS >= RHS;
    ret_val;
}
ASTValue gt_handler(ASTNode *this)
{
    this->value.i = LHS > RHS;
    ret_val;
}
ASTValue le_handler(ASTNode *this)
{
    this->value.i = LHS <= RHS;
    ret_val;
}
ASTValue lt_handler(ASTNode *this)
{
    this->value.i = LHS < RHS;
    ret_val;
}
ASTValue add_handler(ASTNode *this)
{
    this->value.i = LHS + RHS;
    ret_val;
}
ASTValue sub_handler(ASTNode *this)
{
    this->value.i = LHS - RHS;
    ret_val;
}
ASTValue mul_handler(ASTNode *this)
{
    this->value.i = LHS * RHS;
    ret_val;
}
ASTValue div_handler(ASTNode *this)
{
    word_t rhs = RHS;
    this->value.i = rhs == 0 ? 0 : LHS / rhs;
    ret_val;
}
ASTValue not_handler(ASTNode *this)
{
    this->value.i = !LHS;
    ret_val;
}
ASTValue pos_handler(ASTNode *this)
{
    this->value.i = LHS;
    ret_val;
}
ASTValue neg_handler(ASTNode *this)
{
    this->value.i = -LHS;
    ret_val;
}
ASTValue deref_handler(ASTNode *this)
{
    this->value.i = vaddr_read(LHS, 1);
    ret_val;
}
ASTValue reg_handler(ASTNode *this)
{
    ASTValue val;
    bool *success = NULL;
    val.i = isa_reg_str2val(this->value.ch, success);
    Assert(success == 0, "Failed to get the value of reg %s", this->value.ch);
    ret_val;
}
ASTValue number_handler(ASTNode *this)
{
    ret_val;
}

#pragma endregion

static TokenDef op_table[] = {{"&&", "logical and", AST_AND, and_handler},
                              {"==", "equal", AST_EQ, eq_handler},
                              {"!=", "not equal", AST_NEQ, neq_handler},
                              {">=", "great than or equal", AST_GE, ge_handler},
                              {">", "great than", AST_GT, gt_handler},
                              {"<=", "less than or equal", AST_LE, le_handler},
                              {"<", "less than", AST_LT, lt_handler},
                              {"+", "add", AST_ADD, add_handler},
                              {"-", "subtrate", AST_SUB, sub_handler},
                              {"*", "multiply", AST_MUL, mul_handler},
                              {"/", "is divided by", AST_DIV, div_handler},
                              {"!", "logical not", AST_NOT, not_handler},
                              {"+", "positive sign", AST_POS, pos_handler},
                              {"-", "negative sign", AST_NEG, neg_handler},
                              {"*", "dereference sign", AST_DEREF, deref_handler}};

#define NR_OP ARRLEN(op_table)

#define is_unary(op) (op->type == AST_NOT || op->type == AST_POS || op->type == AST_NEG || op->type == AST_DEREF)

static int index_op(TokenDef *op)
{
    for (int i = 0; i < NR_OP; i++)
        if (op_table[i].type == op->type)
            return i;
    return -1;
}

bool is_double_punct(char *str)
{
    for (int i = 0; i < NR_OP; i++)
    {
        const char *op = op_table[i].str;
        if (op[1] != '\0' && strncmp(str, op, 2) == 0)
            return true;
    }
    return false;
}

#pragma region AST_node

static ASTNode *new_AST_node(ASTNodeType type)
{
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->left_child = NULL;
    node->right_child = NULL;
    node->handler = NULL;
    return node;
}

static ASTNode *new_AST_binary(ASTNodeType type, ASTNode *left_child, ASTNode *right_child, Handler handler)
{
    ASTNode *node = new_AST_node(type);
    node->left_child = left_child;
    node->right_child = right_child;
    node->handler = handler;
    return node;
}

static ASTNode *new_AST_unary(ASTNodeType type, ASTNode *child, Handler handler)
{
    ASTNode *node = new_AST_node(type);
    node->left_child = child;
    node->handler = handler;
    return node;
}

static ASTNode *new_AST_cmd(Token *cmd)
{
    ASTNode *node = new_AST_node(type_cmd(cmd));
    node->handler = handler_cmd(cmd);
    node->value.i = 0;
    return node;
}

static ASTNode *new_AST_subcmd(Token *subcmd)
{
    ASTNode *node = new_AST_node(AST_SUBCMD);
    node->value.ch = calloc(1, sizeof(subcmd->length));
    memcpy(node->value.ch, subcmd->loc, subcmd->length);
    node->handler = subcmd_handler;
    return node;
}

static ASTNode *new_AST_reg(Token *reg)
{
    ASTNode *node = new_AST_node(AST_REG);
    node->value.ch = calloc(1, sizeof(reg->length - 1));
    memcpy(node->value.ch, reg->loc + 1, reg->length - 1);
    node->handler = reg_handler;
    return node;
}

static ASTNode *new_AST_number(Token *number)
{
    ASTNode *node = new_AST_node(AST_NUMBER);
    node->value.i = number->value;
    node->handler = number_handler;
    return node;
}

#pragma endregion

/* interrupt current statement if assert fails */
#define sdb_assert(token, offset, cond, format, ...)           \
    if (!(cond))                                               \
    {                                                          \
        sdb_error(token->loc + offset, format, ##__VA_ARGS__); \
        return NULL;                                           \
    }

#define assert_type(token, tok_type, format, ...) sdb_assert(token, 1, token->type == tok_type, format, ##__VA_ARGS__)

#define assert_str(token, str, format, ...) sdb_assert(token, 1, token_equal(token, str), format, ##__VA_ARGS__)

/* check if currect token is the last one */
#define last_args(token) \
    token = token->next; \
    assert_type(token, TK_EOL, "Unexpected argument.")

/* tokens_ptr returns the position of the pointer in the linked list to the upper level */
static ASTNode *parse_expr(Token *tokens, Token **tokens_ptr);

static ASTNode *parse_number_reg_bracket(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node;
    if (tokens->type == TK_NUM)
    {
        node = new_AST_number(tokens);
    }
    else if (tokens->type == TK_REG)
    {
        node = new_AST_reg(tokens);
    }
    else if (token_equal(tokens, "("))
    {
        node = parse_expr(tokens->next, &tokens);
        assert_str(tokens, ")", "Expected a \")\".");
    }
    else
    {
        node = NULL;
        sdb_error(tokens->loc + 1, "Unexpected token.");
    }
    *tokens_ptr = tokens->next;
    return node;
}

static ASTNode *parse_operator(Token *tokens, Token **tokens_ptr, TokenDef *op)
{
    ASTNode *node;
    TokenDef *next_op = &op_table[index_op(op) + 1];
    if (is_unary(op)) // unary operators
    {
        if (token_equal(tokens, op->str))
        {
            // unary must locate behind the expression, so we parse the operator before entering the next level
            node = new_AST_unary(op->type, parse_operator(tokens->next, &tokens, op), op->handler);
            *tokens_ptr = tokens;
            return node;
        }
        if (index_op(op) == NR_OP - 1) // the last level
        {
            node = parse_number_reg_bracket(tokens, &tokens);
        }
        else
        {
            node = parse_operator(tokens, &tokens, next_op);
        }
    }
    else // binary operators
    {
        node = parse_operator(tokens, &tokens,
                              next_op);      // parse until there is nothing of higher priority in the LHS;
        while (token_equal(tokens, op->str)) // why use while? consider "(2 + 5) && 6 && 1"
        {
            node = new_AST_binary(op->type, node, parse_operator(tokens->next, &tokens, next_op), op->handler);
        }
    }
    *tokens_ptr = tokens;
    return node;
}

/* In-Order traversal */
static ASTNode *parse_expr(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_operator(tokens, &tokens, &op_table[0]);
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_cmd(Token *tokens, Token **tokens_ptr)
{
    assert_type(tokens, TK_CMD, "Expected a command.");
    ASTNode *node;
    ASTNodeType type = type_cmd(tokens);
    sdb_assert(tokens, 0, type != NON, "Command not found.");
    if (type >= AST_CMD_C && type <= AST_CMD_Q) // cmd
    {
        node = new_AST_cmd(tokens);
        last_args(tokens);
    }
    else if (type >= AST_CMD_HELP && type <= AST_CMD_INFO) // cmd subcmd
    {
        node = new_AST_cmd(tokens);
        tokens = tokens->next;
        assert_type(tokens, TK_CMD, "Expected a subcommand.");
        node->left_child = new_AST_subcmd(tokens);
        last_args(tokens);
    }
    else if (type >= AST_CMD_SI && type <= AST_CMD_D) // cmd expr
    {
        node = new_AST_cmd(tokens);
        tokens = tokens->next;
        if (tokens->type == TK_EOL) // assign default value
        {
            if (type == AST_CMD_SI)
            {
                node->left_child = new_AST_node(AST_NUMBER);
                node->left_child->value.i = 1;
            }
            sdb_error(tokens->loc + 1, "Expected an argument.");
        }
        else
        {
            node->left_child = parse_expr(tokens, &tokens);
        }
    }
    else if (type == AST_CMD_X) // cmd N expr
    {
        node = new_AST_cmd(tokens);
        tokens = tokens->next;
        assert_type(tokens, TK_NUM, "Expected a number.");
        node->left_child = new_AST_number(tokens);
        node->right_child = parse_expr(tokens->next, &tokens);
    }
    else
    {
        panic("Unexpected error: illegal type of the command token \"%s\".", tokens->loc);
    }
    *tokens_ptr = tokens;
    return node;
}

ASTNode *parse(Token *tokens)
{
    ASTNode *node = parse_cmd(tokens, &tokens);
    assert_type(tokens, TK_EOL, "Junk after end of expression.");
    return node;
}
#include "sdb.h"
#include <isa.h>
#include <memory/vaddr.h>

#define LHS this->left_child->handler(this->left_child).i
#define RHS this->right_child->handler(this->right_child).i

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
    this->value.i = LHS / RHS;
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
    this->value.i = vaddr_read(LHS, 1); // NOT TEST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
/* functions are listed in order of priority */

static ASTNode *parse_number_reg_bracket(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node;
    if (tokens->type == TK_NUM)
    {
        node = new_AST_number(tokens);
    }
    if (tokens->type == TK_REG)
    {
        node = new_AST_reg(tokens);
    }
    if (token_equal(tokens, "("))
    {
        node = parse_expr(tokens->next, &tokens);
        assert_str(tokens, ")", "Expected a \")\".");
    }
    *tokens_ptr = tokens->next;
    return node;
}

/* unary +, - and * must locate behind the number, so we parse the operator before entering the next level */
static ASTNode *parse_deref(Token *tokens, Token **tokens_ptr)
{
    if (token_equal(tokens, "*"))
    {
        return new_AST_unary(AST_DEREF, parse_deref(tokens->next, &tokens), deref_handler);
    }
    ASTNode *node = parse_number_reg_bracket(tokens, &tokens);
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_pos_neg(Token *tokens, Token **tokens_ptr)
{
    if (token_equal(tokens, "+"))
    {
        return new_AST_unary(AST_POS, parse_pos_neg(tokens->next, &tokens), pos_handler);
    }
    if (token_equal(tokens, "-"))
    {
        return new_AST_unary(AST_NEG, parse_pos_neg(tokens->next, &tokens), neg_handler);
    }
    ASTNode *node = parse_deref(tokens, &tokens);
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_not(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_pos_neg(tokens, &tokens);
    while (token_equal(tokens, "!"))
    {
        node = new_AST_unary(AST_NOT, parse_pos_neg(tokens->next, &tokens), not_handler);
    }
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_mul_div(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_not(tokens, &tokens);
    while (tokens->type == TK_PUNCT)
    {
        if (token_equal(tokens, "*"))
        {
            node = new_AST_binary(AST_MUL, node, parse_not(tokens->next, &tokens), mul_handler);
            continue;
        }
        if (token_equal(tokens, "/"))
        {
            node = new_AST_binary(AST_DIV, node, parse_not(tokens->next, &tokens), div_handler);
            continue;
        }
        break;
    }
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_add_sub(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_mul_div(tokens, &tokens);
    while (tokens->type == TK_PUNCT)
    {
        if (token_equal(tokens, "+"))
        {
            node = new_AST_binary(AST_ADD, node, parse_mul_div(tokens->next, &tokens), add_handler);
            continue;
        }
        if (token_equal(tokens, "-"))
        {
            node = new_AST_binary(AST_SUB, node, parse_mul_div(tokens->next, &tokens), sub_handler);
            continue;
        }
        break;
    }
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_ge_gt_le_lt(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_add_sub(tokens, &tokens);
    while (tokens->type == TK_PUNCT)
    {
        if (token_equal(tokens, ">="))
        {
            node = new_AST_binary(AST_GE, node, parse_add_sub(tokens->next, &tokens), ge_handler);
            continue;
        }
        if (token_equal(tokens, ">"))
        {
            node = new_AST_binary(AST_GT, node, parse_add_sub(tokens->next, &tokens), gt_handler);
            continue;
        }
        if (token_equal(tokens, "<="))
        {
            node = new_AST_binary(AST_LE, node, parse_add_sub(tokens->next, &tokens), le_handler);
            continue;
        }
        if (token_equal(tokens, "<"))
        {
            node = new_AST_binary(AST_LT, node, parse_add_sub(tokens->next, &tokens), lt_handler);
            continue;
        }
        break;
    }
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_eq_neq(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_ge_gt_le_lt(tokens, &tokens);
    while (tokens->type == TK_PUNCT)
    {
        if (token_equal(tokens, "=="))
        {
            node = new_AST_binary(AST_EQ, node, parse_ge_gt_le_lt(tokens->next, &tokens), eq_handler);
            continue;
        }
        if (token_equal(tokens, "!="))
        {
            node = new_AST_binary(AST_NEQ, node, parse_ge_gt_le_lt(tokens->next, &tokens), neq_handler);
            continue;
        }
        break;
    }
    *tokens_ptr = tokens;
    return node;
}

static ASTNode *parse_and(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_eq_neq(tokens, &tokens); // parse until there is nothing of higher priority in the LHS;
    while (token_equal(tokens, "&&"))              // why use while? consider "(2 + 5) && 6 && 1"
    {
        node = new_AST_binary(AST_AND, node, parse_eq_neq(tokens->next, &tokens), and_handler);
    }
    *tokens_ptr = tokens;
    return node;
}

/* In-Order traversal */
static ASTNode *parse_expr(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_and(tokens, &tokens);
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
    if (type >= AST_CMD_HELP && type <= AST_CMD_INFO) // cmd subcmd
    {
        node = new_AST_cmd(tokens);
        tokens = tokens->next;
        assert_type(tokens, TK_CMD, "Expected a subcommand.");
        node->left_child = new_AST_subcmd(tokens);
        last_args(tokens);
    }
    if (type >= AST_CMD_SI && type <= AST_CMD_D) // cmd expr
    {
        node = new_AST_cmd(tokens);
        node->left_child = parse_expr(tokens->next, &tokens);
    }
    if (type == AST_CMD_X) // cmd N expr
    {
        node = new_AST_cmd(tokens);
        tokens = tokens->next;
        assert_type(tokens, TK_NUM, "Expected a number.");
        node->left_child = new_AST_number(tokens);
        node->right_child = parse_expr(tokens->next, &tokens);
    }
    // panic("Unexpected error: illegal type of the command token \"%s\".", tokens->loc);
    *tokens_ptr = tokens;
    return node;
}

ASTNode *parse(Token *tokens)
{
    ASTNode *node = parse_cmd(tokens, &tokens);
    assert_type(tokens, TK_EOL, "Junk after end of expression.");
    return node;
}
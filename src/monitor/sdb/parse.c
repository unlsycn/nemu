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
ASTValue logi_or_handler(ASTNode *this)
{
    this->value.i = LHS || RHS;
    ret_val;
}
ASTValue logi_and_handler(ASTNode *this)
{
    this->value.i = LHS && RHS;
    ret_val;
}
ASTValue bit_or_handler(ASTNode *this)
{
    this->value.i = LHS | RHS;
    ret_val;
}
ASTValue bit_xor_handler(ASTNode *this)
{
    this->value.i = LHS ^ RHS;
    ret_val;
}
ASTValue bit_and_handler(ASTNode *this)
{
    this->value.i = LHS & RHS;
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
ASTValue ls_handler(ASTNode *this)
{
    this->value.i = LHS << RHS;
    ret_val;
}
ASTValue rs_handler(ASTNode *this)
{
    this->value.i = LHS >> RHS;
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
    Assert(rhs != 0, "Division by zero is illegal.");
    this->value.i = LHS / rhs;
    ret_val;
}
ASTValue mod_handler(ASTNode *this)
{
    word_t rhs = RHS;
    Assert(rhs != 0, "Remainder by zero is illegal.");
    this->value.i = LHS % rhs;
    ret_val;
}
ASTValue logi_not_handler(ASTNode *this)
{
    this->value.i = !LHS;
    ret_val;
}
ASTValue bit_not_handler(ASTNode *this)
{
    this->value.i = ~LHS;
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
    bool success = false;
    val.i = isa_reg_str2val(this->value.str, &success);
    Assert(success, "Failed to get the value of reg %s.", this->value.str);
    return val;
}
ASTValue number_handler(ASTNode *this)
{
    ret_val;
}

#pragma endregion

static OperatorPrec unary = {5,
                             NULL,
                             {{"!", AST_LOGI_NOT, logi_not_handler},
                              {"~", AST_BIT_NOT, bit_not_handler},
                              {"+", AST_POS, pos_handler},
                              {"-", AST_NEG, neg_handler},
                              {"*", AST_DEREF, deref_handler}}};
static OperatorPrec mul_div_mod = {
    3, &unary, {{"*", AST_MUL, mul_handler}, {"/", AST_DIV, div_handler}, {"%", AST_MOD, mod_handler}}};
static OperatorPrec add_sub = {2, &mul_div_mod, {{"+", AST_ADD, add_handler}, {"-", AST_SUB, sub_handler}}};
static OperatorPrec shift = {2, &add_sub, {{"<<", AST_LS, ls_handler}, {">>", AST_RS, rs_handler}}};
static OperatorPrec relation = {
    4,
    &shift,
    {{"<=", AST_LE, le_handler}, {"<", AST_LT, lt_handler}, {">=", AST_GE, ge_handler}, {">", AST_GT, gt_handler}}};
static OperatorPrec eq = {2, &relation, {{"==", AST_EQ, eq_handler}, {"!=", AST_NEQ, neq_handler}}};
static OperatorPrec bit_and = {1, &eq, {{"&", AST_BIT_AND, bit_and_handler}}};
static OperatorPrec bit_xor = {1, &bit_and, {{"^", AST_BIT_XOR, bit_xor_handler}}};
static OperatorPrec bit_or = {1, &bit_xor, {{"|", AST_BIT_OR, bit_or_handler}}};
static OperatorPrec logi_and = {1, &bit_or, {{"&&", AST_LOGI_AND, logi_and_handler}}};
static OperatorPrec logi_or = {1, &logi_and, {{"||", AST_LOGI_OR, logi_or_handler}}};

#define HEAD_OP logi_or

bool is_double_punct(char *str)
{
    OperatorPrec *prec = &HEAD_OP;
    while (prec != NULL)
    {
        for (int i = 0; i < prec->n; i++)
        {
            const char *op = prec->list[i].str;
            if (op[1] != '\0' && strncmp(str, op, 2) == 0)
                return true;
        }
        prec = prec->next;
    }
    return false;
}

#pragma region AST_node

extern void delete_AST_node(ASTNode *node)
{
    if (node->type == AST_SUBCMD || node->type == AST_REG)
        free(node->value.str);
    free(node);
}

extern void free_AST(ASTNode *node)
{
    if (node->left_child)
        free(node->left_child);
    if (node->right_child)
        free(node->right_child);
    delete_AST_node(node);
}

static ASTNode *new_AST_node(ASTNodeType type)
{
    ASTNode *node = sdb_calloc(1, sizeof(ASTNode));
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
    node->value.i = SDB_RUN;
    return node;
}

static ASTNode *proto_AST_subcmd(int len, char *str)
{
    ASTNode *node = new_AST_node(AST_SUBCMD);
    node->value.str =
        sdb_calloc(1, len * sizeof(char)); // copy str to ensure the accessibility of the AST in the wp_pool
    memcpy(node->value.str, str, len);
    node->handler = subcmd_handler;
    return node;
}

static ASTNode *new_AST_subcmd(Token *subcmd)
{
    return proto_AST_subcmd(subcmd->length, subcmd->loc);
}

static ASTNode *new_AST_reg(Token *reg)
{
    ASTNode *node = new_AST_node(AST_REG);
    node->value.str = sdb_calloc(1, (reg->length - 1) * sizeof(char));
    memcpy(node->value.str, reg->loc + 1, reg->length - 1);
    node->handler = reg_handler;
    return node;
}

static ASTNode *proto_AST_number(word_t val)
{
    ASTNode *node = new_AST_node(AST_NUMBER);
    node->value.i = val;
    node->handler = number_handler;
    return node;
}

static ASTNode *new_AST_number(Token *number)
{
    return proto_AST_number(number->value);
}

#pragma endregion

/* interrupt current statement if assert fails */
#define sdb_assert(token, offset, cond, format, ...)           \
    if (!(cond))                                               \
    {                                                          \
        sdb_error(token->loc + offset, format, ##__VA_ARGS__); \
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

static ASTNode *parse_operator(Token *tokens, Token **tokens_ptr, OperatorPrec *prec)
{
    Assert(prec != NULL, "Invalid Operator.");
    ASTNode *node;
    if (prec == &unary) // unary operators
    {
        for (int i = 0; i < prec->n; i++)
        {
            const Operator *op = &prec->list[i];
            if (token_equal(tokens, op->str))
            {
                // unary must locate behind the expression, so we parse the operator before entering the next level
                node = new_AST_unary(op->type, parse_operator(tokens->next, &tokens, prec), op->handler);
                goto ret;
            }
        }
        node = parse_number_reg_bracket(tokens, &tokens);
    }
    else // binary operators
    {
        node = parse_operator(tokens, &tokens,
                              prec->next); // parse until there is nothing of higher priority in the LHS;
    loop:
        for (int i = 0; i < prec->n; i++)
        {
            const Operator *op = &prec->list[i];
            if (token_equal(tokens, op->str))
            {
                node = new_AST_binary(op->type, node, parse_operator(tokens->next, &tokens, prec->next), op->handler);
                goto loop;
            }
        }
        goto ret;
    }
ret:
    *tokens_ptr = tokens;
    return node;
}

/* In-Order traversal */
static ASTNode *parse_expr(Token *tokens, Token **tokens_ptr)
{
    ASTNode *node = parse_operator(tokens, &tokens, &HEAD_OP);
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
        if (tokens->type == TK_EOL)
        {
            if (type == AST_CMD_HELP)
            {
                node->left_child = proto_AST_subcmd(3, "all");
            }
            else
            {
                sdb_error(tokens->loc + 1, "Expected a subcommand.");
            }
        }
        else
        {
            node->left_child = new_AST_subcmd(tokens);
            last_args(tokens);
        }
    }
    else if (type >= AST_CMD_SI && type <= AST_CMD_D) // cmd expr
    {
        node = new_AST_cmd(tokens);
        tokens = tokens->next;
        if (tokens->type == TK_EOL) // assign default value
        {
            if (type == AST_CMD_SI)
            {
                node->left_child = proto_AST_number(1);
            }
            else
            {
                sdb_error(tokens->loc + 1, "Expected an argument.");
            }
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
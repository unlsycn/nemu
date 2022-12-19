#include "common.h"
#include "isa.h"
#include "sdb.h"
#include <cpu/cpu.h>
#include <memory/vaddr.h>

#define ARG_1 this->left_child->handler(this->left_child)
#define ARG_2 this->left_child->handler(this->left_child)

ASTValue cmd_c(ASTNode *this)
{
    cpu_exec(-1);
    ret_val;
}

ASTValue cmd_q(ASTNode *this)
{
    nemu_state.state = NEMU_QUIT;
    this->value.i = SDB_QUIT;
    ret_val;
}

ASTValue cmd_help(ASTNode *this);

ASTValue cmd_info(ASTNode *this)
{
    char *subcmd = ARG_1.str;
    if (strcmp(subcmd, "r") == 0)
    {
        isa_reg_display();
    }
    else if (strcmp(subcmd, "w") == 0)
    {
        print_wp();
    }
    ret_val;
}

ASTValue cmd_si(ASTNode *this)
{
    word_t n = ARG_1.i;
    cpu_exec(n);
    ret_val;
}

ASTValue cmd_p(ASTNode *this)
{
    printf(FMT_WORD_LD "\n", ARG_1.i);
    ret_val;
}

ASTValue cmd_w(ASTNode *this)
{
    printf("Watchpoint created: Watchpoint %d.\n", new_wp(this->left_child));
    this->value.i = SDB_WP;
    ret_val;
}

ASTValue cmd_d(ASTNode *this)
{
    int no = ARG_1.i;
    free_wp(no);
    printf("Watchpoint %d removed.\n", no);
    ret_val;
}

ASTValue cmd_x(ASTNode *this)
{
    int n = ARG_1.i;
    vaddr_t addr = ARG_2.i;
    vaddr_read(addr, n * 4);
    ret_val;
}

static Cmd cmd_table[] = {
    {"help", "Display information about all supported commands.", AST_CMD_HELP, cmd_help},
    {"c", "Continue the execution of the program.", AST_CMD_C, cmd_c},
    {"q", "Exit NEMU.", AST_CMD_Q, cmd_q},
    {"si", "Excute N instructions.", AST_CMD_SI, cmd_si},
    {"info", "Describe the state of your program.", AST_CMD_INFO, cmd_info},
    {"x", "Scan the memory.", AST_CMD_X, cmd_x},
    {"p", "Print the value of the expression.", AST_CMD_P, cmd_p},
    {"w", "Add a watchpoint which pauses the execution when the value of the expression changes.", AST_CMD_W, cmd_w},
    {"d", "Delete specified watchpoint.", AST_CMD_D, cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static Cmd *iter_cmd(Token *token)
{
    for (int i = 0; i < NR_CMD; i++)
        if (token_equal(token, cmd_table[i].name))
            return &cmd_table[i];
    return NULL;
}

ASTNodeType type_cmd(Token *token)
{
    Cmd *cmd = iter_cmd(token);
    return cmd == NULL ? NON : cmd->type;
}

Handler handler_cmd(Token *token)
{
    Cmd *cmd = iter_cmd(token);
    return cmd == NULL ? NULL : cmd->handler;
}

ASTValue cmd_help(ASTNode *this)
{
    char *cmd = ARG_1.str;
    if (strcmp(cmd, "all") == 0)
    {
        for (int i = 0; i < NR_CMD; i++)
        {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else
    {
        for (int i = 0; i < NR_CMD; i++)
        {
            if (strcmp(cmd, cmd_table[i].name) == 0)
            {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                ret_val;
            }
        }
        printf("Unknown command '%s'.\n", cmd);
    }
    ret_val;
}
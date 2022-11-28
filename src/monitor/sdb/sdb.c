#include "sdb.h"
#include <cpu/cpu.h>
#include <isa.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdarg.h>

static int is_batch_mode = false;

static char *stmt = NULL;
static bool error_unhandled = false; // global error flag

void init_wp_pool();

static void free_tokens(Token *tokens)
{
    if (tokens->type == TK_EOL)
    {

        free(tokens);
        return;
    }
    free_tokens(tokens->next);
}

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
    static char *line_read = NULL;

    if (line_read)
    {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read)
    {
        add_history(line_read);
    }

    return line_read;
}

/* output error info */
void sdb_error(char *loc, char *format, ...)
{
    va_list VA_ARGS;
    va_start(VA_ARGS, format);
    printf("Error: %*s", (int)(loc - stmt), "^");
    vprintf(format, VA_ARGS);
    printf("\n");
    va_end(VA_ARGS);
    error_unhandled = true;
}

void sdb_set_batch_mode()
{
    is_batch_mode = true;
}

#define check_error              \
    if (error_unhandled)         \
    {                            \
        error_unhandled = false; \
        continue;                \
    }

void sdb_mainloop()
{
    if (is_batch_mode)
    {
        //        cmd_c(NULL);
        return;
    }

    for (; (stmt = rl_gets()) != NULL;)
    {
        Token *tokens = tokenize(stmt);
        check_error;

        ASTNode *ast = parse(tokens);
        check_error;

        free_tokens(tokens);

        if ((ast->handler(ast)).i > 0)
            return;

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif
    }
}

void init_sdb()
{
    /* Initialize the watchpoint pool. */
    init_wp_pool();
}

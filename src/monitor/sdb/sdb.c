#include "sdb.h"
#include <cpu/cpu.h>
#include <isa.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdarg.h>

static int is_batch_mode = false;

static char *stmt = NULL;
static bool error_unhandled = false; // global error flag

void init_regex();
void init_wp_pool();

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

static int cmd_q(char *args)
{
    nemu_state.state = NEMU_QUIT;
    return -1;
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
        cmd_c(NULL);
        return;
    }

    for (char *str; (str = rl_gets()) != NULL;)
    {
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL)
        {
            continue;
        }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end)
        {
            args = NULL;
        }

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(cmd, cmd_table[i].name) == 0)
            {
                if (cmd_table[i].handler(args) < 0)
                {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD)
        {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}

void init_sdb()
{
    /* Compile the regular expressions. */
    init_regex();

    /* Initialize the watchpoint pool. */
    init_wp_pool();
}

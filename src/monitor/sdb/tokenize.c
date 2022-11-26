#include "sdb.h"
#include <ctype.h>

static Token *new_token(TokenKind kind, char *begin, char *end)
{
    Token *token = calloc(1, sizeof(Token));
    token->kind = kind;
    token->loc = begin;
    token->length = end - begin;
    return token;
}

bool token_equal(Token *token, char *str)
{
    return memcmp(token->loc, str, token->length) == 0 && str[token->length] == '\0';
}

#define next_token(kind, begin, end)         \
    cur->next = new_token(kind, begin, end); \
    cur = cur->next;

#define error(loc, format, ...)            \
    sdb_error(loc, format, ##__VA_ARGS__); \
    return head;

Token *tokenize(char *stmt)
{
    Token *head = new_token(TK_EOL, stmt, stmt);
    Token *cur = head;
    while (*stmt)
    {
        if (isspace(*stmt))
        {
            stmt++;
            continue;
        }
        char *beg;
        if (isdigit(*stmt))
        {
            next_token(TK_NUM, stmt, stmt);
            beg = stmt;
            word_t val = strtoul(stmt, &stmt, 10); // DEC
            if (val == 0)
            {
                if (*stmt == 'x') // HEX
                {
                    stmt++;
                    val = strtoul(stmt, &stmt, 16);
                }
            }
            cur->length = stmt - beg; // length of the string but the number
            cur->value = val;
            continue;
        }
        if (*stmt == '$') // reg
        {
            beg = stmt;
            do
            {
                stmt++;
            } while (isalpha(*stmt) || isdigit(*stmt));
            next_token(TK_REG, beg, stmt);
            continue;
        }
        if (ispunct(*stmt))
        {
            beg = stmt;
            stmt++;
            if (ispunct(*stmt))
            {
                stmt++;
            }
            next_token(TK_PUNCT, beg, stmt);
            continue;
        }
        if (isalpha(*stmt)) // cmd
        {
            beg = stmt;
            do
            {
                stmt++;
            } while (isalpha(*stmt) || isdigit(*stmt));
            if (*stmt != '\0' && *stmt != ' ')
            {
                error(stmt + 1, "Expected an space after commands.");
            }
            next_token(TK_CMD, beg, stmt);
            continue;
        }
        error(stmt, "Invalid Token.");
    }
    next_token(TK_EOL, stmt, stmt);
    return head->next;
}
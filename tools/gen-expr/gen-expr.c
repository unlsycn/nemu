#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// this should be enough
static char buf[65536] = {};
static int loc = 0;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format = "#include <stdio.h>\n"
                           "#include <stdint.h>\n"
                           "int main() { "
                           "  uint64_t result = %s; "
                           "  printf(\"%%lu\\n\", result); "
                           "  return 0; "
                           "}";

#define gen_spaces(n)               \
    for (int i = 0; i < r % n; i++) \
    {                               \
        buf[loc] = ' ';             \
        loc++;                      \
    }

#define gen_num(num)                 \
    sprintf(&buf[loc], "%dul", num); \
    loc += (int)(log10(num) + 3);

#define RANGE 1000;

static const char *op[] = {"+", "-", "*", "/", "%"};
static const int op_prob[] = {0, 400, 440, 600, 970, 1000};

static void gen_rand_expr(int n)
{
    int r = rand() % RANGE;
    gen_num(r + 1); // log10(0) equals -inf
    while (n > 0)
    {
        r = rand() % RANGE;
        for (int i = 0; i < 15; i++)
        {
            if (r >= op_prob[i] && r <= op_prob[i + 1])
            {
                sprintf(&buf[loc], "%s", op[i]);
                loc += strlen(op[i]);
                break;
            }
        }
        gen_spaces(4);
        if (r % 20)
        {
            gen_num(r + 1);
        }
        else
        {
            buf[loc++] = '(';
            int dn = n >= 4 ? r % (n / 4) + 1 : 1;
            gen_rand_expr(dn);
            n -= dn;
            buf[loc++] = ')';
        }
        gen_spaces(5);
        n--;
    }
    return;
}

int main(int argc, char *argv[])
{
    int seed = time(0);
    srand(seed);
    int loop = LOOP;
    if (argc > 1)
    {
        sscanf(argv[1], "%d", &loop);
    }
    int i, ret;
    system("rm /tmp/.result /tmp/.input");
    for (i = 0; i < loop;)
    {
        loc = 0;
        memset(buf, 0, sizeof(buf));
        gen_rand_expr(DEPTH);

        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);
        ret = system("clang -Werror /tmp/.code.c -o /tmp/.expr");
        if (ret != 0)
            continue;

        ret = system("/tmp/.expr >> /tmp/.result");
        assert(ret == 0);
        fp = fopen("/tmp/.input", "a+");
        assert(fp != NULL);
        fputs(buf, fp);
        fputs("\n", fp);
        fclose(fp);
        i++;
    }
    ret = system("sed -i 's/^/p /' /tmp/.input");
    ret += system("sed -i 's/ul//g' /tmp/.input");
    ret += system("sed -i '$a q' /tmp/.input");
    assert(ret == 0);
    return 0;
}

#include<stdio.h>

struct cpsr_state {
    int N;
    int Z;
    int C;
    int V;
};

int init_cpsr_state(struct cpsr_state *cpsr)
{
    cpsr->N = 0;
    cpsr->Z = 0;
    cpsr->C = 0;
    cpsr->V = 0;
}

int cmp(struct cpsr_state *cpsr, unsigned int a, unsigned int b) {
    int as, bs, result;
    long long al, bl;
    
    as = (int) a;
    bs = (int) b;
    al = (long long) a;
    bl = (long long) b;
    
    result = as - bs;

    cpsr->N = (result < 0);

    cpsr->Z = (result == 0);

    cpsr->C = (b > a);

    cpsr-> V = 0;
    if ((as > 0) && (bs < 0)) {
        if ((al + bl) > 0x7FFFFFFF) {
            cpsr->V = 1;
        }
    } else if ((as < 0) && (bs > 0)) {
        if ((al + bl) > 0x80000000) {
            cpsr->V = 1;
        }
    }

    return 0;
}

int cond_check(struct cpsr_state *cpsr, int a, int b)
{
    /* EQ */

    printf("EQ/NE\n");
    if (cpsr->Z) {
        printf("%d == %d\n", a, b);
    } else {
        printf("%d != %d\n", a, b);
    }

    /* LT */

    printf("LT\n");
    if (cpsr->N != cpsr->V) {
        printf("%d < %d\n", a, b);
    } else {
        printf("%d >= %d\n", a, b);
    }

    /* LE */

    printf("LE\n");
    if ((cpsr->Z == 1) || (cpsr->N != cpsr->V)) {
        printf("%d <= %d\n", a, b);
    } else {
        printf("%d > %d\n", a, b);
    }
    
    /* GT */

    printf("GT\n");
    if ((cpsr->Z == 0) && (cpsr->N == cpsr->V)) {
        printf("%d > %d\n", a, b);
    } else {
        printf("%d <= %d\n", a, b);
    }

    /* GE */
    
    printf("GE\n");
    if (cpsr->N == cpsr->V) {
        printf("%d >= %d\n", a, b);
    } else {
        printf("%d < %d\n", a, b);
    }
    
    return 0;
}

int main(int argc, char **argv)
{
    struct cpsr_state cpsr;

    init_cpsr_state(&cpsr);

    cmp(&cpsr, 3, 4);
    cond_check(&cpsr, 3, 4);

    cmp(&cpsr, 0, 0);
    cond_check(&cpsr, 0, 0);

    cmp(&cpsr, 7, 2);
    cond_check(&cpsr, 7, 2);

    cmp(&cpsr, -8, -100);
    cond_check(&cpsr, (unsigned int) -8, (unsigned int) -100);    
    
    return 0;
}

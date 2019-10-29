#include <stdbool.h>
#include <stdio.h>

#define NREGS 16
#define STACK_SIZE 1024
#define SP 13
#define LR 14
#define PC 15

/* Assembly functions to emulate */
int add_a(int a, int b);
int add2_a(int a, int b);
int sub_a(int a, int b);

/* The complete machine state */
struct arm_state {
    unsigned int regs[NREGS];
    unsigned int cpsr;
    unsigned char stack[STACK_SIZE];
};

/* The CPSR variables */
struct cpsr_state {
    int n;
    int z;
    int c;
    int v;
};
/**/
void init_cpsr_state(struct cpsr_state *cpsr)
{
    cpsr->n = 0;
    cpsr->z = 0;
    cpsr->c = 0;
    cpsr->v = 0;
}

/* Initialize an arm_state struct with a function pointer and arguments */
void arm_state_init(struct arm_state *as, unsigned int *func,
                    unsigned int arg0, unsigned int arg1,
                    unsigned int arg2, unsigned int arg3)
{
    int i;

    /* Zero out all registers */
    for (i = 0; i < NREGS; i++) {
        as->regs[i] = 0;
    }

    /* Zero out CPSR */
    as->cpsr = 0;

    /* Zero out the stack */
    for (i = 0; i < STACK_SIZE; i++) {
        as->stack[i] = 0;
    }

    /* Set the PC to point to the address of the function to emulate */
    as->regs[PC] = (unsigned int) func;

    /* Set the SP to the top of the stack (the stack grows down) */
    as->regs[SP] = (unsigned int) &as->stack[STACK_SIZE];

    /* Initialize LR to 0, this will be used to determine when the function has called bx lr */
    as->regs[LR] = 0;

    /* Initialize the first 4 arguments */
    as->regs[0] = arg0;
    as->regs[1] = arg1;
    as->regs[2] = arg2;
    as->regs[3] = arg3;
}

void arm_state_print(struct arm_state *as)
{
    int i;

    for (i = 0; i < NREGS; i++) {
        printf("reg[%d] = %d\n", i, as->regs[i]);
    }
    printf("cpsr = %X\n", as->cpsr);
}

void armemu_sub(struct arm_state *state)
{
    unsigned int iw;
    unsigned int rd, rn, rm;

    iw = *((unsigned int *) state->regs[PC]);
    
    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    rm = iw & 0xF;

    state->regs[rd] = state->regs[rn] - state->regs[rm];
    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

bool is_sub_inst(unsigned int iw) {
    
    unsigned int op;
    unsigned int opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0) && (opcode == 0b0010);
}

bool is_add_inst(unsigned int iw)
{
    unsigned int op;
    unsigned int opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0) && (opcode == 0b0100);
}

void armemu_add(struct arm_state *state)
{
    unsigned int iw;
    unsigned int rd, rn, rm;

    iw = *((unsigned int *) state->regs[PC]);
    
    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    rm = iw & 0xF;

    state->regs[rd] = state->regs[rn] + state->regs[rm];
    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

bool is_bx_inst(unsigned int iw)
{
    unsigned int bx_code;

    bx_code = (iw >> 4) & 0x00FFFFFF;

    return (bx_code == 0b000100101111111111110001);
}

void armemu_bx(struct arm_state *state)
{
    unsigned int iw;
    unsigned int rn;

    iw = *((unsigned int *) state->regs[PC]);
    rn = iw & 0b1111;

    state->regs[PC] = state->regs[rn];
}

bool is_b_inst(unsigned int iw)
{
    unsigned int b_code;
    
    b_code = (iw >> 25) & 0x7;

    return (b_code == 0b101);

}


void armemu_b(struct arm_state *state)
{
    unsigned int iw;
    unsigned int cond;
    unsigned int offset;
    unsigned int cpsr;

    /* Put cpsr into struct */

   /* iw = *((unsigned int *) state->regs[PC]);
    cond = (iw >> 28) & 0b1111;
    cpsr = state->cpsr;
    offset = (iw << 8);
    offset = (iw >> 8);
    

   if(is_bl_inst(iw)){
        state->regs[LR] = state->regs[PC]+4;
    }*/

    /* Change the PC to the new address now */
 

}

bool cond_check(struct arm_state *state)
{
   /* unsigned int iw;
    unsigned int cpsr;
    unsigned int cond;

    iw = *((unsigned int *) state->regs[PC]);
    cond = (iw >> 28) & 0b1111;



    switch (cond)
    {
        0b0000: 
	     Compare flags from cpsr

	0b0001:

	0b1011:

	0b1100;

        default:
    }*/

}

bool is_bl_inst(unsigned int iw) 
{
    unsigned int bl_code;

    bl_code = (iw >> 24) & 0xF;

    return (bl_code == 0b1011);

}

void is_data_processing_inst(struxt arm_state *state)
{
    unsigned int iw;
    iw = *((unsigned int *) state->regs[PC]);

    if (is_mul_inst(iw)) {
        armemu_mul(state)
    }
}

void armemu_one(struct arm_state *state)
{
    unsigned int iw;
    
    iw = *((unsigned int *) state->regs[PC]);

    if (is_bx_inst(iw)) {
        armemu_bx(state);
    } else if (is_add_inst(iw)) {
        armemu_add(state);
    } else if (is_sub_inst(iw)) {
	armemu_sub(state);
    }
}

unsigned int armemu(struct arm_state *state)
{
    /* Execute instructions until PC = 0 */
    /* This happens when bx lr is issued and lr is 0 */
    while (state->regs[PC] != 0) {
        armemu_one(state);
    }

    return state->regs[0];
}                
  
int main(int argc, char **argv)
{
    struct arm_state state;
    unsigned int r;




    /* Emulate add_a */
    arm_state_init(&state, (unsigned int *) add_a, 1, 2, 0, 0);
    arm_state_print(&state);
    r = armemu(&state);
    printf("armemu(add_a(1,2)) = %d\n", r);

    /* Emulate add2_a */
    arm_state_init(&state, (unsigned int *) add2_a, 1, 2, 0, 0);
    arm_state_print(&state);
    r = armemu(&state);
    printf("armemu(add2_a(1,2)) = %d\n", r);

    arm_state_init(&state, (unsigned int *) sub_a, 2, 1, 0, 0);
    arm_state_print(&state);
    r = armemu(&state);
    printf("armemu(sub_a(1,2)) = %d\n", r);
    
    return 0;
}

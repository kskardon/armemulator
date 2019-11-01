#include <stdio.h>
#include <stdbool.h>

#define NREGS 16
#define STACK_SIZE 1024
#define SP 13
#define LR 14
#define PC 15

/* Assembly functions to emulate */
int mov_a(int a, int b);
int add_a(int a, int b);
int sub_a(int a, int b);
int mul_a(int a, int b);
int cmp_a(int a, int b);
int cmpb_a(int a, int b);
int cmpbne_a(int a, int b);
int cmpbeq_a(int a, int b);
/*int beq_a(int a, int b);
int bne_a(int a, int b);*/

/* The complete cpsr */
struct cpsr_state {
    int N;
    int Z;
    int C;
    int V;
};

/* The complete machine as */
struct arm_state {
    unsigned int regs[NREGS];
    struct cpsr_state cpsr;
    unsigned char stack[STACK_SIZE];

    /* Dynamic Analysis: Three Types of Processing */
    int data_processing_count;
    int memory_count;
    int branch_count;

    /* Dynamic Analysis */
    int branches_taken;
    int branches_not_taken;

    /* Cache values */
    int request_count;
    int hit_count;
    int miss_count;

};

/* Initialize cpsr as */
int init_cpsr_state(struct cpsr_state *cpsr)
{
    cpsr->N = 0;
    cpsr->Z = 0;
    cpsr->C = 0;
    cpsr->V = 0;
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
    init_cpsr_state(&as->cpsr);

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

bool is_bx_inst(unsigned int iw)
{
    unsigned int bx_code;

    bx_code = (iw >> 4) & 0x00FFFFFF;

    return (bx_code == 0b000100101111111111110001);
}

void armemu_bx(struct arm_state *as)
{
    unsigned int iw;
    unsigned int rn;

    iw = *((unsigned int *) as->regs[PC]);
    rn = iw & 0b1111;

    as->regs[PC] = as->regs[rn];
}

bool is_mul_inst(unsigned int iw)
{
    //unsigned int op;
    //unsigned int opcode;
 
    unsigned int op;
    unsigned int opcode;
 
    op = (iw >> 22) & 0b111111; //getting the op code
    opcode = (iw >> 4) & 0b1111; //getting bits 4 through 7
 
    return (op == 0b000000) && (opcode == 0b1001);
}

void armemu_mul(struct arm_state *as)
{
    unsigned int iw;
    unsigned int rd, rs, rm;
 
    iw = *((unsigned int *) as->regs[PC]);
   
    rd = (iw >> 16) & 0xF;
   
    rs = (iw >> 8) & 0xF;
   
    rm = iw & 0xF;
 
    as->regs[rd] = as->regs[rm] * as->regs[rs];
   
    if (rd != PC)
    {
        as->regs[PC] = as->regs[PC] + 4;
    }
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

    cpsr->V = 0;
    if ((as > 0) && (bs < 0)) {
        if ((al + (-1 * bl)) > 0x7FFFFFFF) {
            cpsr->V = 1;
        }
    } else if ((as < 0) && (bs > 0)) {
        if (((-1 * al) + bl) > 0x80000000) {
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

bool is_b_inst(unsigned int iw)
{
    unsigned int b_code;
    
    b_code = (iw >> 25) & 0b111;

    return (b_code == 0b101);

}

int branch_condition(struct arm_state *as)
{
    unsigned int iw = *((unsigned int *) as->regs[PC]);
    unsigned int cond = (iw >> 28) & 0b1111;
   
    int branch = 0;
    switch(cond) {
	case 0b0000: // EQ
	//If the cases are equal, z is 1
	    if(as->cpsr.Z)
	    {
	        printf("NUMBERS ARE EQUAL");
	        branch = 1;
	    } else if(!as->cpsr.Z) {
                printf("NUMBERS ARE NOT EQUAL");
		branch = 0;
	    }
	    break;
	case 0b0001: // NE
	//The the cases are not equal, the 
	//i
	    if(as->cpsr.Z) 
	    {
		printf("NUMBERS ARE EQUAL");
	        branch =  1;
	    } else if(!as->cpsr.Z) {
                printf("NUMBERS ARE NOT  EQUAL");
		branch = 0;
	    }
	    break;
	case 0b1011: // LT
	    break;
	case 0b1100: // GT
	    break;
	case 0b1010: // GE
	    break;
	case 0b1101: // LE
	    break;
	case 0b1110:
	    branch = 1;
	    break;
    }
    return branch;

}

void armemu_b(struct arm_state *as)
{

    unsigned int iw = *((unsigned int *) as->regs[PC]);
    printf("WORD: %u\n", iw);
    unsigned int offset;
    unsigned int bl_bit = (iw >> 24) & 0b1;
    unsigned int sign;

    sign = (iw >> 23) & 0b1;

    if(sign) 
    {
        offset = (iw | 0xFF000000);
	offset = ((~offset) +1) * -1;

    } else {
        offset = (iw & 0xFFFFFF);
    }
    
    /* Shift bit left to truncate left bits*/

    /* Shift right to preserve MSB */

    

    printf("OFFSET: %u", offset);

    if(bl_bit)
    {
        as->regs[LR] = as->regs[PC]+4;
    }

    /* Change the PC to the new address now */
    unsigned int new_address = (offset*4) + 8;
    as->regs[PC] += new_address;


}


bool is_data_processing_inst(unsigned int iw)
{
    unsigned int dp_code;

    dp_code = (iw >> 26) & 0b11;

    return (dp_code == 0b00);
}

void armemu_data_processing(struct arm_state *as)
{
    /* Op1 is a/rn and Op2 is b/rm */
    unsigned int iw = *((unsigned int *) as->regs[PC]);
    unsigned int cond = (iw >> 28) & 0b1111;
    unsigned int imm_bit = (iw >> 25) & 0b1;
    unsigned int opcode = (iw >> 21) & 0b1111;
    unsigned int set_bit = (iw >> 20) & 0b1;
    unsigned int rn = (iw >> 16) & 0b1111;
    unsigned int rd = (iw >> 12) & 0b1111;
    unsigned int rm, imm;
    /* Values for CMP */
    int cs, bs, result;
    long long cl, bl;

	    
    
    /* Setting RM with either immediate value or RM */
    if(!imm_bit) {
        rm = iw & 0xF;
    } else {
        rm = iw & 0xFF;
    }

    switch(opcode) {
        case 0b1101: //mov
            as->regs[rd] = as->regs[rm];
            break;
        case 0b0100: //add
            as->regs[rd] = as->regs[rn] + as->regs[rm];
            break;
        case 0b0010: //sub
            as->regs[rd] = as->regs[rn] - as->regs[rm];
            break;
        case 0b1010: //cmp
            cmp(&as->cpsr, as->regs[rn], as->regs[rm]);
	    break;

    }
        
    if (rd != PC)
    {
        as->regs[PC] += 4;
    }
}

void armemu_one(struct arm_state *as)
{   
    unsigned int iw;
    
    iw = *((unsigned int *) as->regs[PC]);
    /* Simulate chache here*
     * simulate_cache(cache, PC)
     * PC
     * slot = get_slot(size, addr)
     * tag = get_tag(size, addr)
     * Look at notes to see if this particular address is in the cache or not*/   

    /* Cache data structure... array of cache slots
     * Cache slot: v bit, tag */
 

    if (is_bx_inst(iw)) {
        armemu_bx(as);
    } else if (is_mul_inst(iw)) {
        armemu_mul(as);
    } else if (is_data_processing_inst(iw)) {
    	armemu_data_processing(as);
    } else if (is_b_inst(iw)) {
        if(branch_condition(as)) {
	/* If branch condition, then branch*/
            armemu_b(as);
	} else {
            as->regs[PC] += 4;
	}
    }
}

unsigned int armemu(struct arm_state *as)
{
    /* Execute instructions until PC = 0 */
    /* This happens when bx lr is issued and lr is 0 */
    while (as->regs[PC] != 0) {
        armemu_one(as);
    }

    return as->regs[0];
}       

int main(int argc, char **argv)
{
    struct arm_state as;
    unsigned int r;
    
    /* Emulate add_a */
    arm_state_init(&as, (unsigned int *) add_a, 5, 3, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(add_a(1,2)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) sub_a, 5, 3, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(sub_a(2,1)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) mul_a, 5, 3, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(mul_a(2,1)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) mov_a, 5, 3, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(mov_a(2,1)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) cmp_a, 5, 3, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(cmp_a(2,1)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) cmpb_a, 5, 7, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(cmpb_a(2,1)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) cmpbne_a, 5, 7, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(cmpbne_a(5,7)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) cmpbeq_a, 5, 7, 0, 0);
    arm_state_print(&as);
    r = armemu(&as);
    printf("armemu(cmpbeq_a(5,7)) = %d\n", r);



    
    


    return 0;
}

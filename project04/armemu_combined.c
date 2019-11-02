#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define NREGS 16
#define STACK_SIZE 1024
#define SP 13
#define LR 14
#define PC 15

/* Assembly functions to emulate */
int quadratic_c(int x, int a, int b, int c);
int quadratic_a(int x, int a, int b, int c);
int find_max_a(int *array, int len);
int fib_rec_a(int n);

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
    float data_processing_count;
    float memory_count;
    float branch_count;

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

void dynamic_analysis_init(struct arm_state *as) 
{
    as->data_processing_count = 0;
    as->branch_count = 0;
    as->memory_count = 0;
    as->branches_taken = 0;
    as->branches_not_taken = 0;

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
        as->regs[PC] += 4;
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
	    if(as->cpsr.Z) {
	        branch = 1;
	    }
	    break;
	case 0b0001: // NE
	    if(!as->cpsr.Z) {
	        branch = 1;
	    } 
	    break;
	case 0b1011: // LT
	    if(as->cpsr.N != as->cpsr.V) {
		branch = 1;
	    }
	    break;
	case 0b1100: // GT
            if((as->cpsr.N == as->cpsr.V) && !as->cpsr.Z) {
                branch = 1;
	    }
	    break;
	case 0b1010: // GE
            if(as->cpsr.N == as->cpsr.V) {
                branch = 1;
	    }
	    break;
	case 0b1101: // LE
	    if((as->cpsr.N != as->cpsr.V) || as->cpsr.Z) {
                branch = 1;
	    }
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
    unsigned int imm = (iw >> 25) & 0b1;
    unsigned int opcode = (iw >> 21) & 0b1111;
    unsigned int set_bit = (iw >> 20) & 0b1;
    unsigned int rn = (iw >> 16) & 0b1111;
    unsigned int rd = (iw >> 12) & 0b1111;
    unsigned int rm;
    /* Values for CMP */
    int cs, bs, result;
    long long cl, bl;

	    
    
    /* Setting RM with either immediate value or RM */
    if(!imm) {
        rm = as->regs[iw & 0xF];
    } else {
        rm = iw & 0xFF;
    }

    switch(opcode) {
        case 0b1101: //mov
            as->regs[rd] = rm;
            break;
        case 0b0100: //add
            as->regs[rd] = as->regs[rn] + rm;
            break;
        case 0b0010: //sub
            as->regs[rd] = as->regs[rn] - rm;
            break;
        case 0b1010: //cmp
            cmp(&as->cpsr, as->regs[rn], rm);
	    break;

    }
        
    if (rd != PC)
    {
        as->regs[PC] += 4;
    }
}

bool is_sdt_inst(unsigned int iw) 
{
    unsigned int sdt_code;

    sdt_code = (iw >> 26) & 0b11;

    return (sdt_code == 0b01);

}

void armemu_sdt(struct arm_state *as)
{
    unsigned int iw = *((unsigned int *) as->regs[PC]);
    unsigned int cond = (iw >> 28) & 0b1111;
    unsigned int immediate = (iw >> 25) & 0b1;
    unsigned int updown = (iw >> 23) & 0b1;
    unsigned int byteword = (iw >> 22) & 0b1;
    unsigned int loadstore = (iw >> 20) & 0b1;
    unsigned int rn = as->regs[(iw >> 16) & 0xF];
    unsigned int rd = (iw >> 12) & 0xF;
    unsigned int imm = iw & 0xF;
    unsigned int rm_offset; 
    unsigned int target_address;
    
    
        
    /* Check if the rm is rm or immediate value */
    if(!immediate) {

	/* Sets immediate offset */
        rm_offset = iw & 0xFFF;

           /* If the given value is register instead of an immediate */
    } else if(immediate) {

	/* Returns offset value stored within Rm register */
        rm_offset = as->regs[iw & 0xF];

       
    }

    /* If immediate value is negative */
    if(updown == 0) {
        /* Perform two's complement */
        rm_offset = ((~rm_offset) +1) * -1;

        /* Subtract offset from rn to get target address */
        target_address = rn - rm_offset;
    } else {
    /* If immediate value is positive, add value to rn */
        target_address = rn + rm_offset;
    }

    
    /* If loading from memory */
    if(loadstore) { 
       
	/* If it is a byte */
	if (byteword) {
	    as->regs[rd] =  *((unsigned char *) target_address);

           /* as->regs[rd] = */
	} else if (!byteword) {
	    /* Get the value from the stack */
            
	    as->regs[rd] = *((unsigned int *)target_address);
	}


    /* If storing to memory */
    } else if(!loadstore) {
        /* Store from memory */
	if(byteword == 1) {
	    /* Store byte */ 
	    *((unsigned char *)target_address) = as->regs[rd];
            

	} else if (byteword == 0) {
            /* Store word */

            *((unsigned int *)target_address) = as->regs[rd]; 

	}
    }
    as->regs[PC] += 4;
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
	as->branches_not_taken++;
	as->branch_count++;
        armemu_bx(as);
    } else if (is_mul_inst(iw)) {
	as->data_processing_count++;
        armemu_mul(as);
    } else if (is_data_processing_inst(iw)) {
	as->data_processing_count++;
    	armemu_data_processing(as);
    } else if (is_b_inst(iw)) {
        if(branch_condition(as)) {
	    as->branches_taken++;
	    as->branch_count++;
            armemu_b(as);
	} else {
            as->regs[PC] += 4;
	    as->branches_not_taken++;
	    as->branch_count++;
	}
    } else if (is_sdt_inst(iw)) {
	as->memory_count++;
        armemu_sdt(as);
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

void dynamic_analysis(struct arm_state *as)
{
    float total_inst = as->branch_count + as->data_processing_count + as->memory_count;
    float data_per = (as->data_processing_count / total_inst) * 100;
    float branch_per = (as->branch_count / total_inst) * 100;
    float mem_per = (as->memory_count / total_inst) * 100;
    printf("Dynamic Analysis:\n");
    printf("\t# of instructions:\t%u\n", (unsigned int)total_inst);
    printf("\t# of data processing:\t%u\t%.f%\n", (unsigned int)as->data_processing_count, data_per);
    printf("\t# of branch:\t\t%u\t%.f%\n", (unsigned int)as->branch_count, branch_per);
    printf("\t# of memory:\t\t%u\t%.f%\n", (unsigned int)as->memory_count, mem_per);
    printf("\tbranches taken:\t\t%u\n", (unsigned int)as->branches_taken);
    printf("\tbranches not taken:\t%u\n", (unsigned int)as->branches_not_taken);

}

void quadratic_emulator()
{
    
}

int main(int argc, char **argv)
{
    struct arm_state as;
    unsigned int r;
    dynamic_analysis_init(&as);

    int arr1[] = {1,0,22,0,3,1,2,3,-1,5,6,-7,8,9};
    /* QUADRATIC */
    r = quadratic_c(-2,2,-3,4);
    printf("quadratic_c(-2,2,-3,4) = %d\n", r);
    r = quadratic_a(-2,2,-3,4);
    printf("quadratic_a(-2,2,-3,4) = %d\n", r);
    arm_state_init(&as, (unsigned int *) quadratic_a, -2, 2, -3, 4);
    r = armemu(&as);
    printf("quadtratic_a[emu](-2,2,-3,4) = %d\n", r);



    arm_state_init(&as, (unsigned int *) find_max_a, (unsigned int) arr1,  5, 0, 0);
    r = armemu(&as);
    printf("armemu(find_max(1,2)) = %d\n", r);

    arm_state_init(&as, (unsigned int *) fib_rec_a, (unsigned int) 7, 0, 0, 0);
    r = armemu(&as);
    printf("armemu(fib_rec(7)) = %d\n", r);


    dynamic_analysis(&as);

    return 0;
}

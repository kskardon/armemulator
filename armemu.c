#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define NREGS 16
#define STACK_SIZE 1024
#define SP 13
#define LR 14
#define PC 15

/* Assembly functions to emulate */
int quadratic_c(int x, int a, int b, int c);
int quadratic_a(int x, int a, int b, int c);
int sum_array_c(int *array, int len);
int sum_array_a(int *array, int len);
int find_max_c(int *array, int len);
int find_max_a(int *array, int len);
int fib_rec_c(int n);
int fib_rec_a(int n);
int fib_iter_c(int n);
int fib_iter_a(int n);
int strlen_c(char* x);
int strlen_a(char* x);

/* The complete cpsr */
struct cpsr_state {
    int N;
    int Z;
    int C;
    int V;
};

struct cache_slot {
    unsigned int v;
    unsigned int tag;
};

struct cache_direct_mapped {
    struct cache_slot *slots;
    int hits;
    int misses;
    int requests;
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

    /* Cache */
    struct cache_direct_mapped dmc;
    unsigned int cache_size = 32;
};

int cache_direct_mapped_init(struct cache_direct_mapped *dmc) {
    
    dmc->hits = 0;
    dmc->misses = 0;
    dmc->requests = 0;
    dmc->slots (struct cache_slot *) malloc(sizeof(struct cache_slot) * cache_size);
}

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

/* Initialize dynamic analysis values */
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

/* Used to parse command line for cache size */
int str_to_int(char *s)
{
    int neg = 0;
    int i = 0;
    if(s[0] == '-') {
        neg = 1;
        i = 1;
    }

    int integer = 0;
    for(i = i; s[i] != '\0'; i++) {
        integer *= 10;
	integer += (int)(s[i]-'0');
    }

    if(neg == 1) {
        integer *= -1;
    }
    return integer;
}

bool is_bx_inst(unsigned int iw)
{
    unsigned int bx_code = (iw >> 4) & 0x00FFFFFF;

    return (bx_code == 0b000100101111111111110001);
}

void armemu_bx(struct arm_state *as)
{
    unsigned int iw = *((unsigned int *) as->regs[PC]);
    unsigned int rn = iw & 0b1111;

    as->regs[PC] = as->regs[rn];
}

bool is_mul_inst(unsigned int iw)
{ 
    unsigned int op = (iw >> 22) & 0b111111; 
    unsigned int opcode = (iw >> 4) & 0b1111;
  
    return (op == 0b000000) && (opcode == 0b1001);
}

void armemu_mul(struct arm_state *as)
{
    unsigned int iw = *((unsigned int *) as->regs[PC]);
    unsigned int rd = (iw >> 16) & 0xF;
    unsigned int rs = (iw >> 8) & 0xF;
    unsigned int rm = iw & 0xF;
   
    as->regs[rd] = as->regs[rm] * as->regs[rs];
   
    if (rd != PC)
    {
        as->regs[PC] += 4;
    }
}

int cmp(struct cpsr_state *cpsr, unsigned int a, unsigned int b) 
{
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
	case 0b1110: // AL
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
    unsigned int sign = (iw >> 23) & 0b1;

    if(sign) 
    {
        offset = (iw | 0xFF000000);
	offset = ((~offset) + 1) * -1;
    } else {
        offset = (iw & 0xFFFFFF);
    }
    
    if(bl_bit)
    {
        as->regs[LR] = as->regs[PC] + 4;
    }
    
    /* Change the PC to the new address now */
    as->regs[PC] += (offset * 4) + 8;
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
    unsigned int iw = *((unsigned int *) as->regs[PC]);
   
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
    while (as->regs[PC] != 0) {
        armemu_one(as);
    }
    return as->regs[0];
}   

void dynamic_analysis(struct arm_state *as)
{
    float total_inst = 0;
    total_inst = as->branch_count + as->data_processing_count + as->memory_count;
    float taken_per = (as->branches_taken / as->branch_count) * 100;
    float not_taken_per = (as->branches_not_taken / as->branch_count) * 100;
    float data_per = (as->data_processing_count / total_inst) * 100;
    float branch_per = (as->branch_count / total_inst) * 100;
    float mem_per = (as->memory_count / total_inst) * 100;
    printf("\nDynamic Analysis:\n");
    printf("\t# of instructions:\t%u\n", (unsigned int)total_inst);
    printf("\t# of data processing:\t%u\t%.f%\n", (unsigned int)as->data_processing_count, data_per);
    printf("\t# of branch:\t\t%u\t%.f%\n", (unsigned int)as->branch_count, branch_per);
    printf("\t# of memory:\t\t%u\t%.f%\n", (unsigned int)as->memory_count, mem_per);
    printf("\tbranches taken:\t\t%u\t%.f%\n", (unsigned int)as->branches_taken, taken_per);
    printf("\tbranches not taken:\t%u\t%.f%\n\n", (unsigned int)as->branches_not_taken, not_taken_per);
}

void quadratic_emulator(struct arm_state *as)
{  
    dynamic_analysis_init(as);
    printf("QUADRATIC\n");
    printf("\tquadratic_c(-2,2,-3,4) =\t%d\n", quadratic_c(-2,2,-3,4));
    printf("\tquadratic_a(-2,2,-3,4) =\t%d\n", quadratic_a(-2,2,-3,4));
    arm_state_init(as, (unsigned int *) quadratic_a, -2,2,-3,4);
    printf("\tquadratic_a[emu](-2,2,-3,4) = \t%d\n", armemu(as));

    printf("\tquadratic_c(20,16,10,4) = \t%d\n", quadratic_c(20,16,10,4));
    printf("\tquadratic_a(20,16,10,4) = \t%d\n", quadratic_a(20,16,10,4));
    arm_state_init(as, (unsigned int *) quadratic_a, 20,16,10,4);
    printf("\tquadratic_a[emu](20,16,10,4) = \t%d\n", armemu(as));

    printf("\tquadratic_c(-3,1,7,6) = \t%d\n", quadratic_c(-3,1,7,6));
    printf("\tquadratic_a(-3,1,7,6) = \t%d\n", quadratic_a(-3,1,7,6));
    arm_state_init(as, (unsigned int *) quadratic_a, -3,1,7,6);
    printf("\tquadratic_a[emu](-3,1,7,6) = \t%d\n", armemu(as));

    printf("\tquadratic_c(9,2,-3,0) = \t%d\n", quadratic_c(9,2,-3,0));
    printf("\tquadratic_a(9,2,-3,0) = \t%d\n", quadratic_a(9,2,-3,0));
    arm_state_init(as, (unsigned int *) quadratic_a, 9,2,-3,0);
    printf("\tquadratic_a[emu](9,2,-3,0) = \t%d\n", armemu(as));
    dynamic_analysis(as);
}

int print_array(int *array, int len)
{
    int i;
    
    if(len > 20) {
        len = 20;
    }
    printf("{");
    for (i = 0; i < len; i++) {
        printf("%d", array[i]);
        if (i < (len - 1)) {
            printf(",");
        }
    }
    printf("}\n");
}

void find_max_emulator(struct arm_state *as)
{
    dynamic_analysis_init(as);
    int arr1[] = {1,0,22,0,3,1,2,3,-1,5,6,-7,8,9};
    int arr2[] = {-1,-2,-3,-4};   	
    int arr3[1000];
    int arr4[] = {0,0,0,0,1,2};
    int r;

    int array_of_lens[4] = {5, 1000, 4, 6};
    int *array_of_arrays[4];
    char *array_of_titles[4] = {"FIND_MAX Positive Cases\n",
	    "FIND_MAX 1000 Length Array\n",
	    "FIND_MAX Negative Cases\n",
	    "FIND_MAX Zero Cases\n"};

    /* Populate an array with 1000 elements */
    for(int i = 0; i < 1000; i++) {
        if(i%2 == 0) {
	    arr3[i] = 3;
	} else {
            arr3[i] = -2;
	}
    }
    arr3[2] = 88;

    array_of_arrays[0] = arr1;
    array_of_arrays[1] = arr3;
    array_of_arrays[2] = arr2;
    array_of_arrays[3] = arr4;

    int j;
    for(j = 0; j < 4; j++) {
        printf(array_of_titles[j]);
	print_array(array_of_arrays[j], array_of_lens[j]);
        r = find_max_c(array_of_arrays[j], array_of_lens[j]);
        printf("\tfind_max_c(%d) = \t\t%d\n", array_of_lens[j], r);
        r = find_max_a(array_of_arrays[j], array_of_lens[j]);
        printf("\tfind_max_a(%d) = \t\t%d\n", array_of_lens[j], r);
        arm_state_init(as, (unsigned int *) find_max_a, (unsigned int) array_of_arrays[j], array_of_lens[j], 0, 0);
	if(array_of_lens[j] > 10) {
            printf("\tfind_max_a[emu](%d) = \t%d\n", array_of_lens[j], armemu(as));
	} else {
            printf("\tfind_max_a[emu](%d) = \t\t%d\n", array_of_lens[j], armemu(as));

	}
    }
    dynamic_analysis(as);
}

void print_sum_array(int *array, int len)
{
    int i;
    if(len > 20) {
        len = 20;
    }
    printf("{");
    for (i = 0; i < len; i++) {
        printf("%d", array[i]);
        if (i < (len - 1)) {
            printf(",");
        }
    }
    printf("}\n");
}

void sum_array_emulator(struct arm_state *as)
{
    dynamic_analysis_init(as);
    int arr1[] = {1,0,2,0,3,1,2,3,-1,5,6,-7,8,9};
    int arr2[] = {-1,-2,-3,-4};
    int arr3[1000];
    int arr4[] = {0,0,0,0,1,2};
    int r;

    int array_of_lens[4] = {5, 1000, 4, 6};
    int *array_of_arrays[4];
    char *array_of_titles[4] = {"SUM_ARRAY Positive Cases\n",
	    "SUM_ARRAY 1000 Length Array\n",
	    "SUM_ARRAY Negative Cases\n",
	    "SUM_ARRAY Zero Cases\n"};

    /* Populate an array with 1000 elements */
    for(int i = 0; i < 1000; i++) {
        if(i%2 == 0) {
	    arr3[i] = 3;
	} else {
            arr3[i] = -2;
	}
    }

    array_of_arrays[0] = arr1;
    array_of_arrays[1] = arr3;
    array_of_arrays[2] = arr2;
    array_of_arrays[3] = arr4;

    int j;
    for(j = 0; j < 4; j++) {
        printf(array_of_titles[j]);
	print_sum_array(array_of_arrays[j], array_of_lens[j]);
        r = sum_array_c(array_of_arrays[j], array_of_lens[j]);
        printf("\tsum_array_c(%d) = \t\t%d\n", array_of_lens[j], r);
        r = sum_array_a(array_of_arrays[j], array_of_lens[j]);
        printf("\tsum_array_a(%d) = \t\t%d\n", array_of_lens[j], r);
        arm_state_init(as, (unsigned int *) sum_array_a, (unsigned int) array_of_arrays[j], array_of_lens[j], 0, 0);
	if(array_of_lens[j] > 10) {
            printf("\tsum_array_a[emu](%d) = \t%d\n", array_of_lens[j], armemu(as));
	} else {
            printf("\tsum_array_a[emu](%d) = \t\t%d\n", array_of_lens[j], armemu(as));

	}
    }
    dynamic_analysis(as);
}

void fib_rec_emulator(struct arm_state *as) 
{
    dynamic_analysis_init(as);
    printf("FIB_REC\n");

    int j;
    for(j = 0; j < 20; j++) {
        if(j < 10) {
            printf("\tfib_rec_c(%d) = \t\t%d\n", j, fib_rec_c(j));
            printf("\tfib_rec_a(%d) = \t\t%d\n", j, fib_rec_a(j)); 
	} else {
            printf("\tfib_rec_c(%d) = \t%d\n", j, fib_rec_c(j));
            printf("\tfib_rec_a(%d) = \t%d\n", j, fib_rec_a(j)); 
        }
	arm_state_init(as, (unsigned int *) fib_rec_a, j,0,0,0);
        printf("\tfib_rec_a[emu](%d) = \t%d\n", j, armemu(as));  	
    }
    dynamic_analysis(as);
}

void fib_iter_emulator(struct arm_state *as) 
{   
    dynamic_analysis_init(as);
    printf("FIB_ITER\n"); 
    int j;
    for(j = 0; j < 20; j++) {
        printf("\tfib_iter_c(%d) = \t%d\n", j, fib_iter_c(j));
	printf("\tfib_iter_a(%d) = \t%d\n", j, fib_iter_a(j)); 
        arm_state_init(as, (unsigned int *) fib_iter_a, j,0,0,0);
        printf("\tfib_iter_a[emu](%d) = \t%d\n", j, armemu(as));
    }
    dynamic_analysis(as);
}

void strlen_emulator(struct arm_state *as)
{
    dynamic_analysis_init(as);
    char string[] = "Howdy There!!!";
    char string1[] = "ssstttrrriiinnnggg";
    char string2[] = "CS315 is Awesome";
    char string3[] = "!@#$#$##@";
    char string4[] = "1234567890";
    char *array_of_strings[5] = {string, string1, string2, string3, string4};
    
    int j;
    printf("STRLEN\n");

    for(j = 0; j < 5; j++) {
        printf("%s\n", array_of_strings[j]);
        printf("\tstrlen_c = \t%d\n", strlen_c(array_of_strings[j]));
        printf("\tstrlen_a = \t%d\n", strlen_a(array_of_strings[j]));
        arm_state_init(as, (unsigned int *) strlen_a, (unsigned int) array_of_strings[j],0,0,0);
        printf("\tstrlen_a[emu] = %d\n", armemu(as));
    }
    dynamic_analysis(as);
}

unsigned int get_slot(unsigned int cache_size, unsigned int address) 
{
    return ((address >> 2) & (cache_size - 1));
}

int get_tag(unsigned int cache_size, unsigned int address) 
{ 

    /*TODO: fill in */	
}

int get_cache_vbit(struct arm_state *as, int slot) 
{
    return as->dmc.slots[slot].v;
}

int get_cache_tag(struct arm_state *as, int slot) 
{
    return as->dmc.slots[slot].tag;
}

void update_cache(struct arm_state *as, int slot, int tag) 
{
    as->dmc.slots[slot].v = 1;
    as->dmc.slots[slot].tag = tag;
}

void simulate_cache(struct arm_state *as, unsigned int address) 
{
    unsigned int slot = get_slot(state->cache_size, address);
    unsigned int tag = get_tag(state->cache_size, address);
    unsigned int v = get_cache_vbit(as, slot);
    unsigned int tag_c = get_cache_tag(as, slot); 

    if (!v) {
        as->dmc.misses++;
	update_cache(as, slot, tag);
    } else {
        if (tag == tag_c){
        state->dmc.hits++;
        } else {
            state->dmc.misses++;
            update_cache(as, slot, tag);
        }
    }
    as->dmc.requests++;
}

void check_cache_size(int argc, char **argv, struct arm_state *as) 
{
    /* Sort through each argument */
    for(int i = 0; i < argc; i++) {
        /* If arg is tag -c */
    	printf(argv[i]);
        exit(0);	
        if(argv[i][0] == '-' && argv[i][1] == 'c') {

	    printf("GOT HERE: %s", argv[i+1]);

	   /* int arg_to_int = str_to_int((argv[i + 1]));	
            /* If cache_size is divisible by 2
    	    if(arg_to_int%2 == 0) {
		/* Store the cache_size in the struct 
	        as->cache_size = (unsigned int) argv[i+1];
	    } else {
	        printf("Error: given cache size must be 2^N.");
		exit(0);
	    }
	} else {
	    printf("Error: No cache size given.");
	    exit(0);*/
	}
    }
}

int main(int argc, char **argv)
{
    struct arm_state as;

    /* Intake size for cache here */
    check_cache_size(argc, argv, &as);
 
    quadratic_emulator(&as);
    sum_array_emulator(&as);
    find_max_emulator(&as);
    fib_rec_emulator(&as);
    fib_iter_emulator(&as);
    strlen_emulator(&as);
   
    return 0;
}

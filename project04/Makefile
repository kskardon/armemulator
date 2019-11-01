PROGS = mov_a add_a mul_a sub_a cmp_a cmpb_a cmpbne_a cmpbeq_a cmpblt_a ldr_a str_a strb_a

OBJS_ARMEMU = mov_a.o add_a.o mul_a.o sub_a.o cmp_a.o cmpb_a.o cmpbne_a.o cmpbeq_a.o cmpblt_a.o ldr_a.o str_a.o strb_a.o


all : ${PROGS}

strb_a : strb_a.s
	as -o strb_a.o strb_a.s

ldr_a : ldr_a.s
	as -o ldr_a.o ldr_a.s

str_a : str_a.s
	as -o str_a.o str_a.s

cmpblt_a : cmpblt_a.s
	as -o cmpblt_a.o cmpblt_a.s

cmpbne_a : cmpbne_a.s
	as -o cmpbne_a.o cmpbne_a.s

cmpbeq_a : cmpbeq_a.s
	as -o cmpbeq_a.o cmpbeq_a.s

cmpb_a : cmpb_a.s
	as -o cmpb_a.o cmpb_a.s

cmp_a : cmp_a.s
	as -o cmp_a.o cmp_a.s

mov_a : mov_a.s
	as -o mov_a.o mov_a.s

add_a : add_a.s
	as -o add_a.o add_a.s

mul_a: mul_a.s
	as -o mul_a.o mul_a.s

sub_a: sub_a.s
	as -o sub_a.o sub_a.s

armemu_combined : armemu_combined.c ${OBJS_ARMEMU}
	gcc -o armemu_combined armemu_combined.c ${OBJS_ARMEMU}

test : armemu_combined
	./armemu_combined

clean :
	rm -rf armemu_combined ${OBJS_ARMEMU}

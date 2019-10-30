PROGS = mov_a add_a mul_a sub_a cmp_a cmpb_a

OBJS_ARMEMU = mov_a.o add_a.o mul_a.o sub_a.o cmp_a.o cmpb_a.o


all : ${PROGS}

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

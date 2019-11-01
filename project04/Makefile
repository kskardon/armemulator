PROGS = find_max_a
OBJS_ARMEMU = find_max_a.o

all : ${PROGS}
find_max_a : find_max_a.s
	as -o find_max_a.o find_max_a.s

armemu_combined : armemu_combined.c ${OBJS_ARMEMU}
	gcc -o armemu_combined armemu_combined.c ${OBJS_ARMEMU}

test : armemu_combined
	./armemu_combined

clean :
	rm -rf armemu_combined ${OBJS_ARMEMU}

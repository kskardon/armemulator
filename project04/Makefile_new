PROGS = fib_iter_a fib_rec_a strlen_a quadratic_a sum_array_a find_max_a 

OBJS_ARMEMU = fib_iter_a.o fib_rec_a.o strlen_a.o quadratic_a.s sum_array_a.s find_max_a.s


all : ${PROGS}

fib_iter_a : fib_iter_a.s
	as -o fib_iter_a.o fib_iter_a.s

fib_rec_a : fib_rec_a.s
	as -o fib_rec_a.o fib_rec_a.s

strlen_a: strlen_a.s
	as -o strlen_a.o strlen_a.s

quadratic_a: quadratic_a.s
	as -o quadratic_a.o quadratic_a.s

sum_array_a : sum_array_a.s
	as -o sum_array_a.o sum_array_a.s

find_max_a : find_max_a.s
	as -o find_max_a.o find_max_a.s

armemu : armemu_new.c ${OBJS_ARMEMU}
	gcc -o armemu_new armemu_new.c ${OBJS_ARMEMU}

test : armemu_new
	./armemu_new

clean :
	rm -rf armemu_new ${OBJS_ARMEMU}

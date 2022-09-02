CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=c11 #-O2 original
CFLAGS += -Wmissing-prototypes -Wvla
CPPFLAGS := -D_DEFAULT_SOURCE
VFLAGS=--leak-check=full --track-origins=yes --track-fds=yes --show-reachable=yes --error-exitcode=-1 --quiet

PROGS := pingpong primes find xargs

all: $(PROGS)

find: find.o
xargs: xargs.o
primes: primes.o
pingpong: pingpong.o

# =========================================================================

format: lab-fork/fork/.clang-files lab-fork/fork/.clang-format
	xargs -r clang-format -i <$<

clean:
	rm -f $(PROGS) *.o core vgcore.*

# =========================================================================

comp_pingpong: lab-fork/fork/pingpong.c
	gcc $(CFLAGS) -o pingpong.out lab-fork/fork/pingpong.c

valgrind_pingpong: comp_pingpong
	valgrind $(VFLAGS) ./pingpong.out

# =========================================================================

comp_primes: lab-fork/fork/primes.c
	gcc $(CFLAGS) -o primes.out lab-fork/fork/primes.c

valgrind_primes: comp_primes
	valgrind $(VFLAGS) ./primes.out

# =========================================================================

comp_find: lab-fork/fork/find.c
	gcc $(CFLAGS) -o find.out lab-fork/fork/find.c

valgrind_find: comp_find
	valgrind $(VFLAGS) ./find.out

# =========================================================================

comp_xargs: lab-fork/fork/xargs.c
	gcc $(CFLAGS) -o xargs.out lab-fork/fork/xargs.c

valgrind_xargs: comp_xargs
	valgrind $(VFLAGS) ./xargs.out

# =========================================================================


.PHONY: all clean format

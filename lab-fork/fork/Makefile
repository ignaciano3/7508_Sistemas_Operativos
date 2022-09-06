CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=c11
CFLAGS += -Wmissing-prototypes -Wvla
CPPFLAGS := -D_DEFAULT_SOURCE

PROGS := pingpong primes find xargs

VFLAGS:=--leak-check=full --track-origins=yes --track-fds=yes --show-reachable=yes --error-exitcode=-1 --quiet

PRIMES_ARG ?= 10

FIND_ARGS ?= -i .c

XARGS_ARG ?= ls

all: $(PROGS)

find: find.o
xargs: xargs.o
primes: primes.o
pingpong: pingpong.o




format: .clang-files .clang-format
	xargs -r clang-format -i <$<

clean:
	rm -f $(PROGS) *.o core vgcore.*



# =========================================================================

comp_pingpong: pingpong.c
	gcc $(CFLAGS) pingpong.c -o pingpong.o

valgrind_pingpong: comp_pingpong
	valgrind $(VFLAGS) ./pingpong.o

# =========================================================================

comp_primes: primes.c
	gcc $(CFLAGS) primes.c -o primes.o

valgrind_primes: comp_primes
	valgrind $(VFLAGS) ./primes.o $(PRIMES_ARG)

# =========================================================================

comp_find: find.c
	gcc $(CFLAGS) find.c -o find.o

valgrind_find: comp_find
	valgrind $(VFLAGS) ./find.o $(FIND_ARGS)

# =========================================================================

comp_xargs: xargs.c
	gcc $(CFLAGS) xargs.c -o xargs.o

valgrind_xargs: comp_xargs
	valgrind $(VFLAGS) ./xargs.o $(XARGS_ARG)

# =========================================================================



.PHONY: all clean format

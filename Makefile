CFLAGS := -std=c11 -Wall -Wextra -pedantic -g

.PHONY: all clean test

all: vxc

vxc: $(wildcard src/*.c src/*.h)
	$(CC) $(CFLAGS) -o $@ $(filter %.c, $^)

clean:
	rm -f vxc test/test.o test/test.out

test: vxc
	./vxc test/test.vx > test/test.o
	ld test/test.o -o test/test.out
	./test/test.out | xxd

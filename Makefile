CFLAGS := -std=c11 -Wall -Wextra -pedantic

.PHONY: all clean test

all: vxc

vxc: $(wildcard src/*.c src/*.h)
	$(CC) $(CFLAGS) -o $@ $(filter %.c, $^)

clean:
	rm -f vxc test.o test.out

test: vxc
	./vxc > test.o
	ld test.o -o test.out
	./test.out | xxd

CFLAGS := -std=c11 -Wall -Wextra -pedantic -g

.PHONY: all clean test test-record

all: vxc

vxc: $(wildcard src/*.c src/*.h)
	$(CC) $(CFLAGS) -o $@ $(filter %.c, $^)

clean:
	rm -f vxc
	rm -f $(shell find test -name "*.o")
	rm -f $(shell find test -name "*.out")

test: vxc
	test/test.sh $(shell find test -name "*.vx")

test-record: vxc
	test/test.sh --record $(shell find test -name "*.vx")

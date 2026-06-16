.PHONY: all test loader launcher clean

all: test loader launcher
test:
	$(MAKE) -C test

loader:
	$(MAKE) -C loader

launcher:
	$(MAKE) -C launcher

run:
	./bin/launch ./test/fib

clean:
	$(MAKE) -C test clean
	$(MAKE) -C loader clean
	$(MAKE) -C launcher clean
	rm -rf bin


all: ../bin/libsimpleloader.so
	mkdir -p ../bin
	gcc -Wall -Wextra -m32 -o ../bin/launch launch.c -I../loader -L../bin -Wl,-rpath,'$$ORIGIN' -lsimpleloader

clean:
	-@rm -f ../bin/launch


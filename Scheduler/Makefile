
all: SimpleShell job


SimpleShell: SimpleShell.c SimpleScheduler.c
	gcc -o SimpleShell SimpleShell.c SimpleScheduler.c -Wall


job: job.c
	gcc -o job job.c -Wall


clean:
	rm -f SimpleShell job
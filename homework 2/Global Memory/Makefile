CC = gcc
CFLAGS = -O3 -framework OpenCL

blurer: blur.c
	$(CC) $(CFLAGS) blur.c -o blur

.phony: clean

clean:
	rm -f blur

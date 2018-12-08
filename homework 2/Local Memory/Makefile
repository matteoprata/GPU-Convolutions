CC = gcc
CFLAGS = -O3 -framework OpenCL

blurer: blur_local.c
	$(CC) $(CFLAGS) blur_local.c -o blur_local

.phony: clean

clean:
	rm -f blur_local

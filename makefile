all:
	cc -o prog main.c -lm -L./ -lraylib
	./prog

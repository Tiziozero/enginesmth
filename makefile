all:
	cc -o prog main.c editor.c -lm -L./ -lraylib
	./prog

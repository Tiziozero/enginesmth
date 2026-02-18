all:
	cc -ggdb -o prog main.c editor.c -lm -L./ -lraylib -lvterm -lm
	./prog

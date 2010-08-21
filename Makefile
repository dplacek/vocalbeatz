all: main sine

main: main.c
	gcc -g3 main.c -o main

sine: sine.c
	gcc -g3 sine.c -o sine -lm

clean:
	rm -f main sine

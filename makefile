main: main.c graph.c
	gcc -o main main.c graph.c -I. -g

clean:
	rm -rf main
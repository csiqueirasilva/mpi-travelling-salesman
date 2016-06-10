main: main.c graph.c
	mpicc main.c graph.c -o main

clean:
	rm -rf main
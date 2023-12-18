all: clean trace

trace: LC4.o loader.o trace1.c
	clang -g LC4.o loader.o trace1.c -o trace

LC4.o: loader.c
	clang -c LC4.c 

loader.o: loader.c
	clang -c loader.c
clean:
	rm -rf *.o

clobber: clean
	rm -rf trace

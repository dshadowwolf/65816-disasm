.c.o:
	gcc -c -O2 -g -ggdb $< -o $@
	
tester: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o main.o
	gcc -o $@ $^

clean:
	rm -f *.o tester


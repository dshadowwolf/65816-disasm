.c.o:
	gcc -c -O2 $< -o $@
	
tester: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o main.o  processor.o processor_helpers.o
	gcc -o $@ $^

lib: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o processor.o processor_helpers.o
	ar rcs lib65816disasm.a $^
	ranlib lib65816disasm.a

clean:
	rm -f *.o tester lib65816disasm.a


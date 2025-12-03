.c.o:
	gcc -c -O2 $< -o $@
	
tester: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o main.o  processor.o processor_helpers.o
	gcc -o $@ $^

test_processor: test_processor.o processor.o processor_helpers.o state.o
	gcc -o $@ $^

lib: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o processor.o processor_helpers.o
	ar rcs lib65816disasm.a $^
	ranlib lib65816disasm.a

test: test_processor
	./test_processor

clean:
	rm -f *.o tester test_processor lib65816disasm.a


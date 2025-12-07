.c.o:
	gcc -c -O0 -ggdb $< -o $@
	
tester: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o main.o  processor.o processor_helpers.o
	gcc -o $@ $^

test_processor: test_processor.o processor.o processor_helpers.o state.o machine_setup.o
	gcc -o $@ $^

test_via: test_via.o via6522.o
	gcc -o $@ $^

test_pia: test_pia.o pia6521.o
	gcc -o $@ $^

test_acia: test_acia.o acia6551.o
	gcc -o $@ $^

lib: list.o map.o codetable.o outs.o map.o tbl.o state.o disasm.o processor.o processor_helpers.o machine_setup.o via6522.o pia6521.o acia6551.o
	ar rcs lib65816disasm.a $^
	ranlib lib65816disasm.a

test: test_processor
	./test_processor

clean:
	rm -f *.o tester test_processor test_via test_pia test_acia lib65816disasm.a


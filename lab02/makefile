all:
	nasm -f elf -o my_print.o my_print.asm
	g++ -o run main.cpp my_print.o
	./run
.PHONY: clean
clean:
	-rm -f *.o
	-rm -f run 

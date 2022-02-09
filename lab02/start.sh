nasm -f elf -o my_print.o my_print.asm
g++ -o run main.cpp my_print.o
rm my_print.o
./run
rm run

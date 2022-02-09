nasm -f elf -g add_mul.asm
gcc -g -o test add_mul.o
rm ./add_mul.o

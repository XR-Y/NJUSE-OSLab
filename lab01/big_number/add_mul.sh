nasm -f elf -F dwarf -g add_mul.asm
ld -m elf_i386 add_mul.o -o runnable
rm add_mul.o
./runnable
rm runnable

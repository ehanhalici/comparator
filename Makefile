main:
	gcc -c -g comparator.c -o comparator.o
	gcc -c -g cstring.c -o cstring.o
	gcc -o comparator comparator.o cstring.o -lncurses -lpanel
	rm comparator.o cstring.o

link: compile
	gcc disassembler.o cstring.o -o disassembler -g
	rm disassembler.o
	rm cstring.o

compile:
	gcc -c disassembler.c -o disassembler.o -g
	gcc -c cstring.c -o cstring.o -g

do:
	gcc -c main.c -o main.o -g
	objdump --no-addresses   --no-show-raw-insn -S  -M intel -d main.o > main.asm



clean:
	rm main.o main.asm
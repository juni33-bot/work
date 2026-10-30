all: hello
	@echo "build finished!"

hello: main.o func.o
	aarch64-none-linux-gnu-gcc -o hello main.o func.o

main.o: main.c
	aarch64-none-linux-gnu-gcc -c main.c

func.o: func.c
	aarch64-none-linux-gnu-gcc -c func.c

clean:
	@rm -f hello main.o func.o
	@echo "clean finished!"

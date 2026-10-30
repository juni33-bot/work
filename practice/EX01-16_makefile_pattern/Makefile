TARGET = hello
OBJS = main.o func.o
CC = aarch64-none-linux-gnu-gcc
CFLAGS = -c

all: $(TARGET)
	@echo "build finished!"

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $< 

clean:
	@rm -f $(TARGET) $(OBJS)
	@echo "clean finished!"


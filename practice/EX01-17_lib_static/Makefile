TARGET = mycalStatic1
OBJS = main.o
LIB_DIR = lib
LIB_TARGET = $(LIB_DIR)/libmycal.a
LIB_OBJS = $(LIB_DIR)/plus.o $(LIB_DIR)/minus.o
CC = aarch64-none-linux-gnu-gcc
AR = aarch64-none-linux-gnu-ar
CFLAGS = -c

all: $(TARGET)
	@echo "build finished!"

$(TARGET): $(OBJS) $(LIB_TARGET)
	$(CC) -o $@ $^
	sudo cp $(TARGET) /nfsroot

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) crv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $< 

clean:
	@rm -f $(TARGET) $(OBJS) $(LIB_TARGET) $(LIB_OBJS)
	@echo "clean finished!"


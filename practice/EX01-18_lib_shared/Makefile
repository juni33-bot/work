TARGET = mycalDynamic
OBJS = main.o
LIB_DIR = lib

LIB_ORG = libmycal.so.1.2
LIB_SONAME = libmycal.so.1
LIB_LINK = libmycal.so
LIB_ORG_PATH = $(LIB_DIR)/$(LIB_ORG)
LIB_SONAME_PATH = $(LIB_DIR)/$(LIB_SONAME)
LIB_LINK_PATH = $(LIB_DIR)/$(LIB_LINK)
LIB_OBJS = $(LIB_DIR)/plus.o $(LIB_DIR)/minus.o
LIB_NAME = mycal
CC = aarch64-none-linux-gnu-gcc
CFLAGS = -c

all: $(TARGET)
	@echo "build finished!"

$(TARGET): $(OBJS) $(LIB_ORG_PATH)
	$(CC) -o $@ $(OBJS) -L$(LIB_DIR) -l$(LIB_NAME)
	sudo cp -f $(LIB_ORG_PATH) /nfsroot
	sudo cp -af $(LIB_SONAME_PATH) /nfsroot
	sudo cp -af $(LIB_LINK_PATH) /nfsroot
	sudo cp -f $(TARGET) /nfsroot

$(LIB_ORG_PATH): $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,$(LIB_SONAME) -o $@ $^
	ln -sf $(LIB_ORG) $(LIB_SONAME_PATH)
	ln -sf $(LIB_ORG) $(LIB_LINK_PATH)

$(LIB_DIR)/%.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -fPIC -o $@ $< 

%.o: %.c
	$(CC) $(CFLAGS) $< 

clean:
	@rm -f $(TARGET) $(OBJS) $(LIB_ORG_PATH) $(LIB_OBJS) $(LIB_SONAME_PATH) $(LIB_LINK_PATH)
	@echo "clean finished!"


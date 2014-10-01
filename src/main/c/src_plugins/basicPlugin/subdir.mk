ANALYSIS+=analysis/basicPlugin
ANALYSIS_LIB+=lib/libbasicPlugin.so
OBJS_FILES += src_plugins/basicPlugin/*.o

# Source lists
SRCS:=src_plugins/basicPlugin/basicPlugin.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/basicPlugin: src_plugins/basicPlugin/libbasicPlugin.so

src_plugins/basicPlugin/libbasicPlugin.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libbasicPlugin.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
	mv $@ lib/libbasicPlugin.so



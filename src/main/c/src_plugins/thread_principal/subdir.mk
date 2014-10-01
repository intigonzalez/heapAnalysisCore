ANALYSIS+=analysis/thread_principal
ANALYSIS_LIB+=lib/libthread_principal.so
OBJS_FILES += src_plugins/thread_principal/*.o

# Source lists
SRCS:=src_plugins/thread_principal/thread_principal.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/thread_principal: src_plugins/thread_principal/libthread_principal.so

src_plugins/thread_principal/libthread_principal.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libthread_principal.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
	mv $@ lib/libthread_principal.so

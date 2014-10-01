ANALYSIS+=analysis/threadgroup_principal
ANALYSIS_LIB+=lib/libthreadgroup_principal.so
OBJS_FILES += src_plugins/threadgroup_principal/*.o

# Source lists
SRCS:=src_plugins/threadgroup_principal/threadgroup_principal.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/threadgroup_principal: src_plugins/threadgroup_principal/libthreadgroup_principal.so

src_plugins/threadgroup_principal/libthreadgroup_principal.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libthreadgroup_principal.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
	mv $@ lib/libthreadgroup_principal.so

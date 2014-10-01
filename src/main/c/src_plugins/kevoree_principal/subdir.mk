
ANALYSIS+=analysis/kevoree_principal
ANALYSIS_LIB+=lib/libkevoree_principal.so
OBJS_FILES += src_plugins/kevoree_principal/*.o

# Source lists
SRCS:=src_plugins/kevoree_principal/kevoree_principal.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/kevoree_principal: src_plugins/kevoree_principal/libkevoree_principal.so

src_plugins/kevoree_principal/libkevoree_principal.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libkevoree_principal.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
	mv $@ lib/libkevoree_principal.so

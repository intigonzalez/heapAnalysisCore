ANALYSIS+=analysis/jvm_principal
ANALYSIS_LIB+=lib/libjvm_principal.so
OBJS_FILES += src_plugins/jvm_principal/*.o

# Source lists
SRCS:=src_plugins/jvm_principal/jvm_principal.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/jvm_principal: src_plugins/jvm_principal/libjvm_principal.so

src_plugins/jvm_principal/libjvm_principal.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libjvm_principal.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
	mv $@ lib/libjvm_principal.so
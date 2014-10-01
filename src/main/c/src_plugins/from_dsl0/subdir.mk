
ANALYSIS+=analysis/from_dsl0
ANALYSIS_LIB+=lib/libfrom_dsl0.so
OBJS_FILES += src_plugins/from_dsl0/*.o

# Source lists
SRCS:=src_plugins/from_dsl0/list.c src_plugins/from_dsl0/common.c src_plugins/from_dsl0/simple_accounting.c src_plugins/from_dsl0/from_dsl0.c src_plugins/from_dsl0/RuntimeObjects.c 

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/from_dsl0: src_plugins/from_dsl0/libfrom_dsl0.so

src_plugins/from_dsl0/libfrom_dsl0.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libfrom_dsl0.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
    mv $@ lib/libfrom_dsl0.so

src_plugins/from_dsl0/%.o: src_plugins/from_dsl0/%.c
	$(CC) $(CFLAGS) -std=c99 -c -o $@ $< 


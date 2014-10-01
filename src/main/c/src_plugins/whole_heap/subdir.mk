
ANALYSIS+=analysis/whole_heap
ANALYSIS_LIB+=lib/libwhole_heap.so
OBJS_FILES += src_plugins/whole_heap/*.o

# Source lists
SRCS:=src_plugins/whole_heap/whole_heap.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/whole_heap: src_plugins/whole_heap/libwhole_heap.so

src_plugins/whole_heap/libwhole_heap.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libwhole_heap.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer
	mv $@ lib/libwhole_heap.so
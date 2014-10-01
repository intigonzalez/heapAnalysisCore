# Source lists
LIBNAME=heapViewer
SOURCES=heapViewer.c agent_util.c ResourceManagement.c

# Linux GNU C Compiler
ifeq ($(OSNAME), linux)
    # GNU Compiler options needed to build it
    COMMON_FLAGS=-fno-strict-aliasing -fPIC -fno-omit-frame-pointer
    # Options that help find errors
    COMMON_FLAGS+= -W  -Wno-parentheses # -Wall -Wunused-parameter
    ifeq ($(OPT), true)
        CFLAGS=-O2 $(COMMON_FLAGS) 
    else
        CFLAGS=-g $(COMMON_FLAGS) 
    endif
    # Object files needed to create library
    OBJECTS=$(SOURCES:%.c=%.o)
    # Library name and options needed to build it
    LIBRARY=lib$(LIBNAME).so
    LDFLAGS=-Wl,-soname=$(LIBRARY) -static-libgcc
    # Libraries we are dependent on
    LIBRARIES=-lc -ldl
    # Building a shared library
    LINK_SHARED=$(LINK.c) -shared -o $@
endif

# Common -I options
CFLAGS += -I.
CFLAGS += -I$(JDK)/include -I$(JDK)/include/$(OSNAME)

# Default rule
all: $(LIBRARY)

ANALYSIS:=

ANALYSIS_LIB:=

OBJS_FILES:=

include src_plugins/basicPlugin/subdir.mk
include src_plugins/jvm_principal/subdir.mk
include src_plugins/thread_principal/subdir.mk
include src_plugins/threadgroup_principal/subdir.mk
include src_plugins/whole_heap/subdir.mk
#include src_plugins/from_dsl0/subdir.mk
include src_plugins/kevoree_principal/subdir.mk

#LOCAL_PATH:=/home/inti/programs/heapAnalysisCore/src/localMavenRepo/repo/

#JAVA_PATHS := org/heapexplorer/heapanalysis/AnalysisType.java org/heapexplorer/heapanalysis/HeapAnalysis.java org/heapexplorer/heapanalysis/ClassDetailsUsage.java org/heapexplorer/heapanalysis/UpcallGetObjects.java

#JAVA_CLASS_PATHS := $(JAVA_PATHS:%.java=%.class)

# Build native library
$(LIBRARY): $(OBJECTS)
	$(LINK_SHARED) $(OBJECTS) $(LIBRARIES)
	cp libheapViewer.so ../resources/

#plugins

analysis/%: src_plugins/%
	@echo "Done $<"
plugins: $(ANALYSIS)
	@echo "Done $<"

installplugins:
	echo $(ANALYSIS_LIB) | tr " " "\n" > config.ini
	cp config.ini ../resources/
	cp -r lib ../resources/

# Cleanup the built bits
clean:
	rm -f $(LIBRARY) $(OBJECTS)
	rm -f $(ANALYSIS_LIB)
	rm -f $(OBJS_FILES)

# Simple tester
test: all
	LD_LIBRARY_PATH=`pwd` $(JDK)/bin/java -agentlib:$(LIBNAME) -version


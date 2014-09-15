#
# Copyright (c) 2004, 2005, Oracle and/or its affiliates. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   - Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#   - Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#   - Neither the name of Oracle nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

########################################################################
#
# Sample GNU Makefile for building JVMTI Demo heapViewer
#
#  Example uses:    
#       gnumake JDK=<java_home> OSNAME=linux   [OPT=true]
#
########################################################################

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
all: $(LIBRARY) heapanalysis.jar

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

LOCAL_PATH:=/home/inti/programs/jdk1.7.0_65_demos/demo/jvmti/heap\ analyzer/src/localMavenRepo/repo/

JAVA_PATHS := org/heapexplorer/heapanalysis/AnalysisType.java org/heapexplorer/heapanalysis/HeapAnalysis.java org/heapexplorer/heapanalysis/ClassDetailsUsage.java

JAVA_CLASS_PATHS := $(JAVA_PATHS:%.java=%.class)

# Build native library
$(LIBRARY): $(OBJECTS)
	$(LINK_SHARED) $(OBJECTS) $(LIBRARIES)

# Jar file
heapanalysis.jar:  Test.java Test2.java
	cd src_java && javac $(JAVA_PATHS)
	cd src_java && jar cf ../heapanalysis.jar $(JAVA_CLASS_PATHS)
	javac -cp heapanalysis.jar Test.java
	#javac Test2.java
	mvn deploy:deploy-file -Durl=file:///home/inti/programs/jdk1.7.0_65_demos/demo/jvmti/heap\ analyzer/src/localMavenRepo/repo/ -Dfile=heapanalysis.jar -DgroupId=org.heapexplorer -DartifactId=org.heapexplorer.heapanalysis -Dpackaging=jar -Dversion=1.0

#plugins
analysis/%: src_plugins/%
	@echo "Done $<"
plugins: $(ANALYSIS)
	@echo "Done $<"

#install plugins
installplugins:
	echo $(ANALYSIS_LIB) | tr " " "\n" > config.ini

# Cleanup the built bits
clean:
	rm -f $(LIBRARY) $(OBJECTS)
	rm -f heapexplorer.jar *.class
	rm -f $(ANALYSIS_LIB)
	rm -f $(OBJS_FILES)

# Simple tester
test: all
	LD_LIBRARY_PATH=`pwd` $(JDK)/bin/java -agentlib:$(LIBNAME) -version


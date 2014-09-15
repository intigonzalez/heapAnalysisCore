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
#       gnumake JDK=<java_home> OSNAME=solaris [OPT=true] [LIBARCH=sparc]
#       gnumake JDK=<java_home> OSNAME=solaris [OPT=true] [LIBARCH=sparcv9]
#       gnumake JDK=<java_home> OSNAME=linux   [OPT=true]
#       gnumake JDK=<java_home> OSNAME=win32   [OPT=true]
#
########################################################################

ANALYSIS+=analysis/whole_heap
ANALYSIS_LIB+=src_plugins/whole_heap/libwhole_heap.so
OBJS_FILES += src_plugins/whole_heap/*.o

# Source lists
SRCS:=src_plugins/whole_heap/whole_heap.c

#Objects
OBJS:=$(SRCS:%.c=%.o)

src_plugins/whole_heap: src_plugins/whole_heap/libwhole_heap.so

src_plugins/whole_heap/libwhole_heap.so: $(OBJS)
	$(CC) $(CFLAGS) -Wl,-soname=libwhole_heap.so -static-libgcc -L. -shared -o $@ $^ -lc -lheapViewer

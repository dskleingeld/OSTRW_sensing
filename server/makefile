#this checks how many threads the building machine has and passes that
#to 'make' for multithreaded compiling (speeds up compile time)
MAKEFLAGS+="-j -l $(shell grep -c ^processor /proc/cpuinfo) "
#set the standard shell for make to bash shell
SHELL := /bin/bash

CXX = g++
CXXFLAGS= -Wall -pedantic -std=c++14 -g #-pedantic

#this prevents obj files from flooding the source dir
ODIR = .BUILD
OBJS = $(addprefix $(ODIR)/,main.o mainServer.o)

#these 3 lines allow 'make debug' to have debug prints and 'make' not
all: checkLibs directories server 
debug: CXXFLAGS += -DDEBUG -g
debug: checkLibs directories server


server: $(OBJS)
	$(CXX) $(CXXFLAGS) -o server $(OBJS) \
	-lmicrohttpd

$(ODIR)/main.o: main.cpp mainServer.h
	$(CXX) $(CXXFLAGS) -o$@ -c main.cpp

$(ODIR)/mainServer.o: mainServer.cpp mainServer.h
	$(CXX) $(CXXFLAGS) -lmicrohttpd -o$@ -c mainServer.cpp

#non compiling (phony) command to clean temp files
.PHONY : clean
clean:
	rm server $(OBJS)
	
#non compiling (phony) command to create dir to keep obj files
.PHONY: directories
directories: ${ODIR}

${ODIR}:
	mkdir -p ${ODIR}
	
.PHONY: checkLibs
checkLibs: 
	@(ld -lmicrohttpd 2>/dev/null || (echo -e "please install libmicrohttpd\n" \
	" for debian/ubuntu: sudo apt-get install libmicrohttpd-dev"))

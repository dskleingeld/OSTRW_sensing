#this checks how many threads the building machine has and passes that
#to 'make' for multithreaded compiling (speeds up compile time)
MAKEFLAGS+="-j -l $(shell grep -c ^processor /proc/cpuinfo) "

CXX = g++
CXXFLAGS= -Wall -pedantic -std=c++14 #-pedantic

#this prevents obj files from flooding the source dir
ODIR = .BUILD
OBJS = $(addprefix $(ODIR)/,main.o)

#these 3 lines allow 'make debug' to have debug prints and 'make' not
all: directories pi 
debug: CXXFLAGS += -DDEBUG -g
debug: directories pi


pi: $(OBJS)
	$(CXX) $(CXXFLAGS) -o pi $(OBJS) \
	-lmicrohttpd

$(ODIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(ROOTFLAGS) -o$@ -c main.cpp

$(ODIR)/mainServer.o: httpServer/mainServer.cpp httpServer/mainServer.h \
	httpServer/pages/webGraph.h
	$(CXX) $(CXXFLAGS) -lmicrohttpd -o$@ -c httpServer/mainServer.cpp

#non compiling (phony) command to clean temp files
.PHONY : clean
clean:
	rm pi $(OBJS)
	
#non compiling (phony) command to create dir to keep obj files
.PHONY: directories
directories: ${ODIR}

${ODIR}:
	mkdir -p ${ODIR}

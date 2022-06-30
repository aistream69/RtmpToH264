CPP = g++
CPPFLAGS = -std=c++11
CPPFLAGS += -Wall -O2 
CPPFLAGS += -Wno-deprecated-declarations 
CPPFLAGS += -Wno-sign-compare -Wno-reorder
CPPFLAGS += -Iinclude

LDFLAGS = -Llib
LDFLAGS += -lavformat -lavfilter -lavcodec -lavutil
LDFLAGS += -lpthread
LDFLAGS += -fPIE -Wl,-rpath,lib

SRC = main.cpp 
all:
	$(CPP) $(CPPFLAGS) $(SRC) $(LDFLAGS) -o test

clean:
	rm -f test


TARGETS = scope dump sendF0
all:		$(TARGETS)

PCLIBS = SDL2_gfx
CXXFLAGS = -std=c++11 -ggdb3
LDFLAGS = -ggdb3
LDLIBS = -lm

CXXFLAGS += $(shell pkg-config --cflags $(PCLIBS))
LDLIBS += $(shell pkg-config --libs $(PCLIBS))

CC=gcc

.PHONY:		clean

clean:
		rm -f $(TARGETS) *.o

run:		scope
		./scope

%.o:		%.cc common.h
		$(CXX) -c $(CFLAGS) $(CXXFLAGS) $^

scope:		scope.o ProggyOpti.o screen.o ui.o
		$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

dump:		dump.o
		$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

sendF0:		sendF0.o
		$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)


CXX=gcc
CXXFLAGS=-g -Wall
BIN=mac_add
SRCS=mac_add.o

%.o : %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@

OBJS   = $(patsubst %.c,%.o,$(SRCS))

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN)

clean:
	rm -f $(OBJS) $(BIN)

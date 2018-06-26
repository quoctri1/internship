CXX=gcc
CXXFLAGS=-g -Wall
BIN=icmp
SRCS=icmp.o

%.o : %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@

OBJS   = $(patsubst %.c,%.o,$(SRCS))

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN)

clean:
	rm -f $(OBJS) $(BIN)

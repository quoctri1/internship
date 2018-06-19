CXX=gcc 
CXXFLAGS=-O3 -Wall
BIN=http_server
SRCS=http_server.o

%.o : %.c
	$(CXX) $(CXXFLAGS) $< -c -o $@

OBJS   = $(patsubst %.c,%.o,$(SRCS))

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN)

clean:
	rm -f $(OBJS) $(BIN)

export PATH:=/home/pqtri/code/project/embedded_linux_skeleton/build/buildroot/host/usr/bin:$(PATH)
CXX=arm-linux-gcc
CXXFLAGS=-g -Wall
BIN=arp
SRCS=arp.o

%.o : %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@

OBJS   = $(patsubst %.c,%.o,$(SRCS))

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN)

clean:
	rm -f $(OBJS) $(BIN)

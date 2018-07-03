CXX=gcc
CXXFLAGS=-Wall
EXE=lldp
SRCS=lldp.o

%.o: %.c
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(EXE): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(EXE)

clean:
	rm -f $(EXE) $(SRCS)

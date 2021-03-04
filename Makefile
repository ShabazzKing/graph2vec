CXX = g++
CFLAGS = -c -Wall -pedantic -std=c++17
PROGRAM = graph2vec
OBJS = Main.o Graph.o SubgraphExtract.o word2vec.o

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(OBJS) -ljsoncpp -o $@

Main.o: Main.cpp
	$(CXX) $< $(CFLAGS) -o $@

%.o: %.cpp
	$(CXX) $< $(CFLAGS) -o $@

clean:
	rm -f $(PROGRAM) $(OBJS)

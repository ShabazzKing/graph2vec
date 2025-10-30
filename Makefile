CXX = g++
CFLAGS = -c -Wall -pedantic -std=c++17
PROGRAM = graph2vec
OBJS = Main.o Graph.o SubgraphExtract.o word2vec.o
JSONFLAGS = `pkg-config --cflags --libs jsoncpp`

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(OBJS) $(JSONFLAGS) -o $@

Main.o: Main.cpp
	$(CXX) $< $(CFLAGS) $(JSONFLAGS) -o $@

%.o: %.cpp
	$(CXX) $< $(CFLAGS) $(JSONFLAGS) -o $@

clean:
	rm -f $(PROGRAM) $(OBJS)

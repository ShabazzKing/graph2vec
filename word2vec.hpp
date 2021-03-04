#ifndef WORD2VEC_HPP
#define WORD2VEC_HPP

#include <vector>
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

void word2vec(SubgrSubMap &, ReverseSubgraphsMap &, RadialContext &, 
              const Graph &, unsigned, unsigned, unsigned, double, unsigned);

#endif

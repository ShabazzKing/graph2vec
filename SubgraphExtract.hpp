#ifndef SUBGRAPHEXTRACT_HPP
#define SUBGRAPHEXTRACT_HPP

#include <string>
#include <vector>
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

std::string getWLSubgraph(SubgraphsMap &, ReverseSubgraphsMap &, const Graph &, 
                          const Graph::Vertex *, unsigned, unsigned);

void radialSkipGram(RadialContext &, SubgraphsMap &, const std::vector<Graph> &, 
                    unsigned);

void radialSkipGramCore(RadialContext &, SubgraphsMap &, unsigned, const Graph &, 
                        const Graph::Vertex *, unsigned, unsigned);

#endif

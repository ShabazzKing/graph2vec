#ifndef SUBGRAPHMAPS_HPP
#define SUBGRAPHMAPS_HPP

#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>
#include "Graph.hpp"

// For every graph, root vertex and degree of rooted subgraph map subgraph ID,
// subgraph string and subgraph embedding
typedef std::map<const Graph *, std::map<const Graph::Vertex *, 
    std::map<unsigned, 
    std::pair<unsigned, std::pair<std::string, std::vector<long double>>>>>> SubgraphsMap;

// This is submap of map above
typedef std::map<const Graph::Vertex *, std::map<unsigned, 
    std::pair<unsigned, std::pair<std::string, std::vector<long double>>>>> SubgrSubMap;

// For every rooted subgraph (ID) map multiset of subgraphs (ID), which are in the radial
// context of this subgraph
typedef std::map<unsigned, std::multiset<unsigned>> RadialContext;

// For every rooted subgraph ID map parent graph, root vertex and degree of this subgraph
typedef std::map<unsigned, std::pair<const Graph *, std::pair<const Graph::Vertex *, 
    unsigned>>> ReverseSubgraphsMap;

#endif

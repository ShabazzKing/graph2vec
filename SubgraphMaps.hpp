#ifndef SUBGRAPHMAPS_HPP
#define SUBGRAPHMAPS_HPP

#include <map>
#include <set>

// For every rooted subgraph (ID) map multiset of subgraphs (ID), which are in the radial
// context of this subgraph
typedef std::map<unsigned, std::multiset<unsigned>> RadialContext;

#endif

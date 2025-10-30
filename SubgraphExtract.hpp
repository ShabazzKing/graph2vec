#ifndef SUBGRAPHEXTRACT_HPP
#define SUBGRAPHEXTRACT_HPP

#include <string>
#include <vector>
#include <fstream>
#include <json/json.h>
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

void getWLSubgraph(Json::Value &, const Graph &, const Graph::Vertex *, unsigned, unsigned, unsigned, unsigned);

void radialSkipGram(RadialContext &, const std::vector<std::string> &, const std::vector<Graph> &, unsigned);

void radialSkipGramCore(RadialContext &, Json::Value &, unsigned, const Graph &, const Graph::Vertex *, unsigned, unsigned);

#endif

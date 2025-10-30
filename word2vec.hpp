#ifndef WORD2VEC_HPP
#define WORD2VEC_HPP

#include <vector>
#include <json/json.h>
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

void word2vec(Json::Value &, RadialContext &, const Graph &, unsigned, unsigned, unsigned, double, unsigned);

#endif

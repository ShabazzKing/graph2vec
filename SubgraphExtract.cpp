#include <random>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <utility>
#include "Graph.hpp"
#include "SubgraphMaps.hpp"
#include "SubgraphExtract.hpp"

// Extract rooted subgraphs information
std::string getWLSubgraph(SubgraphsMap & subgraphs, ReverseSubgraphsMap & revSubgraphs, 
                          const Graph & graph, const Graph::Vertex * node, 
                          unsigned degree, unsigned dimensions)
{
    static unsigned subgraphID = 0;
    unsigned tempID = subgraphID;
    std::string subgraphString("");
    // If this subgraph is already in map of subgraphs, return it
    if (subgraphs.count(&graph) == 1 && subgraphs[&graph].count(node) == 1 && 
        subgraphs[&graph][node].count(degree) == 1)
        subgraphString = subgraphs[&graph][node][degree].second.first;
    else
    {
        if (degree == 0)
            subgraphString = std::to_string(node->getLabel());
        else
        {
            std::set<const Graph::Vertex *> adjacentNodes;
            for (unsigned i = 0; i < graph.getMaxVertex(); i++)
                if (graph.getEdge(node->getNumber(), i) != nullptr)
                    adjacentNodes.insert(graph.getVertex(i));
            // Building subgraph strings
            std::set<std::string> subgraphStrings;
            for (std::set<const Graph::Vertex *>::iterator i = adjacentNodes.cbegin(); 
                 i != adjacentNodes.cend(); i++)
                subgraphStrings.insert(getWLSubgraph(subgraphs, revSubgraphs, graph, *i, 
                                                     degree - 1, dimensions));
            subgraphString = subgraphString + getWLSubgraph(subgraphs, revSubgraphs, 
                                                            graph, node, degree - 1, 
                                                            dimensions);
            std::string temp("");
            for (std::set<std::string>::iterator i = subgraphStrings.cbegin(); 
                 i != subgraphStrings.cend(); i++)
                temp = temp + *i;
            subgraphString = subgraphString + temp;
        }
        // Increment unique subgraph ID
        subgraphID++;
        if (subgraphs.count(&graph) == 0)
            subgraphs[&graph] = std::map<const Graph::Vertex *, std::map<unsigned, 
            std::pair<unsigned, std::pair<std::string, std::vector<long double>>>>>();
        if (subgraphs[&graph].count(node) == 0)
            subgraphs[&graph][node] = std::map<unsigned, 
            std::pair<unsigned, std::pair<std::string, std::vector<long double>>>>();
        if (subgraphs[&graph][node].count(degree) == 0)
        {
            subgraphs[&graph][node][degree] = 
            std::pair<unsigned, std::pair<std::string, std::vector<long double>>>();
            subgraphs[&graph][node][degree].first = tempID;
            subgraphs[&graph][node][degree].second.first = subgraphString;
            std::random_device dev;
            std::uniform_real_distribution<long double> unidist(-1.0L, 1.0L);
            // Generate random vector representations of subgraphs
            for (unsigned i = 0; i < dimensions; i++)
                subgraphs[&graph][node][degree].second.second.push_back(unidist(dev));
        }
        if (revSubgraphs.count(tempID) == 0)
        {
            revSubgraphs[tempID] = 
                std::pair<const Graph *, std::pair<const Graph::Vertex *, unsigned>>();
            revSubgraphs[tempID].first = &graph;
            revSubgraphs[tempID].second.first = node;
            revSubgraphs[tempID].second.second = degree;
        }
    }
    return subgraphString;
}

void radialSkipGram(RadialContext & context, SubgraphsMap & subgraphs, 
                    const std::vector<Graph> & graphs, unsigned degree)
{
    for (unsigned i = 0; i < graphs.size(); i++)
    {
        for (unsigned j = 0; j < graphs[i].getMaxVertex(); j++)
        {
            if (graphs[i].getVertex(j) != nullptr)
            {
                for (unsigned d = 0; d <= degree; d++)
                {
                    unsigned subgraphID = 
                            subgraphs[&graphs[i]][graphs[i].getVertex(j)][d].first;
                    radialSkipGramCore(context, subgraphs, subgraphID, graphs[i], 
                                       graphs[i].getVertex(j), d, degree);
                                        
                }
            }
        }
    }
}

void radialSkipGramCore(RadialContext & context, SubgraphsMap & subgraphs, 
                        unsigned subgraphID, const Graph & graph, 
                        const Graph::Vertex * node, unsigned d, unsigned degree)
{
    for (unsigned i = 0; i < graph.getMaxVertex(); i++)
    {
        if (node->getNumber() != graph.getVertex(i)->getNumber() &&
            graph.getEdge(node->getNumber(), graph.getVertex(i)->getNumber()) != nullptr)
        {
            for (unsigned delta = ((long long) d - 1 > 0 ? d - 1 : 0); 
                 delta <= ((long long) d + 1 < degree ? d + 1 : degree); delta++)
            {
                unsigned tempID = subgraphs[&graph][graph.getVertex(i)][delta].first;
                if (context.count(subgraphID) == 0)
                    context[subgraphID] = std::multiset<unsigned>();
                context[subgraphID].insert(tempID);
            }
        }
    }
}

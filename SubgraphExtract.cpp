#include <random>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <utility>
#include <iostream>
#include <filesystem>
#include <jsoncpp/json/json.h>
#include "Graph.hpp"
#include "SubgraphMaps.hpp"
#include "SubgraphExtract.hpp"

// Extract rooted subgraphs information
void getWLSubgraph(Json::Value & JSONmap, const Graph & graph, const Graph::Vertex * node, unsigned graphNumber, unsigned nodeNumber, unsigned degree, unsigned dimensions)
{
    static unsigned subgraphID = 0;
    unsigned tempID = subgraphID;
    //std::string subgraphString("");
    bool alreadyInMap = false;
    // If this subgraph is already in map of subgraphs, return it
    if (! JSONmap["graphID"].empty() && JSONmap["graphID"].asUInt() == graphNumber && ! JSONmap["rootVertices"].empty())
    {
        for (unsigned i = 0; i < JSONmap["rootVertices"].size(); i++)
        {
            if (! JSONmap["rootVertices"][i].empty() && JSONmap["rootVertices"][i]["vertexNumber"].asUInt() == nodeNumber && ! JSONmap["rootVertices"][i]["degrees"].empty())
            {
                for (unsigned j = 0; j < JSONmap["rootVertices"][i]["degrees"].size(); j++)
                {
                    if (! JSONmap["rootVertices"][i]["degrees"][j].empty() && JSONmap["rootVertices"][i]["degrees"][j]["degree"].asUInt() == degree)
                    {
                        alreadyInMap = true;
                    }
                }
            }
        }
    }
    if (! alreadyInMap)
    {
        if (degree == 0)
            ;
        else
        {
            std::set<const Graph::Vertex *> adjacentNodes;
            for (unsigned i = 0; i < graph.getMaxVertex(); i++)
            {
                if (graph.getEdge(node->getNumber(), i) != nullptr)
                {
                    adjacentNodes.insert(graph.getVertex(i));
                }
            }
            for (std::set<const Graph::Vertex *>::iterator i = adjacentNodes.cbegin(); i != adjacentNodes.cend(); i++)
            {
                getWLSubgraph(JSONmap, graph, *i, graphNumber, (*i)->getNumber(), degree - 1, dimensions);
            }
            getWLSubgraph(JSONmap, graph, node, graphNumber, nodeNumber, degree - 1, dimensions);
        }
        // Increment unique subgraph ID
        subgraphID++;
        std::random_device dev;
        std::uniform_real_distribution<double> unidist(-1.0, 1.0);
        if (JSONmap["graphID"].empty())
        {
            std::cout << "Subgraph no " << subgraphID << "\n";
            JSONmap["graphID"] = graphNumber;
            JSONmap["rootVertices"][0]["vertexNumber"] = nodeNumber;
            JSONmap["rootVertices"][0]["degrees"][0]["degree"] = degree;
            JSONmap["rootVertices"][0]["degrees"][0]["subgraphID"] = tempID;
            // Generate random vector representations of subgraphs
            for (unsigned i = 0; i < dimensions; i++)
            {
                JSONmap["rootVertices"][0]["degrees"][0]["subgraphEmbedding"][i] = unidist(dev);
            }
        }
        else
        {
            bool nodeNumberExists = false, degreeExists = false;
            unsigned i, j;
            for (i = 0; i < JSONmap["rootVertices"].size(); i++)
            {
                if (JSONmap["rootVertices"][i]["vertexNumber"].asUInt() == nodeNumber)
                {
                    nodeNumberExists = true;
                    for (j = 0; j < JSONmap["rootVertices"][i]["degrees"].size(); j++)
                    {
                        if (JSONmap["rootVertices"][i]["degrees"][j]["degree"].asUInt() == degree)
                        {
                            degreeExists = true;
                            break;
                        }
                    }
                }
                if (nodeNumberExists)
                {
                    break;
                }
            }
            if (! nodeNumberExists)
            {
                std::cout << "Subgraph no " << subgraphID << "\n";
                JSONmap["rootVertices"][i]["vertexNumber"] = nodeNumber;
                JSONmap["rootVertices"][i]["degrees"][0]["degree"] = degree;
                JSONmap["rootVertices"][i]["degrees"][0]["subgraphID"] = tempID;
                // Generate random vector representations of subgraphs
                for (unsigned k = 0; k < dimensions; k++)
                {
                    JSONmap["rootVertices"][i]["degrees"][0]["subgraphEmbedding"][k] = unidist(dev);
                }
            }
            else if (nodeNumberExists && ! degreeExists)
            {
                std::cout << "Subgraph no " << subgraphID << "\n";
                JSONmap["rootVertices"][i]["degrees"][j]["degree"] = degree;
                JSONmap["rootVertices"][i]["degrees"][j]["subgraphID"] = tempID;
                // Generate random vector representations of subgraphs
                for (unsigned k = 0; k < dimensions; k++)
                {
                    JSONmap["rootVertices"][i]["degrees"][j]["subgraphEmbedding"][k] = unidist(dev);
                }
            }
        }
    }
}

void radialSkipGram(RadialContext & context, const std::vector<std::string> & subgraphs, const std::vector<Graph> & graphs, unsigned degree)
{
    for (unsigned i = 0; i < graphs.size(); i++)
    {
        std::ifstream subgraphsMap(subgraphs[i]);
        Json::Value JSONmap;
        subgraphsMap >> JSONmap;
        for (unsigned j = 0; j < graphs[i].getMaxVertex(); j++)
        {
            if (graphs[i].getVertex(j) != nullptr)
            {
                for (unsigned d = 0; d <= degree; d++)
                {
                    unsigned subgraphID = JSONmap["rootVertices"][j]["degrees"][d]["subgraphID"].asUInt();
                    radialSkipGramCore(context, JSONmap, subgraphID, graphs[i], graphs[i].getVertex(j), d, degree);
                                        
                }
            }
        }
        JSONmap.clear();
        subgraphsMap.close();
    }
}

void radialSkipGramCore(RadialContext & context, Json::Value & JSONmap, unsigned subgraphID, const Graph & graph, const Graph::Vertex * node, unsigned d, unsigned degree)
{
    bool hasAdjacentVertices = false;
    for (unsigned i = 0; i < graph.getMaxVertex(); i++)
    {
        if (node->getNumber() != graph.getVertex(i)->getNumber() && graph.getEdge(node->getNumber(), graph.getVertex(i)->getNumber()) != nullptr)
        {
            hasAdjacentVertices = true;
            for (unsigned delta = ((long long) d - 1 > 0 ? d - 1 : 0); delta <= ((long long) d + 1 < degree ? d + 1 : degree); delta++)
            {
                unsigned tempID = JSONmap["rootVertices"][i]["degrees"][delta]["subgraphID"].asUInt();
                if (context.count(subgraphID) == 0)
                    context[subgraphID] = std::multiset<unsigned>();
                context[subgraphID].insert(tempID);
            }
        }
    }
    // If particular vertex in particular graph doesn't have adjacent vertices, generate its context vertex randomly
    if (! hasAdjacentVertices)
    {
        std::random_device dev;
        std::uniform_int_distribution<unsigned> unidist(0, graph.getMaxVertex() - 1);
        unsigned temp;
        do
            temp = unidist(dev);
        while (graph.getVertex(temp) == nullptr);
        for (unsigned delta = ((long long) d - 1 > 0 ? d - 1 : 0); delta <= ((long long) d + 1 < degree ? d + 1 : degree); delta++)
        {
            unsigned tempID = JSONmap["rootVertices"][temp]["degrees"][delta]["subgraphID"].asUInt();
            if (context.count(subgraphID) == 0)
                context[subgraphID] = std::multiset<unsigned>();
            context[subgraphID].insert(tempID);
        }
    }
}

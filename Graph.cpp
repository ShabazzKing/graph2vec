#include <vector>
#include <iostream>
#include "Graph.hpp"

Graph::Vertex::Vertex(unsigned n, unsigned l) : number(n), label(l) {}

unsigned Graph::Vertex::getNumber() const
{
    return number;
}

unsigned Graph::Vertex::getLabel() const
{
    return label;
}

Graph::Edge::Edge(const Vertex * v1, const Vertex * v2) : first(v1), second(v2) {}

const Graph::Vertex * Graph::Edge::getFirstVertex() const
{
    return first;
}

const Graph::Vertex * Graph::Edge::getSecondVertex() const
{
    return second;
}

Graph::Graph() : numberOfVertices(0), numberOfEdges(0) {}

Graph::Graph(const Graph & g) : numberOfVertices(g.numberOfVertices), 
                                numberOfEdges(g.numberOfEdges)
{
    for (unsigned i = 0; i < g.vertices.size(); i++)
    {
        unsigned number, label;
        number = g.vertices[i]->getNumber();
        label = g.vertices[i]->getLabel();
        vertices.push_back(new Vertex(number, label));
    }
    for (unsigned i = 0; i < g.adjacencyMatrix.size(); i++)
    {
        adjacencyMatrix.push_back(std::vector<Edge *>());
        for (unsigned j = 0; j < g.adjacencyMatrix[0].size(); j++)
        {
            const Vertex * first = g.adjacencyMatrix[i][j]->getFirstVertex();
            const Vertex * second = g.adjacencyMatrix[i][j]->getSecondVertex();
            adjacencyMatrix[i].push_back(new Edge(first, second));
        }
    }
}

Graph::~Graph()
{
    for (unsigned i = 0; i < adjacencyMatrix.size(); i++)
        for (unsigned j = 0; j < adjacencyMatrix[i].size(); j++)
            delete adjacencyMatrix[i][j];
    for (unsigned i = 0; i < vertices.size(); i++)
        delete vertices[i];
}

Graph & Graph::operator=(const Graph & g)
{
    if (this == &g)
        return *this;
    for (unsigned i = 0; i < adjacencyMatrix.size(); i++)
    {
        for (unsigned j = 0; j < adjacencyMatrix[i].size(); j++)
            delete adjacencyMatrix[i][j];
        adjacencyMatrix[i].clear();
    }
    adjacencyMatrix.clear();
    for (unsigned i = 0; i < vertices.size(); i++)
        delete vertices[i];
    vertices.clear();
    numberOfVertices = g.numberOfVertices;
    numberOfEdges = g.numberOfEdges;
    for (unsigned i = 0; i < g.vertices.size(); i++)
    {
        unsigned number, label;
        number = g.vertices[i]->getNumber();
        label = g.vertices[i]->getLabel();
        vertices.push_back(new Vertex(number, label));
    }
    for (unsigned i = 0; i < g.adjacencyMatrix.size(); i++)
    {
        adjacencyMatrix.push_back(std::vector<Edge *>());
        for (unsigned j = 0; j < g.adjacencyMatrix[0].size(); j++)
        {
            const Vertex * first = g.adjacencyMatrix[i][j]->getFirstVertex();
            const Vertex * second = g.adjacencyMatrix[i][j]->getSecondVertex();
            adjacencyMatrix[i].push_back(new Edge(first, second));
        }
    }
    return *this;
}

unsigned Graph::getNumberOfVertices() const
{
    return numberOfVertices;
}

unsigned Graph::getNumberOfEdges() const
{
    return numberOfEdges;
}

unsigned Graph::getMaxVertex() const
{
    return vertices.size();
}

void Graph::addVertex(unsigned n, unsigned l)
{
    if (n >= vertices.size())
    {
        for (unsigned i = vertices.size(); i < n; i++)
            vertices.push_back(nullptr);
        vertices.push_back(new Vertex(n, l));
        numberOfVertices++;
        for (unsigned i = 0; i < adjacencyMatrix.size(); i++)
            for (unsigned j = adjacencyMatrix[i].size(); j < vertices.size(); j++)
                adjacencyMatrix[i].push_back(nullptr);
        for (unsigned i = adjacencyMatrix.size(); i < vertices.size(); i++)
        {
            adjacencyMatrix.push_back(std::vector<Edge *>());
            for (unsigned j = 0; j < vertices.size(); j++)
                adjacencyMatrix[i].push_back(nullptr);
        }
    }
    else
    {
        if (vertices[n] != nullptr)
        {
            std::cerr << "Vertex of number " << n << " already exists.\n";
            return;
        }
        vertices[n] = new Vertex(n, l);
        numberOfVertices++;
    }
}

void Graph::addEdge(unsigned v1Number, unsigned v2Number)
{
    if (v1Number >= vertices.size() || v2Number >= vertices.size() || 
        vertices[v1Number] == nullptr || vertices[v2Number] == nullptr)
    {
        std::cerr << "Edge " << v1Number << " " << v2Number << " cannot be added, "; 
        std::cerr << "one of vertices doesn't exist.\n";
        return;
    }
    if (adjacencyMatrix[v1Number][v2Number] != nullptr)
    {
        std::cerr << "Edge (" << v1Number << " " << v2Number << ") already exists.\n";
        return;
    }
    adjacencyMatrix[v1Number][v2Number] = new Edge(getVertex(v1Number), 
                                                   getVertex(v2Number));
    numberOfEdges++;
}

void Graph::removeVertex(unsigned n)
{
    if (n >= vertices.size() || vertices[n] == nullptr)
    {
        std::cerr << "Vertex of number " << n << " doesn't exist.\n";
        return;
    }
    for (unsigned i = 0; i < vertices.size(); i++)
    {
        if (adjacencyMatrix[i][n] != nullptr)
        {
            delete adjacencyMatrix[i][n];
            adjacencyMatrix[i][n] = nullptr;
            numberOfEdges--;
        }
        if (adjacencyMatrix[n][i] != nullptr)
        {
            delete adjacencyMatrix[n][i];
            adjacencyMatrix[n][i] = nullptr;
            numberOfEdges--;
        }
    }
    delete vertices[n];
    vertices[n] = nullptr;
    numberOfVertices--;
    if (n == getMaxVertex() - 1)
    {
        unsigned i = n;
        while (vertices[i] == nullptr)
        {
            vertices.pop_back();
            i--;
        }
        for (unsigned j = n; j > i; j--)
            adjacencyMatrix.pop_back();
        for (unsigned j = n; j > i; j--)
            for (unsigned k = 0; k < adjacencyMatrix.size(); k++)
                adjacencyMatrix[k].pop_back();
    }
}

void Graph::removeEdge(unsigned v1Number, unsigned v2Number)
{
    if (v1Number >= vertices.size() || v2Number >= vertices.size() || 
        adjacencyMatrix[v1Number][v2Number] == nullptr)
    {
        std::cerr << "Edge (" << v1Number << " " << v2Number << ") doesn't exist.\n";
        return;
    }
    delete adjacencyMatrix[v1Number][v2Number];
    adjacencyMatrix[v1Number][v2Number] = nullptr;
    numberOfEdges--;
}

const Graph::Vertex * Graph::getVertex(unsigned n) const
{
    if (n >= vertices.size())
    {
        std::cerr << "Vertex number out of range.\n";
        return nullptr;
    }
    return vertices[n];
}

const Graph::Edge * Graph::getEdge(unsigned v1Number, unsigned v2Number) const
{
    if (v1Number >= vertices.size() || v2Number >= vertices.size())
    {
        std::cerr << "Vertex number out of range.\n";
        return nullptr;
    }
    return adjacencyMatrix[v1Number][v2Number];
}

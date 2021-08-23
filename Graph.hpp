#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>

class Graph
{
public:
    class Vertex
    {
    private:
        const unsigned number;
        const unsigned label;
    public:
        Vertex(unsigned, unsigned);
        unsigned getNumber() const;
        unsigned getLabel() const;
    };
    class Edge
    {
    private:
        const Vertex * first;
        const Vertex * second;
    public:
        Edge(const Vertex *, const Vertex *);
        const Vertex * getFirstVertex() const;
        const Vertex * getSecondVertex() const;
    };
private:
    std::vector<Vertex *> vertices;
    std::vector<std::vector<Edge *>> adjacencyMatrix;
    unsigned numberOfVertices;
    unsigned numberOfEdges;
public:
    Graph();
    Graph(const Graph &);
    Graph(Graph &&);
    ~Graph();
    Graph & operator=(const Graph &);
    unsigned getNumberOfVertices() const;
    unsigned getNumberOfEdges() const;
    unsigned getMaxVertex() const;
    void addVertex(unsigned, unsigned);
    void addEdge(unsigned, unsigned);
    void removeVertex(unsigned);
    void removeEdge(unsigned, unsigned);
    const Vertex * getVertex(unsigned) const;
    const Edge * getEdge(unsigned, unsigned) const;
};

#endif

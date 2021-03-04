#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <set>
#include <utility>
#include <string>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <json/json.h>
#include "Graph.hpp"
#include "word2vec.hpp"
#include "SubgraphMaps.hpp"
#include "SubgraphExtract.hpp"

int argPos(const char *, int, char **);

void readGraphs(std::filesystem::directory_entry &, std::vector<Graph> &);

std::vector<unsigned> getRandomIndexes(unsigned);

std::vector<std::vector<long double>> negativeSampling(unsigned, unsigned, 
                                                       const std::vector<Graph> &, 
                                                       unsigned, SubgraphsMap &);

void updateGraphsEmbeddings(std::vector<std::vector<long double>> &, unsigned, 
                            const std::vector<long double> &, 
                            const std::vector<std::vector<long double>> &, double);

int main(int argc, char ** argv)
{
    if ((argc == 2 && std::strcmp(argv[1], "--help") == 0) || argc == 1)
    {
        std::cout << "Usage:\ngraph2vec --dataset <JSON graph files directory>\n";
        std::cout << "\t--output <graphs embeddings file>\n";
        std::cout << "\t--deg <maximum degree of rooted subgraphs> (default: 10)\n";
        std::cout << "\t--dim <number of dimensions of embedding vectors> (default: 10)\n";
        std::cout << "\t--ep <number of epochs> (default: 3)\n";
        std::cout << "\t--alpha <learning rate> (default: 0.025)\n";
        std::cout << "\t--neg <number of negative samples> (default: 20)\n";
        return 0;
    }
    std::filesystem::path inputDirName, inputFileName, outputFileName;
    std::filesystem::directory_entry inputDir;
    unsigned degree, dimensions, epochs, negSamples;
    double alpha;
    int pos = argPos("--dataset", argc, argv);
    if (pos == argc)
    {
        std::cerr << "Lack of input data.\n";
        return EXIT_FAILURE;
    }
    inputDirName = std::filesystem::path(argv[pos + 1]);
    pos = argPos("--output", argc, argv);
    if (pos == argc)
    {
        std::cerr << "Lack of output file.\n";
        return EXIT_FAILURE;
    }
    outputFileName = std::filesystem::path(argv[pos + 1]);
    pos = argPos("--deg", argc, argv);
    if (pos == argc)
        degree = 10;
    else
        degree = (unsigned) std::atoi(argv[pos + 1]);
    pos = argPos("--dim", argc, argv);
    if (pos == argc)
        dimensions = 10;
    else
        dimensions = (unsigned) std::atoi(argv[pos + 1]);
    pos = argPos("--ep", argc, argv);
    if (pos == argc)
        epochs = 3;
    else
        epochs = (unsigned) std::atoi(argv[pos + 1]);
    pos = argPos("--alpha", argc, argv);
    if (pos == argc)
        alpha = 0.025;
    else
        alpha = std::atof(argv[pos + 1]);
    pos = argPos("--neg", argc, argv);
    if (pos == argc)
        negSamples = 20;
    else
    {
        negSamples = (unsigned) std::atoi(argv[pos + 1]);
        if (negSamples <= 1)
        {
            std::cerr << "Too few negative samples (at least 2).\n";
            return EXIT_FAILURE;
        }
    }
    inputDir = std::filesystem::directory_entry(inputDirName);
    if (! inputDir.exists())
    {
        std::cerr << "Input directory doesn't exist.\n";
        return EXIT_FAILURE;
    }
    if (inputDir.exists() && ! inputDir.is_directory())
    {
        std::cerr << "Input source is not a directory.\n";
        return EXIT_FAILURE;
    }
    std::vector<Graph> graphsVector; // Vector of the graphs to be embedded
    readGraphs(inputDir, graphsVector);
    std::vector<std::vector<long double>> graphsEmbeddings; // Matrix of embeddings
    std::random_device dev;
    std::uniform_real_distribution<long double> unidist(-1.0L, 1.0L);
    // Initialization of embeddings matrix by random real values
    for (unsigned i = 0; i < graphsVector.size(); i++)
    {
        graphsEmbeddings.push_back(std::vector<long double>());
        for (unsigned j = 0; j < dimensions; j++)
            graphsEmbeddings[i].push_back(unidist(dev));
    }
    SubgraphsMap subgraphs; // Look to the SubgraphMaps.hpp
    ReverseSubgraphsMap revSubgraphs; // Look to the SubgraphMaps.hpp
    // Now we call function, which extracts rooted subgraphs, assigns to them string and ID
    for (unsigned i = 0; i < graphsVector.size(); i++)
        for (unsigned j = 0; j < graphsVector[i].getMaxVertex(); j++)
            if (graphsVector[i].getVertex(j) != nullptr)
                for (unsigned k = 0; k <= degree; k++)
                    getWLSubgraph(subgraphs, revSubgraphs, graphsVector[i], 
                                  graphsVector[i].getVertex(j), k, dimensions);
    RadialContext subgraphContext; // Look to the SubgraphMaps.hpp
    // Now radial context of every rooted subgraph is being set, like in
    // subgraph2vec algorithm
    radialSkipGram(subgraphContext, subgraphs, graphsVector, degree);
    // Now we call word2vec algorithm in order to make vector representations of
    // rooted subgraphs
    for (unsigned i = 0; i < graphsVector.size(); i++)
    {
        std::cout << "word2vec for subgraphs of Graph no " << i << std::endl;
        // Here the minimal subgraph ID for every graph in dataset is chosen. We want that
        // because subgraph IDs are unique for every subgraph in the vocabulary
        // (for all graphs in dataset), but in word2vec we need to assign word IDs from 0
        unsigned minID = subgraphs[&graphsVector[i]]
                            [graphsVector[i].getVertex(0)][0].first;
        for (unsigned j = 0; j < graphsVector[i].getMaxVertex(); j++)
            for (unsigned k = 0; k <= degree; k++)
                if (subgraphs[&graphsVector[i]][graphsVector[i].getVertex(j)][k].first 
                    < minID)
                    minID = subgraphs[&graphsVector[i]]
                            [graphsVector[i].getVertex(j)][k].first;
        word2vec(subgraphs[&graphsVector[i]], revSubgraphs, subgraphContext, 
                 graphsVector[i], degree, dimensions, epochs, alpha, minID);
    }
    // Main loop of the algorithm
    for (unsigned e = 0; e < epochs; e++)
    {
        std::cout << "Epoch number " << e << std::endl;
        // Shuffle dataset graphs
        std::vector<unsigned> indexes = getRandomIndexes(graphsVector.size());
        for (unsigned i = 0; i < graphsVector.size(); i++)
        {
            // Choosing negative samples for negative skipgram
            std::vector<std::vector<long double>> negSamplesVector = 
                        negativeSampling(negSamples, indexes[i], graphsVector, 
                                         degree, subgraphs);
            for (unsigned j = 0; j < graphsVector[indexes[i]].getMaxVertex(); j++)
            {
                if (graphsVector[indexes[i]].getVertex(j) != nullptr)
                    for (unsigned k = 0; k <= degree; k++)
                    {
                        // Training graph embeddings
                        std::vector<long double> temp = 
                            subgraphs[&graphsVector[indexes[i]]]
                            [graphsVector[indexes[i]].getVertex(j)][k].second.second;
                        updateGraphsEmbeddings(graphsEmbeddings, indexes[i], temp, 
                                               negSamplesVector, alpha);
                    }
            }
        }
    }
    std::filesystem::directory_entry outputDir(outputFileName.parent_path());
    if (! outputDir.exists())
        std::filesystem::create_directories(outputFileName.parent_path());
    std::ofstream outputFile(outputFileName);
    // Writing embeddings to the file
    for (unsigned i = 0; i < graphsVector.size(); i++)
    {
        outputFile << "Graph no " << i << std::endl;
        for (unsigned j = 0; j < dimensions; j++)
            outputFile << "\tx_" << j + 1 << ": " << graphsEmbeddings[i][j] << std::endl;
    }
    outputFile.close();
    return 0;
}

int argPos(const char * s, int argc, char ** argv)
{
    int pos;
    for (pos = 1; pos < argc; pos += 2)
    {
        if (pos == argc - 1)
        {
            std::cerr << "Invalid arguments.\n";
            std::exit(EXIT_FAILURE);
        }
        if (std::strcmp(argv[pos], s) == 0)
            return pos;
    }
    return pos;
}

void readGraphs(std::filesystem::directory_entry & dir, std::vector<Graph> & graphs)
{
    std::vector<unsigned> ft;
    std::set<std::pair<unsigned, unsigned>> edgesSet;
    std::filesystem::directory_iterator dir_it(dir.path());
    std::filesystem::path inputFileName;
    std::ifstream inputFile;
    unsigned graphNumber;
    Json::Value sourceJSON;
    for (dir_it = begin(dir_it); dir_it != end(dir_it); dir_it++)
    {
        inputFileName = dir_it->path();
        inputFile.open(inputFileName);
        inputFile >> sourceJSON;
        inputFile.close();
        for (unsigned i = 0; i < sourceJSON["features"].size(); i++)
            ft.push_back(std::stoi(sourceJSON["features"][std::to_string(i)].asString()));
        for (unsigned i = 0; i < sourceJSON["edges"].size(); i++)
        {
            std::pair<unsigned, unsigned> temp;
            temp.first = sourceJSON["edges"][i][0].asUInt();
            temp.second = sourceJSON["edges"][i][1].asUInt();
            edgesSet.insert(temp);
        }
        sourceJSON.clear();
        graphNumber = (unsigned) std::stoi(inputFileName.stem().string());
        if (graphNumber >= graphs.size())
        {
            graphs.resize(graphNumber);
            Graph * g = new Graph;
            graphs.push_back(*g);
            delete g;
        }
        for (unsigned i = 0; i < ft.size(); i++)
            graphs[graphNumber].addVertex(i, ft[i]);
        for (std::set<std::pair<unsigned, unsigned>>::iterator i = edgesSet.cbegin(); 
             i != edgesSet.cend(); i++)
            graphs[graphNumber].addEdge((*i).first, (*i).second);
        ft.clear();
        edgesSet.clear();
    }
}

std::vector<unsigned> getRandomIndexes(unsigned size)
{
    std::random_device dev;
    std::uniform_int_distribution<unsigned> unidist(0, size - 1);
    std::set<unsigned> indexes_used;
    std::vector<unsigned> indexes;
    for (unsigned i = 0; i < size; i++)
    {
        unsigned temp;
        do
            temp = unidist(dev);
        while (indexes_used.count(temp) == 1);
        indexes_used.insert(temp);
        indexes.push_back(temp);
    }
    return indexes;
}

std::vector<std::vector<long double>> negativeSampling(unsigned samples, 
                                                       unsigned graphIndex, 
                                                       const std::vector<Graph> & graphs, 
                                                       unsigned degree, 
                                                       SubgraphsMap & subgraphs)
{
    std::random_device dev;
    std::uniform_int_distribution<unsigned> unidist1(0, graphs.size() - 1);
    std::uniform_int_distribution<unsigned> unidist3(0, degree);
    std::vector<std::vector<long double>> result;
    std::set<unsigned> subgraphs_used;
    for (unsigned i = 0; i < samples; i++)
    {
        unsigned tempGraph, tempVertex, tempDegree;
        do
        {
            do
                tempGraph = unidist1(dev);
            while (tempGraph == graphIndex);
            std::uniform_int_distribution<unsigned> unidist2(0, 
                                                graphs[tempGraph].getMaxVertex() - 1);
            tempVertex = unidist2(dev);
            tempDegree = unidist3(dev);
        }
        while (subgraphs_used.count(subgraphs[&graphs[tempGraph]]
            [graphs[tempGraph].getVertex(tempVertex)][tempDegree].first) == 1);
        subgraphs_used.insert(subgraphs[&graphs[tempGraph]]
                    [graphs[tempGraph].getVertex(tempVertex)][tempDegree].first);
        result.push_back(subgraphs[&graphs[tempGraph]]
                    [graphs[tempGraph].getVertex(tempVertex)][tempDegree].second.second);
    }
    return result;
}

void updateGraphsEmbeddings(std::vector<std::vector<long double>> & embeddings, 
                            unsigned graphIndex, 
                            const std::vector<long double> & subgraph, 
                            const std::vector<std::vector<long double>> & negSamples, 
                            double alpha)
{
    // Here we calculate scalar by matrix (graph embeddings) derivative, as described
    // in graph2vec paper
    std::vector<long double> sums1;
    for (unsigned i = 0; i < negSamples.size(); i++)
        sums1.push_back(0.0L);
    for (unsigned i = 0; i < negSamples.size(); i++)
        for (unsigned j = 0; j < embeddings[0].size(); j++)
            sums1[i] += embeddings[graphIndex][j] * negSamples[i][j];
    long double maxSum = sums1[0];
    for (unsigned i = 1; i < negSamples.size(); i++)
        if (maxSum < sums1[i])
            maxSum = sums1[i];
    for (unsigned i = 0; i < embeddings[0].size(); i++)
    {
        long double sum2 = 0.0L, sum3 = 0.0L;
        for (unsigned j = 0; j < negSamples.size(); j++)
        {
            long double ex;
            if (sums1[j] - maxSum < -7.0L)
                ex = 0.0L;
            else
                ex = std::exp(sums1[j] - maxSum);
            sum2 += ex;
            sum3 += ex * negSamples[j][i];
        }
        embeddings[graphIndex][i] -= alpha * (sum3 / sum2 - subgraph[i]);
    }
}

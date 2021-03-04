#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <utility>
#include <set>
#include "word2vec.hpp"
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

std::vector<std::vector<long double>> forwardPropagation(const std::vector<unsigned> &, 
                    const std::vector<std::pair<std::vector<long double>, unsigned>> &, 
                    const std::vector<std::vector<long double>> &, 
                    std::vector<std::vector<long double>> &, 
                    std::vector<std::vector<long double>> &);

void transpose(std::vector<std::vector<long double>> &);

std::vector<std::vector<long double>> matMul(
    const std::vector<std::vector<long double>> &, 
    const std::vector<std::vector<long double>> &);

std::vector<std::vector<long double>> softmax(
    const std::vector<std::vector<long double>> &);

void backwardPropagation(std::vector<std::vector<long double>> &, 
                         std::vector<std::vector<long double>> &, 
                         std::vector<std::vector<long double>> &, 
                         const std::vector<unsigned> &, 
                         const std::vector<std::vector<long double>> &, 
                         const std::vector<std::vector<long double>> &, 
                         const std::vector<std::vector<long double>> &);

void word2vec(SubgrSubMap & subgraphs, ReverseSubgraphsMap & revSubgraphs, 
              RadialContext & context, const Graph & graph, 
              unsigned degree, unsigned dimensions, unsigned epochs, double alpha, 
              unsigned minID)
{
    std::vector<unsigned> X, Y;
    std::vector<std::pair<std::vector<long double>, unsigned>> wordEmbeddings;
    for (unsigned i = 0; i < graph.getMaxVertex(); i++)
    {
        if (graph.getVertex(i) != nullptr)
        {
            for (unsigned j = 0; j <= degree; j++)
            {
                unsigned wordID = 
                subgraphs[graph.getVertex(i)][j].first;
                for (std::multiset<unsigned>::iterator it = context[wordID].cbegin();
                     it != context[wordID].cend(); it++)
                {
                    X.push_back(wordID - minID);
                    Y.push_back(*it - minID);
                }
                std::pair<std::vector<long double>, unsigned> temp;
                temp.first = 
                    subgraphs[graph.getVertex(i)][j].second.second;
                temp.second = wordID;
                wordEmbeddings.push_back(temp);
            }
        }
    }
    std::random_device dev;
    std::uniform_real_distribution<long double> unidist(-1.0L, 1.0L);
    std::vector<std::vector<long double>> denseLayerMatrix;
    for (unsigned i = 0; i < wordEmbeddings.size(); i++)
    {
        denseLayerMatrix.push_back(std::vector<long double>());
        for (unsigned j = 0; j < dimensions; j++)
            denseLayerMatrix[i].push_back(unidist(dev));
    }
    for (unsigned e = 0; e < epochs; e++)
    {
        std::cout << "\tword2vec: epoch number " << e << std::endl;
        std::vector<std::vector<long double>> wordVector, softmaxOutput, Z;
        softmaxOutput = forwardPropagation(X, wordEmbeddings, denseLayerMatrix, 
                                           wordVector, Z);
        std::vector<std::vector<long double>> dL_dZ, dL_dDenseLayerMatrix, dL_dWordVector; 
        backwardPropagation(dL_dZ, dL_dDenseLayerMatrix, dL_dWordVector, Y, softmaxOutput, 
                            denseLayerMatrix, wordVector);
        transpose(dL_dWordVector);
        for (unsigned i = 0; i < X.size(); i++)
            for (unsigned j = 0; j < wordEmbeddings[0].first.size(); j++)
                wordEmbeddings[X[i]].first[j] -= alpha * dL_dWordVector[i][j];
        for (unsigned i = 0; i < denseLayerMatrix.size(); i++)
            for (unsigned j = 0; j < denseLayerMatrix[0].size(); j++)
                denseLayerMatrix[i][j] -= alpha * dL_dDenseLayerMatrix[i][j];
    }
    for (unsigned i = 0; i < wordEmbeddings.size(); i++)
    {
        unsigned wordID = wordEmbeddings[i].second;
        const Graph::Vertex * v = revSubgraphs[wordID].second.first;
        unsigned d = revSubgraphs[wordID].second.second;
        subgraphs[v][d].second.second = wordEmbeddings[i].first;
    }
}

std::vector<std::vector<long double>> forwardPropagation(const std::vector<unsigned> & X, 
        const std::vector<std::pair<std::vector<long double>, unsigned>> & wordEmbeddings, 
        const std::vector<std::vector<long double>> & denseLayerMatrix, 
        std::vector<std::vector<long double>> & wordVector, 
        std::vector<std::vector<long double>> & Z)
{
    for (unsigned i = 0; i < X.size(); i++)
        wordVector.push_back(wordEmbeddings[X[i]].first);
    transpose(wordVector);
    Z = matMul(denseLayerMatrix, wordVector);
    return softmax(Z);
}

void transpose(std::vector<std::vector<long double>> & v)
{
    std::vector<std::vector<long double>> result;
    for (unsigned i = 0; i < v[0].size(); i++)
    {
        result.push_back(std::vector<long double>());
        for (unsigned j = 0; j < v.size(); j++)
            result[i].push_back(v[j][i]);
    }
    v = result;
}

std::vector<std::vector<long double>> matMul(
    const std::vector<std::vector<long double>> & m1, 
    const std::vector<std::vector<long double>> & m2)
{
    if (m1[0].size() != m2.size())
        std::exit(EXIT_FAILURE);
    std::vector<std::vector<long double>> result;
    for (unsigned i = 0; i < m1.size(); i++)
    {
        result.push_back(std::vector<long double>());
        for (unsigned j = 0; j < m2[0].size(); j++)
        {
            long double temp = 0.0L;
            for (unsigned k = 0; k < m2.size(); k++)
                temp += m1[i][k] * m2[k][j];
            result[i].push_back(temp);
        }
    }
    return result;
}

std::vector<std::vector<long double>> softmax(
    const std::vector<std::vector<long double>> & v)
{
    std::vector<std::vector<long double>> result;
    for (unsigned i = 0; i < v.size(); i++)
    {
        result.push_back(std::vector<long double>());
        for (unsigned j = 0; j < v[0].size(); j++)
            result[i].push_back(0.0L);
    }
    for (unsigned i = 0; i < v[0].size(); i++)
    {
        long double param = v[0][i];
        for (unsigned j = 1; j < v.size(); j++)
            if (param < v[j][i])
                param = v[j][i];
        long double sum = 0.0L;
        for (unsigned j = 0; j < v.size(); j++)
        {
            if (v[j][i] - param < -7.0L)
                continue;
            else
                sum += std::exp(v[j][i] - param);
        }
        for (unsigned j = 0; j < v.size(); j++)
        {
            if (v[j][i] - param < -7.0L)
                result[j][i] = 0.0L;
            else
                result[j][i] = std::exp(v[j][i] - param) / sum;
        }
    }
    return result;
}

void backwardPropagation(std::vector<std::vector<long double>> & dL_dZ, 
                         std::vector<std::vector<long double>> & dL_dDenseLayerMatrix, 
                         std::vector<std::vector<long double>> & dL_dWordVector, 
                         const std::vector<unsigned> & Y, 
                         const std::vector<std::vector<long double>> & softmaxOutput, 
                         const std::vector<std::vector<long double>> & denseLayerMatrix, 
                         const std::vector<std::vector<long double>> & wordVector)
{
    for (unsigned i = 0; i < softmaxOutput.size(); i++)
    {
        dL_dZ.push_back(std::vector<long double>());
        for (unsigned j = 0; j < softmaxOutput[0].size(); j++)
            dL_dZ[i].push_back(softmaxOutput[i][j] - Y[j]);
    }
    std::vector<std::vector<long double>> tempWordVector = wordVector;
    transpose(tempWordVector);
    dL_dDenseLayerMatrix = matMul(dL_dZ, tempWordVector);
    for (unsigned i = 0; i < dL_dDenseLayerMatrix.size(); i++)
        for (unsigned j = 0; j < dL_dDenseLayerMatrix[0].size(); j++)
            dL_dDenseLayerMatrix[i][j] *= 1.0L / tempWordVector.size();
    std::vector<std::vector<long double>> tempDenseLayerMatrix = denseLayerMatrix;
    transpose(tempDenseLayerMatrix);
    dL_dWordVector = matMul(tempDenseLayerMatrix, dL_dZ);
}

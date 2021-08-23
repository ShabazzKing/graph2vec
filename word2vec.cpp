#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <utility>
#include <set>
#include <eigen3/Eigen/Eigen>
#include <jsoncpp/json/json.h>
#include "word2vec.hpp"
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

std::vector<std::vector<double>> forwardPropagation(const std::vector<unsigned> &, const std::vector<std::pair<std::vector<double>, unsigned>> &,
                                                    const std::vector<std::vector<double>> &, std::vector<std::vector<double>> &, std::vector<std::vector<double>> &);

void transpose(std::vector<std::vector<double>> &);

std::vector<std::vector<double>> matMul(const std::vector<std::vector<double>> &, const std::vector<std::vector<double>> &);

std::vector<std::vector<double>> softmax(const std::vector<std::vector<double>> &);

void backwardPropagation(std::vector<std::vector<double>> &, std::vector<std::vector<double>> &, std::vector<std::vector<double>> &, const std::vector<unsigned> &,
                         const std::vector<std::vector<double>> &, const std::vector<std::vector<double>> &, const std::vector<std::vector<double>> &);

void word2vec(Json::Value & subgraphs, RadialContext & context, const Graph & graph, unsigned degree, unsigned dimensions, unsigned epochs, double alpha, unsigned minID)
{
    std::vector<unsigned> X, Y;
    std::vector<std::pair<std::vector<double>, unsigned>> wordEmbeddings;
    for (unsigned i = 0; i < graph.getMaxVertex(); i++)
    {
        if (graph.getVertex(i) != nullptr)
        {
            for (unsigned j = 0; j <= degree; j++)
            {
                unsigned wordID = subgraphs["rootVertices"][i]["degrees"][j]["subgraphID"].asUInt();
                for (std::multiset<unsigned>::iterator it = context[wordID].cbegin(); it != context[wordID].cend(); it++)
                {
                    X.push_back(wordID - minID);
                    Y.push_back(*it - minID);
                }
                std::pair<std::vector<double>, unsigned> temp;
                for (unsigned k = 0; k < dimensions; k++)
                    temp.first.push_back(subgraphs["rootVertices"][i]["degrees"][j]["subgraphEmbedding"][k].asDouble());
                temp.second = wordID;
                wordEmbeddings.push_back(temp);
            }
        }
    }
    std::random_device dev;
    std::uniform_real_distribution<double> unidist(-1.0L, 1.0L);
    std::vector<std::vector<double>> denseLayerMatrix;
    for (unsigned i = 0; i < wordEmbeddings.size(); i++)
    {
        denseLayerMatrix.push_back(std::vector<double>());
        for (unsigned j = 0; j < dimensions; j++)
        {
            denseLayerMatrix[i].push_back(unidist(dev));
        }
    }
    for (unsigned e = 0; e < epochs; e++)
    {
        std::cout << "\tword2vec: epoch number " << e << std::endl;
        std::vector<std::vector<double>> wordVector, softmaxOutput, Z;
        softmaxOutput = forwardPropagation(X, wordEmbeddings, denseLayerMatrix, wordVector, Z);
        std::cout << "\tA\n";
        std::vector<std::vector<double>> dL_dZ, dL_dDenseLayerMatrix, dL_dWordVector; 
        backwardPropagation(dL_dZ, dL_dDenseLayerMatrix, dL_dWordVector, Y, softmaxOutput, denseLayerMatrix, wordVector);
        std::cout << "\tB\n";
        transpose(dL_dWordVector);
        std::cout << "\tC\n";
        for (unsigned i = 0; i < X.size(); i++)
        {
            for (unsigned j = 0; j < wordEmbeddings[0].first.size(); j++)
            {
                wordEmbeddings[X[i]].first[j] -= alpha * dL_dWordVector[i][j];
            }
        }
        for (unsigned i = 0; i < denseLayerMatrix.size(); i++)
        {
            for (unsigned j = 0; j < denseLayerMatrix[0].size(); j++)
            {
                denseLayerMatrix[i][j] -= alpha * dL_dDenseLayerMatrix[i][j];
            }
        }
    }
    for (unsigned i = 0; i < wordEmbeddings.size(); i++)
    {
        unsigned wordID = wordEmbeddings[i].second;
        unsigned j, k;
        bool found = false;
        for (j = 0; j < subgraphs["rootVertices"].size(); j++)
        {
            for (k = 0; k < subgraphs["rootVertices"][j]["degrees"].size(); k++)
            {
                if (subgraphs["rootVertices"][j]["degrees"][k]["subgraphID"].asUInt() == wordID)
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }
        for (unsigned l = 0; l < dimensions; l++)
        {
            subgraphs["rootVertices"][j]["degrees"][k]["subgraphEmbedding"][l] = wordEmbeddings[i].first[l];
        }
    }
}

std::vector<std::vector<double>> forwardPropagation(const std::vector<unsigned> & X, const std::vector<std::pair<std::vector<double>, unsigned>> & wordEmbeddings,
                                                    const std::vector<std::vector<double>> & denseLayerMatrix, std::vector<std::vector<double>> & wordVector,
                                                    std::vector<std::vector<double>> & Z)
{
    std::cout << "X.size():   " << X.size() << "\n";
    for (unsigned i = 0; i < X.size(); i++)
        wordVector.push_back(wordEmbeddings[X[i]].first);
    std::cout << "\tD\n";
    std::cout << wordVector.size() << "       " << wordVector[0].size() << "\n";
    transpose(wordVector);
    std::cout << "\tE\n";
    std::cout << denseLayerMatrix.size() << "       " << denseLayerMatrix[0].size() << "\n";
    std::cout << wordVector.size() << "       " << wordVector[0].size() << "\n";
    std::exit(0);
    Z = matMul(denseLayerMatrix, wordVector);
    return softmax(Z);
}

void transpose(std::vector<std::vector<double>> & v)
{
    std::vector<std::vector<double>> result;
    for (unsigned i = 0; i < v[0].size(); i++)
    {
        result.push_back(std::vector<double>());
        for (unsigned j = 0; j < v.size(); j++)
        {
            result[i].push_back(v[j][i]);
        }
    }
    v = result;
}

std::vector<std::vector<double>> matMul(const std::vector<std::vector<double>> & m1, const std::vector<std::vector<double>> & m2)
{
    if (m1[0].size() != m2.size())
        std::exit(EXIT_FAILURE);
    std::vector<std::vector<double>> result;
    for (unsigned i = 0; i < m1.size(); i++)
    {
        result.push_back(std::vector<double>());
        std::cout << "\tF\n";
        for (unsigned j = 0; j < m2[0].size(); j++)
        {
            double temp = 0.0L;
            for (unsigned k = 0; k < m2.size(); k++)
                temp += m1[i][k] * m2[k][j];
            result[i].push_back(temp);
            std::cout << "\tG\n" << j << "     " << m2[0].size() << "\n";
        }
    }
    return result;
}

std::vector<std::vector<double>> softmax(const std::vector<std::vector<double>> & v)
{
    std::vector<std::vector<double>> result;
    for (unsigned i = 0; i < v.size(); i++)
    {
        result.push_back(std::vector<double>());
        for (unsigned j = 0; j < v[0].size(); j++)
        {
            result[i].push_back(0.0L);
        }
    }
    for (unsigned i = 0; i < v[0].size(); i++)
    {
        double param = v[0][i];
        for (unsigned j = 1; j < v.size(); j++)
        {
            if (param < v[j][i])
            {
                param = v[j][i];
            }
        }
        double sum = 0.0L;
        for (unsigned j = 0; j < v.size(); j++)
        {
            if (v[j][i] - param < -7.0L)
            {
                continue;
            }
            else
            {
                sum += std::exp(v[j][i] - param);
            }
        }
        for (unsigned j = 0; j < v.size(); j++)
        {
            if (v[j][i] - param < -7.0L)
            {
                result[j][i] = 0.0L;
            }
            else
            {
                result[j][i] = std::exp(v[j][i] - param) / sum;
            }
        }
    }
    return result;
}

void backwardPropagation(std::vector<std::vector<double>> & dL_dZ, std::vector<std::vector<double>> & dL_dDenseLayerMatrix, std::vector<std::vector<double>> & dL_dWordVector,
                         const std::vector<unsigned> & Y, const std::vector<std::vector<double>> & softmaxOutput, const std::vector<std::vector<double>> & denseLayerMatrix,
                         const std::vector<std::vector<double>> & wordVector)
{
    for (unsigned i = 0; i < softmaxOutput.size(); i++)
    {
        dL_dZ.push_back(std::vector<double>());
        for (unsigned j = 0; j < softmaxOutput[0].size(); j++)
        {
            dL_dZ[i].push_back(softmaxOutput[i][j] - Y[j]);
        }
    }
    std::vector<std::vector<double>> tempWordVector = wordVector;
    transpose(tempWordVector);
    dL_dDenseLayerMatrix = matMul(dL_dZ, tempWordVector);
    for (unsigned i = 0; i < dL_dDenseLayerMatrix.size(); i++)
    {
        for (unsigned j = 0; j < dL_dDenseLayerMatrix[0].size(); j++)
        {
            dL_dDenseLayerMatrix[i][j] *= 1.0L / tempWordVector.size();
        }
    }
    std::vector<std::vector<double>> tempDenseLayerMatrix = denseLayerMatrix;
    transpose(tempDenseLayerMatrix);
    dL_dWordVector = matMul(tempDenseLayerMatrix, dL_dZ);
}

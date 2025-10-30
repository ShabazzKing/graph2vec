#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <utility>
#include <set>
#include <filesystem>
#include <json/json.h>
#include "word2vec.hpp"
#include "Graph.hpp"
#include "SubgraphMaps.hpp"

void forwardPropagation(const std::vector<unsigned> &, const std::vector<std::pair<std::vector<double>, unsigned>> &,
                        const std::vector<std::vector<double>> &, std::vector<std::vector<double>> &, std::string);

void transpose(std::vector<std::vector<double>> &);

void transpose(std::string, unsigned, unsigned);

void matMul(std::string, const std::vector<std::vector<double>> &, const std::vector<std::vector<double>> &);

void matMul(std::string, std::string, unsigned, const std::vector<std::vector<double>> &);

void matMul(std::string, const std::vector<std::vector<double>> &, std::string, unsigned);

void softmax(std::string, unsigned, unsigned);

void backwardPropagation(std::string, std::string, std::string, const std::vector<unsigned> &,
                         std::string, const std::vector<std::vector<double>> &, const std::vector<std::vector<double>> &);

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
    std::string softmaxOutput = "softmax.dat", dL_dZ = "dL_dZ.dat", dL_dDenseLayerMatrix = "dL_dDenseLayerMatrix.dat", dL_dWordVector = "dL_dWordVector.dat";
    for (unsigned e = 0; e < epochs; e++)
    {
        std::cout << "\tword2vec: epoch number " << e << std::endl;
        std::vector<std::vector<double>> wordVector;
        forwardPropagation(X, wordEmbeddings, denseLayerMatrix, wordVector, softmaxOutput);
        backwardPropagation(dL_dZ, dL_dDenseLayerMatrix, dL_dWordVector, Y, softmaxOutput, denseLayerMatrix, wordVector);
        transpose(dL_dWordVector, denseLayerMatrix[0].size(), wordVector[0].size());
        double currentValue;
        std::ifstream dL_dWordVectorFile(dL_dWordVector, std::ios::binary);
        for (unsigned i = 0; i < X.size(); i++)
        {
            for (unsigned j = 0; j < wordEmbeddings[0].first.size(); j++)
            {
                dL_dWordVectorFile.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
                wordEmbeddings[X[i]].first[j] -= alpha * currentValue;
            }
        }
        dL_dWordVectorFile.close();
        std::ifstream dL_dDenseLayerMatrixFile(dL_dDenseLayerMatrix, std::ios::binary);
        for (unsigned i = 0; i < denseLayerMatrix.size(); i++)
        {
            for (unsigned j = 0; j < denseLayerMatrix[0].size(); j++)
            {
                dL_dDenseLayerMatrixFile.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
                denseLayerMatrix[i][j] -= alpha * currentValue;
            }
        }
        dL_dDenseLayerMatrixFile.close();
    }
    std::filesystem::remove(std::filesystem::path(softmaxOutput));
    std::filesystem::remove(std::filesystem::path(dL_dZ));
    std::filesystem::remove(std::filesystem::path(dL_dDenseLayerMatrix));
    std::filesystem::remove(std::filesystem::path(dL_dWordVector));
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

void forwardPropagation(const std::vector<unsigned> & X, const std::vector<std::pair<std::vector<double>, unsigned>> & wordEmbeddings,
                        const std::vector<std::vector<double>> & denseLayerMatrix, std::vector<std::vector<double>> & wordVector, std::string Z)
{
    for (unsigned i = 0; i < X.size(); i++)
        wordVector.push_back(wordEmbeddings[X[i]].first);
    transpose(wordVector);
    matMul(Z, denseLayerMatrix, wordVector);
    softmax(Z, denseLayerMatrix.size(), wordVector[0].size());
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

void transpose(std::string v, unsigned rows, unsigned cols)
{
    std::ifstream input(v, std::ios::binary);
    std::vector<std::vector<double>> result;
    double currentValue;
    for (unsigned i = 0; i < cols; i++)
    {
        result.push_back(std::vector<double>());
        for (unsigned j = 0; j < rows; j++)
        {
            input.seekg(j * cols * sizeof(double) + i * sizeof(double), std::ios::beg);
            input.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
            result[i].push_back(currentValue);
        }
    }
    input.close();
    std::ofstream output(v, std::ios::binary);
    for (unsigned i = 0; i < cols; i++)
    {
        for (unsigned j = 0; j < rows; j++)
        {
            output.write(reinterpret_cast<char *>(&result[i][j]), sizeof(double));
        }
    }
    output.close();
}

void matMul(std::string result, const std::vector<std::vector<double>> & m1, const std::vector<std::vector<double>> & m2)
{
    if (m1[0].size() != m2.size())
        std::exit(EXIT_FAILURE);
    double resultElement;
    std::ofstream output(result, std::ios::binary);
    for (unsigned i = 0; i < m1.size(); i++)
    {
        for (unsigned j = 0; j < m2[0].size(); j++)
        {
            resultElement = 0.0L;
            for (unsigned k = 0; k < m2.size(); k++)
                resultElement += m1[i][k] * m2[k][j];
            output.write(reinterpret_cast<char *>(&resultElement), sizeof(resultElement));
        }
    }
    output.close();
}

void matMul(std::string result, std::string m1, unsigned m1Rows, const std::vector<std::vector<double>> & m2)
{
    double resultElement, m1Element;
    std::ifstream m1File(m1, std::ios::binary);
    std::ofstream output(result, std::ios::binary);
    for (unsigned i = 0; i < m1Rows; i++)
    {
        for (unsigned j = 0; j < m2[0].size(); j++)
        {
            resultElement = 0.0L;
            m1File.seekg(i * m2.size() * sizeof(double), std::ios::beg);
            for (unsigned k = 0; k < m2.size(); k++)
            {
                m1File.read(reinterpret_cast<char *>(&m1Element), sizeof(double));
                resultElement += m1Element * m2[k][j];
            }
            output.write(reinterpret_cast<char *>(&resultElement), sizeof(resultElement));
        }
    }
    m1File.close();
    output.close();
}

void matMul(std::string result, const std::vector<std::vector<double>> & m1, std::string m2, unsigned m2Cols)
{
    double resultElement, m2Element;
    std::ifstream m2File(m2, std::ios::binary);
    std::ofstream output(result, std::ios::binary);
    for (unsigned i = 0; i < m1.size(); i++)
    {
        for (unsigned j = 0; j < m2Cols; j++)
        {
            resultElement = 0.0L;
            m2File.seekg(j * sizeof(double), std::ios::beg);
            for (unsigned k = 0; k < m2.size(); k++)
            {
                m2File.read(reinterpret_cast<char *>(&m2Element), sizeof(double));
                resultElement += m1[i][k] * m2Element;
                m2File.seekg(m2Cols * sizeof(double));
            }
            output.write(reinterpret_cast<char *>(&resultElement), sizeof(resultElement));
        }
    }
    m2File.close();
    output.close();
}

void softmax(std::string v, unsigned rows, unsigned cols)
{
    std::fstream file(v, std::ios::in | std::ios::ate | std::ios::binary);
    double currentValue;
    for (unsigned i = 0; i < cols; i++)
    {
        file.seekg(i * sizeof(double), std::ios::beg);
        file.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
        double param = currentValue;
        for (unsigned j = 1; j < rows; j++)
        {
            file.seekg(j * cols * sizeof(double) + i * sizeof(double), std::ios::beg);
            file.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
            if (param < currentValue)
            {
                param = currentValue;
            }
        }
        double sum = 0.0L;
        for (unsigned j = 0; j < rows; j++)
        {
            file.seekg(j * cols * sizeof(double) + i * sizeof(double), std::ios::beg);
            file.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
            if (currentValue - param < -7.0L)
            {
                continue;
            }
            else
            {
                sum += std::exp(currentValue - param);
            }
        }
        for (unsigned j = 0; j < rows; j++)
        {
            file.seekg(j * cols * sizeof(double) + i * sizeof(double), std::ios::beg);
            file.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
            file.seekp(j * cols * sizeof(double) + i * sizeof(double), std::ios::beg);
            if (currentValue - param < -7.0L)
            {
                currentValue = 0.0L;
                file.write(reinterpret_cast<char *>(&currentValue), sizeof(double));
            }
            else
            {
                currentValue = std::exp(currentValue - param) / sum;
                file.write(reinterpret_cast<char *>(&currentValue), sizeof(double));
            }
        }
    }
    file.close();
}

void backwardPropagation(std::string dL_dZ, std::string dL_dDenseLayerMatrix, std::string dL_dWordVector,
                         const std::vector<unsigned> & Y, std::string softmaxOutput, const std::vector<std::vector<double>> & denseLayerMatrix,
                         const std::vector<std::vector<double>> & wordVector)
{
    std::fstream softmaxFile(softmaxOutput, std::ios::in | std::ios::binary);
    std::fstream dL_dZFile(dL_dZ, std::ios::ate | std::ios::binary);
    double currentValue;
    for (unsigned i = 0; i < denseLayerMatrix.size(); i++)
    {
        for (unsigned j = 0; j < wordVector[0].size(); j++)
        {
            softmaxFile.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
            currentValue -= Y[j];
            dL_dZFile.write(reinterpret_cast<char *>(&currentValue), sizeof(double));
        }
    }
    softmaxFile.close();
    dL_dZFile.close();
    std::vector<std::vector<double>> tempWordVector = wordVector;
    transpose(tempWordVector);
    matMul(dL_dDenseLayerMatrix, dL_dZ, denseLayerMatrix.size(), tempWordVector);
    std::fstream dL_dDenseLayerMatrixFile(dL_dDenseLayerMatrix, std::ios::in | std::ios::ate | std::ios::binary);
    for (unsigned i = 0; i < denseLayerMatrix.size(); i++)
    {
        for (unsigned j = 0; j < tempWordVector[0].size(); j++)
        {
            dL_dDenseLayerMatrixFile.read(reinterpret_cast<char *>(&currentValue), sizeof(double));
            currentValue *= 1.0L / tempWordVector.size();
            dL_dDenseLayerMatrixFile.write(reinterpret_cast<char *>(&currentValue), sizeof(double));
        }
    }
    dL_dDenseLayerMatrixFile.close();
    std::vector<std::vector<double>> tempDenseLayerMatrix = denseLayerMatrix;
    transpose(tempDenseLayerMatrix);
    matMul(dL_dWordVector, tempDenseLayerMatrix, dL_dZ, wordVector[0].size());
}

// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef GRAPH_DATA_STRUCTURE_H_
#define GRAPH_DATA_STRUCTURE_H_

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <math.h>
#include <vector>
#include <unordered_map>

#include "../generator/EdgeListDataStructure.h"
#include "../../csr_reference.h"

#include "./enums/HashAlgorithmEnum.h"

typedef struct ShufflingEfficiencyStats{
    int64_t lowest_degreesum;
    int64_t highest_degreesum;
    int64_t total_degreesum;
    double mean_degreesum;
    double variance_degreesum;
} ShufflingEfficiencyStats;


class CSRGraph {

    public:
        CSRGraph(){   
            
        }
        
        CSRGraph(ResultFromGenerator generatorResult){  
            this->generatorResult = generatorResult;
        }

        ~CSRGraph() {
            ReleaseResources();
        }

        void ReleaseResources() {
            index.clear();
            neighbors.clear();
        }

        //getter/setter generatorResult
        ResultFromGenerator getGeneratorResult() {
            return generatorResult;
        }
        void setGeneratorResult(ResultFromGenerator generatorResult) {
            this->generatorResult = generatorResult;
        }

        //getter/setter index
        std::vector<int> getIndex() {
            return index;
        }
        void setIndex(std::vector<int> index) {
            this->index = index;
        }
        void setIndexSize(int size) {
            this->index.resize(size);
        }
        void setIndexAtPos(int pos, int val) {
            if(pos>= index.size())
                return;
            this->index[pos]=val;
        }

        //getter/setter neighbors
        std::vector<int64_t> getNeighbors() {
            return neighbors;
        }
        void setNeighbors(std::vector<int64_t> neighbors) {
            this->neighbors = neighbors;
        }
        void setNeighborsSize(int size) {
            this->neighbors.resize(size);
        }
        void setNeighborsAtPos(int pos, int64_t val) {
            if(pos>= neighbors.size())
                return;
            this->neighbors[pos]=val;
        }
        void addNeighbors(int64_t val) {
            this->neighbors.push_back(val);
        }
        
        //getter/setter local_vertices
        int getLocalVertices() {
            return this->local_vertices;
        }
        void setLocalVertices(int local_vertices) {
            this->local_vertices = local_vertices;
        }

        //getter/setter vertex_to_local_map
        virtual int64_t vertex_to_local(int64_t v){
            return vertex_to_local_map[v];
        };
        void setVertexToLocal(int64_t vertex, int64_t vertex_local) {
            vertex_to_local_map[vertex] = vertex_local;
        }
        bool vertex_to_local_exists(int64_t v){
            if(vertex_to_local_map.find(v)!=vertex_to_local_map.end()){
                return true;
            }
            return false;
        }

        //getter/setter local_to_vertex_map
        virtual int64_t local_to_vertex(int64_t localv){
            return local_to_vertex_map[localv];
        }
        virtual int64_t local_to_vertex(int64_t pe, int64_t localv){
            return local_to_vertex_map[localv];
        }
        void setLocalToVertex(int64_t vertex_local, int64_t vertex) {
            local_to_vertex_map[vertex_local] = vertex;
        }

        //functionalities
        int64_t degree(int64_t v) {
            return index[v+1] - index[v];
        }
        virtual int64_t vertex_owner(int64_t v){
            int num_pes = generatorResult.total_pe;
            return (int)( (v) % num_pes);
        };

    protected:
        ResultFromGenerator generatorResult;
        std::vector<int> index;
        std::vector<int64_t> neighbors;

        int local_vertices;

        std::unordered_map<int64_t, int64_t> vertex_to_local_map;
        std::unordered_map<int64_t, int64_t> local_to_vertex_map;

};


class CyclicCSRGraph : public CSRGraph {
private:

public:
    CyclicCSRGraph(ResultFromGenerator generatorResult) : CSRGraph(generatorResult){  

    }

    int64_t local_to_vertex(int64_t pe, int64_t localv){
        int num_pes = generatorResult.total_pe;
        return (int64_t) ((uint64_t)localv*num_pes +  (int)(pe));
    }

    int64_t vertex_to_local(int64_t v){
        int num_pes = generatorResult.total_pe;
        return (int)( v / num_pes );
    }

    int64_t vertex_owner(int64_t v){
        int num_pes = generatorResult.total_pe;
        return (int)( (v) % num_pes);
    };
};


class RangeCSRGraph : public CSRGraph {
private:

public:
    RangeCSRGraph(ResultFromGenerator generatorResult) : CSRGraph(generatorResult){  

    }

    int64_t local_to_vertex(int64_t pe, int64_t localv){
        int num_pes = generatorResult.total_pe;
        return (int64_t) ((uint64_t)localv*num_pes +  (int)(pe));
    }

    int64_t vertex_to_local(int64_t v){
        int num_pes = generatorResult.total_pe;
        return (int)( v % num_pes );
    }

    int64_t vertex_owner(int64_t v){
        int num_pes = generatorResult.total_pe;
        return (int)( (v) / num_pes);
    };
};


class HashPartitionCSRGraph : public CSRGraph {
private:
    HashAlgorithmEnum hashAlgorithm;

public:
    HashPartitionCSRGraph(ResultFromGenerator generatorResult) : CSRGraph(generatorResult){  
        hashAlgorithm = SNAKE_HASH;
    }

    int64_t snake_hash(int64_t v, int64_t num_pes) {
        int64_t hi = v / n;
        int64_t lo = v % n;
        if (hi % 2 == 0) {
            return hi * n + lo;
        } else {
            return hi * n + (n - lo - 1);
        }
    }

    int64_t fibonacci_hash(int64_t v, int64_t num_pes) {
        const float golden_ratio = (1 + sqrt(5)) / 2;
        int64_t hash = (int64_t) floor(v * golden_ratio);
        return hash % num_pes;
    }

    unsigned int xor_hash(int64_t v, int64_t num_pes) {
        int64_t hash = v ^ 0x9e3779b9;
        return hash % num_pes;
    }

    unsigned int xor_fibo_hash(int64_t v, int64_t num_pes) {
        const int64_t prime = 0x9e3779b9;
        int64_t hash1 = v ^ prime;
        hash1 = hash1 % num_pes;

        const float golden_ratio = (1 + sqrt(5)) / 2;
        int64_t hash2 = (int64_t) floor(v * golden_ratio);
        hash2 = hash2 % num_pes;

        return (hash1 * hash2) % num_pes;
    }

    int64_t vertex_owner(int64_t v){
        int num_pes = generatorResult.total_pe;

        if(hashAlgorithm == SNAKE_HASH){
            return snake_hash(v, num_pes);
        }
        else if(hashAlgorithm == FIBONACCI_HASH){
            return fibonacci_hash(v, num_pes);
        }
        else if(hashAlgorithm == XOR_HASH){
            return xor_hash(v, num_pes);
        }
        else if(hashAlgorithm == FIBONACCI_XOR_HASH){
            return xor_fibo_hash(v, num_pes);
        }
        else{
            return snake_hash(v, num_pes);
        }
    };
};


class ResultFromBuilder {

    public:
        CSRGraph csrGraph;

        double build_graph_time;

        int total_pe;

        int SCALE;
        int edgefactor;
        int64_t local_edges;
        int64_t global_vertices;
        int64_t global_edges;

        ShufflingEfficiencyStats shufflingEfficiencyStats;

        void printShufflingEfficiencyStats(){
            fprintf(stderr, "lowest load on PE: %d s\n", shufflingEfficiencyStats.lowest_degreesum);
            fprintf(stderr, "highest load on PE: %d s\n", shufflingEfficiencyStats.highest_degreesum);
            fprintf(stderr, "Average load on PE: %f s\n", shufflingEfficiencyStats.mean_degreesum);
            fprintf(stderr, "Variance load on PE: %f s\n", shufflingEfficiencyStats.variance_degreesum);
            double highest_deviation_from_mean = max(
                abs(shufflingEfficiencyStats.lowest_degreesum - shufflingEfficiencyStats.mean_degreesum)
                , abs(shufflingEfficiencyStats.highest_degreesum - shufflingEfficiencyStats.mean_degreesum)
            );
            fprintf(stderr, "Highest deviation from Average: %f s\n", highest_deviation_from_mean);
        }

        int64_t local_to_vertex(int64_t localv){
            return csrGraph.vertex_to_local(localv);
        }
        int64_t vertex_to_local(int64_t v){
            return csrGraph.vertex_to_local(v);
        };
        int64_t vertex_owner(int64_t v){
            return csrGraph.vertex_owner(v);
        }
};

#endif  // GRAPH_DATA_STRUCTURE_H_

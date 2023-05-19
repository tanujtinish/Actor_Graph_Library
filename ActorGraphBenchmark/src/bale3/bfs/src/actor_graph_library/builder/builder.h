// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef BUILDER_H_
#define BUILDER_H_

#include <algorithm>
#include <cinttypes>
#include <fstream>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "GraphDataStructure.h"

#include "./selectors/BuildGraphCyclicSelector.h"
#include "./selectors/BuildGraphRangeSelector.h"
#include "./selectors/CheckShufflingEfficiencySelector.h"
#include "./enums/GraphBuilderTypeEnum.h"


/*
GAP Benchmark Suite
Class:  BuilderBase
Author: Scott Beamer

Given arguements from the command line (cli), returns a built graph
 - MakeGraph() will parse cli and obtain edgelist and call
   MakeGraphFromEL(edgelist) to perform actual graph construction
 - edgelist can be from file (reader) or synthetically generated (generator)
 - Common case: BuilderBase typedef'd (w/ params) to be Builder (benchmark.h)
*/

class BuilderBase {
    private:
        ResultFromBuilder builderResult;

        ResultFromGenerator generatorResult;
        GraphBuilderTypeEnum type;

        void convert(std::vector<std::vector<int64_t>> csrGraph){

            int local_vertices= csrGraph.size();

            builderResult.csrGraph.setLocalVertices(local_vertices);
            builderResult.csrGraph.setIndexSize(local_vertices+1);
            int degreePrefixSum=0;
            int i;
            for(i=0;i <local_vertices; i++){

                builderResult.csrGraph.setIndexAtPos(i, degreePrefixSum);

                std::vector<int64_t> nbrs= csrGraph[i];
                int nbrs_size= nbrs.size();
                for(int j=0; j<nbrs_size; j++){
                    builderResult.csrGraph.addNeighbors(nbrs[j]);
                }
                degreePrefixSum+= nbrs_size;
            }

            builderResult.csrGraph.setIndexAtPos(i, degreePrefixSum);

            builderResult.total_pe = generatorResult.total_pe;
            builderResult.SCALE = generatorResult.SCALE;
            builderResult.edgefactor = generatorResult.edgefactor;
            builderResult.local_edges = generatorResult.local_edges;
            builderResult.global_vertices = generatorResult.global_vertices;
            builderResult.global_edges = generatorResult.global_edges;
        }

    public:
        BuilderBase(ResultFromGenerator generatorResult) : BuilderBase(generatorResult, Cyclic) {}
        
        BuilderBase(ResultFromGenerator generatorResult, GraphBuilderTypeEnum type){
            this->generatorResult = generatorResult;
            this->type = type;

            builderResult = ResultFromBuilder();
            if(type == Cyclic)
                builderResult.csrGraph = CyclicCSRGraph(generatorResult);
            else if(type == Range)
                builderResult.csrGraph = RangeCSRGraph(generatorResult);
            else
                builderResult.csrGraph = CSRGraph(generatorResult);
        }

        ResultFromBuilder buildCSRGraph(){

            std::vector<std::vector<int64_t>> csrGraph;

            double build_graph_start = MPI_Wtime();

            if(type == Range)
            {
                if(my_pe() == 0){
                    printf("Building Graph using 'Range Based' builder\n");
                }
                BuildGraphRangeSelector *bfss_ptr = new BuildGraphRangeSelector(builderResult, &generatorResult, &csrGraph);
                hclib::finish([=] {
                    bfss_ptr->start();
                    bfss_ptr->send(0, {}, my_pe());
                    bfss_ptr->done(0);
                });
                lgp_barrier();
            }
            else if(type == Cyclic){
                if(my_pe() == 0){
                    printf("Building Graph using 'Cyclic Based' builder\n");
                }
                BuildGraphCyclicSelector *bfss_ptr = new BuildGraphCyclicSelector(builderResult, &generatorResult, &csrGraph);
                hclib::finish([=] {
                    bfss_ptr->start();
                    bfss_ptr->send(0, {}, my_pe());
                    bfss_ptr->done(0);
                });
                lgp_barrier();
            }
            else if(type == HashPartition){
                if(my_pe() == 0){
                    printf("Building Graph using 'Hash Partition Based' builder\n");
                }
                BuildGraphHashPartitionSelector *bfss_ptr = new BuildGraphHashPartitionSelector(builderResult, &generatorResult, &csrGraph);
                hclib::finish([=] {
                    bfss_ptr->start();
                    bfss_ptr->send(0, {}, my_pe());
                    bfss_ptr->done(0);
                });
                lgp_barrier();
            }
            else{
                if(my_pe() == 0){
                    printf("Building Graph using default 'Cyclic Based' builder\n");
                }
                BuildGraphCyclicSelector *bfss_ptr = new BuildGraphCyclicSelector(builderResult, &generatorResult, &csrGraph);
                hclib::finish([=] {
                    bfss_ptr->start();
                    bfss_ptr->send(0, {}, my_pe());
                    bfss_ptr->done(0);
                });
                lgp_barrier();
            }
            convert(csrGraph);

            double build_graph_stop = MPI_Wtime();
            builderResult.build_graph_time = build_graph_stop - build_graph_start;

            calculateAndSetShufflingEfficiencyStats();

            return builderResult;
        }

        void calculateAndSetShufflingEfficiencyStats(){
            std::unordered_map<int, int64_t> pe_to_degreesum_map;

            CheckShufflingEfficiencySelector *bfss_ptr = new CheckShufflingEfficiencySelector(builderResult, pe_to_degreesum_map);
            hclib::finish([=] {
                bfss_ptr->start();
                bfss_ptr->send(0, {}, my_pe());
                bfss_ptr->done(0);
            });
            lgp_barrier();

            int64_t highest_degreesum=-1;
            int64_t lowest_degreesum = INT_MAX;
            int64_t total_degreesum =0;
            int64_t total_degreesum =0;
            double mean_degreesum=0.0, variance_degreesum=0.0;

            for(std::pair<int, int64_t> pe_to_degreesum: pe_to_degreesum_map){
                int64_t sum_of_degree = pe_to_degreesum.second;
                
                total_degreesum += sum_of_degree;

                if(sum_of_degree > highest_degreesum){
                    highest_degreesum = sum_of_degree;
                }

                if(sum_of_degree < highest_degreesum){
                    lowest_degreesum = sum_of_degree;
                }
            }
            mean_degreesum = double(total_degreesum)/double(builderResult.total_pe);

            for(std::pair<int, int64_t> pe_to_degreesum: pe_to_degreesum_map){
                int64_t sum_of_degree = pe_to_degreesum.second;
                variance_degreesum += pow(sum_of_degree - mean_degreesum, 2);
            }
            variance_degreesum /= double(builderResult.total_pe);

            builderResult.shufflingEfficiencyStats.lowest_degreesum = lowest_degreesum;
            builderResult.shufflingEfficiencyStats.highest_degreesum = highest_degreesum;
            builderResult.shufflingEfficiencyStats.total_degreesum = total_degreesum;
            builderResult.shufflingEfficiencyStats.mean_degreesum = mean_degreesum;
            builderResult.shufflingEfficiencyStats.variance_degreesum = variance_degreesum;
        }

};

#endif  // BUILDER_H_

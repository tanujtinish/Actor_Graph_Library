#include "../../../common.h"

#include "libgetput.h"
#include "selector.h"

#include "../EdgeListDataStructure.h"

#include <inttypes.h>
#include <random>

typedef struct uniformGeneratorSelectorVisitmsg {
	
} uniformGeneratorSelectorVisitmsg;

class UniformGeneratorSelector: public hclib::Selector<1, uniformGeneratorSelectorVisitmsg> {
    
    int total_pe;
    int SCALE;
    int edgefactor;
    int64_t global_vertices;
    int64_t global_edges;
    int64_t local_edges;

    packed_edge* restrict edgememory; /* NULL if edges are in file */
    int64_t edgememory_size;
    int64_t max_edgememory_size;
    float* weightmemory;

    EdgeList edgeList;
    WEdgeList wEdgeList;
    int64_t edgelist_size;
    int64_t max_edgelist_size;

    ResultFromGenerator* generatorResult;

    int64_t kRandSeed = 27491095 + rank;

    void saveBackResult() {
        generatorResult->total_pe = total_pe;
        generatorResult->SCALE = SCALE;
        generatorResult->edgefactor = edgefactor;
        generatorResult->local_edges = local_edges;
        generatorResult->global_vertices = global_vertices;
        generatorResult->global_edges = global_edges;

        generatorResult->edgememory = edgememory;
        generatorResult->edgememory_size = edgememory_size;
        generatorResult->max_edgememory_size = max_edgememory_size;
        generatorResult->weightmemory = weightmemory;

        std::vector<Edge>::iterator it1;
        generatorResult->edgeList.clear();
        for(it1=edgeList.begin(); it1!=edgeList.end(); it1++){
            (generatorResult->edgeList).push_back(*it1);
        }

        std::vector<WEdge>::iterator it2;
        generatorResult->wEdgeList.clear();
        for(it2=wEdgeList.begin(); it2!=wEdgeList.end(); it2++){
            (generatorResult->wEdgeList).push_back(*it2);
        }

        generatorResult->edgelist_size = edgelist_size;
        generatorResult->max_edgelist_size = max_edgelist_size;
    }

    // Overwrites existing weights with random from [1,255]
    void insertWeights(EdgeList el) {
        std::mt19937 rng;
        rng.seed(kRandSeed);
        std::uniform_int_distribution<int> udist(1, 255);
        int64_t el_size = el.size();

        wEdgeList.resize(el_size);
        for (int64_t e=0; e < el_size; e+=1) {
            wEdgeList[e].u = el[e].u;
            wEdgeList[e].v.v = el[e].v;
            wEdgeList[e].v.w = static_cast<int64_t>(udist(rng));
        }
    }

    void process(uniformGeneratorSelectorVisitmsg m, int sender_rank) {
        total_pe = size;
        int64_t nedges_per_pe = global_edges / total_pe;
        int64_t extraedges = global_edges % total_pe;
        int64_t my_pe = rank;
        if(my_pe < extraedges){
            local_edges = nedges_per_pe + 1;
        }else{
            local_edges = nedges_per_pe;
        }
        //packed_edge* buf = (packed_edge*)xmalloc(FILE_CHUNKSIZE * sizeof(packedtg.edgememory_size = nedges;
        edgememory_size = local_edges;
        edgememory = (packed_edge*)xmalloc(local_edges * sizeof(packed_edge));

        edgeList.resize(local_edges);
        edgelist_size = local_edges;

        int64_t temp;
        std::mt19937 rng;
        rng.seed(kRandSeed);
        std::uniform_int_distribution<int> udist(0, global_vertices-1);
        for(int m = 0; m < local_edges; m++){
            int64_t u = udist(rng);		
            int64_t v = udist(rng);
            
            // Ask scott if necessary + what to do if u = v
            if(u > v){
                temp = u;
                u = v;
                v = temp;
            }
            packed_edge e;
            write_edge(&e, u, v);
            edgememory[m] = e;

            edgeList[m] = Edge(u, v);
        }
	
        MPI_Allreduce(&edgememory_size, &max_edgememory_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&edgelist_size, &max_edgelist_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);

        saveBackResult();
    }

public:
UniformGeneratorSelector(int edgefactor, int SCALE, ResultFromGenerator* generatorResult){

        this->SCALE = SCALE; 
        this->edgefactor = edgefactor; 
        this->global_vertices = (int64_t)(1) << SCALE; 
        this->global_edges = (int64_t)(edgefactor) << SCALE;

        this->generatorResult = generatorResult;

        mb[0].process = [this](uniformGeneratorSelectorVisitmsg pkt, int sender_rank) { this->process(pkt, sender_rank); };
    }
};
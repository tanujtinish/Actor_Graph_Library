#include "../../../common.h"

#include "libgetput.h"
#include "selector.h"

#include "../../generator/EdgeListDataStructure.h"
#include "../GraphDataStructure.h"

#include <vector>
#include <inttypes.h>
#include <random>

typedef struct buildGraphHashPartitionSelectorVisitmsg {
	int64_t vertex;
    int64_t vertex_neigbour;

} buildGraphHashPartitionSelectorVisitmsg;

class BuildGraphHashPartitionSelector: public hclib::Selector<2, buildGraphHashPartitionSelectorVisitmsg> {
    
    ResultFromBuilder& builderResult;
    
    ResultFromGenerator* generatorResult;
    int local_vertices;
    int local_vertices_to_be_assigned;

    std::vector<std::vector<int64_t>>* csrGraphReference;

    void suffleEdges(buildGraphHashPartitionSelectorVisitmsg m, int sender_rank) {
        
        std::vector<Edge>::iterator it;
        for(it=generatorResult->edgeList.begin(); it!=generatorResult->edgeList.end(); it++){

            int dest1 = builderResult.vertex_owner(it->u);
            buildGraphHashPartitionSelectorVisitmsg m1 = {it->u, it->v};
            send(1, m1, dest1);

            int dest2 = builderResult.vertex_owner(it->v);
            buildGraphHashPartitionSelectorVisitmsg m2 = {it->v, it->u};
            send(1, m2, dest2);
        }

    }

    void recieveEdges(buildGraphHashPartitionSelectorVisitmsg m, int sender_rank) {
        int64_t vertex_local_index;
        if(builderResult.csrGraph.vertex_to_local_exists(m.vertex))
            vertex_local_index = builderResult.csrGraph.vertex_to_local(m.vertex);
        else{    
            vertex_local_index = local_vertices_to_be_assigned++;
            builderResult.csrGraph.setVertexToLocal(m.vertex, vertex_local_index);
            builderResult.csrGraph.setLocalToVertex(vertex_local_index, m.vertex);
        }
        
        if(local_vertices < vertex_local_index+1){
            local_vertices=vertex_local_index+1;
            csrGraphReference->resize(local_vertices);
        }

        csrGraphReference->at(vertex_local_index).push_back(m.vertex_neigbour);
        builderResult.csrGraph.setVertexToLocal(m.vertex, vertex_local_index);
        builderResult.csrGraph.setLocalToVertex(vertex_local_index, m.vertex);
    }

public:
    BuildGraphHashPartitionSelector(ResultFromBuilder& builderResult, ResultFromGenerator* generatorResult, std::vector<std::vector<int64_t>>* csrGraphReference): builderResult(builderResult), csrGraphReference(csrGraphReference){
        
        this->generatorResult = generatorResult;
        this->csrGraphReference = csrGraphReference;
        local_vertices=0;

        int local_vertices_to_be_assigned=0;

        mb[0].process = [this](buildGraphHashPartitionSelectorVisitmsg pkt, int sender_rank) { this->suffleEdges(pkt, sender_rank); };
        mb[1].process = [this](buildGraphHashPartitionSelectorVisitmsg pkt, int sender_rank) { this->recieveEdges(pkt, sender_rank); };

    }
};
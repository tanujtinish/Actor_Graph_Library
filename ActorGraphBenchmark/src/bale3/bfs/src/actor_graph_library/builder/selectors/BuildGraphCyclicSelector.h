#include "../../../common.h"

#include "libgetput.h"
#include "selector.h"

#include "../../generator/EdgeListDataStructure.h"
#include "../GraphDataStructure.h"

#include <vector>
#include <inttypes.h>
#include <random>

typedef struct buildGraphCyclicSelectorVisitmsg {
	int64_t vloc;
    int64_t vneigbour;

} buildGraphCyclicSelectorVisitmsg;

class BuildGraphCyclicSelector: public hclib::Selector<2, buildGraphCyclicSelectorVisitmsg> {
    
    ResultFromBuilder builderResult;
    
    ResultFromGenerator* generatorResult;
    int local_vertices;

    std::vector<std::vector<int64_t>>* csrGraphReference;

    void suffleEdges(buildGraphCyclicSelectorVisitmsg m, int sender_rank) {
        
        std::vector<Edge>::iterator it;
        for(it=generatorResult->edgeList.begin(); it!=generatorResult->edgeList.end(); it++){

            int dest1 = builderResult.vertex_owner(it->u);
            buildGraphCyclicSelectorVisitmsg m1 = {it->u, it->v};
            send(1, m1, dest1);

            int dest2 = builderResult.vertex_owner(it->v);
            buildGraphCyclicSelectorVisitmsg m2 = {it->v, it->u};
            send(1, m2, dest2);
        }

    }

    void recieveEdges(buildGraphCyclicSelectorVisitmsg m, int sender_rank) {
        int64_t vertex_local_index = builderResult.vertex_local(m.vloc);
        
        if(local_vertices < vertex_local_index+1){
            local_vertices=vertex_local_index+1;
            csrGraphReference->resize(local_vertices);
        }

        csrGraphReference->at(vertex_local_index).push_back(m.vneigbour);
    }

public:
    BuildGraphCyclicSelector(ResultFromBuilder builderResult, ResultFromGenerator* generatorResult, std::vector<std::vector<int64_t>>* csrGraphReference): csrGraphReference(csrGraphReference){
        
        this->builderResult = builderResult;
        
        this->generatorResult = generatorResult;
        this->csrGraphReference = csrGraphReference;
        local_vertices=0;

        mb[0].process = [this](buildGraphCyclicSelectorVisitmsg pkt, int sender_rank) { this->suffleEdges(pkt, sender_rank); };
        mb[1].process = [this](buildGraphCyclicSelectorVisitmsg pkt, int sender_rank) { this->recieveEdges(pkt, sender_rank); };

    }
};
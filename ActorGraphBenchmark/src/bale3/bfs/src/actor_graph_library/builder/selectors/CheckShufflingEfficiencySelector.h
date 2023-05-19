#include "libgetput.h"
#include "selector.h"

#include "../../generator/EdgeListDataStructure.h"
#include "../GraphDataStructure.h"

#include <vector>
#include <inttypes.h>
#include <random>

typedef struct checkShufflingEfficiencyVisitmsg {
	int pe_number;
    int64_t sum_of_degree;
} checkShufflingEfficiencyVisitmsg;

class CheckShufflingEfficiencySelector: public hclib::Selector<2, checkShufflingEfficiencyVisitmsg> {
    
    ResultFromBuilder& builderResult;

    std::unordered_map<int, int64_t> pe_to_degreesum_map;

    void sendDegreeSumToPE0(checkShufflingEfficiencyVisitmsg m, int sender_rank) {
        
        int dest = 0;
        checkShufflingEfficiencyVisitmsg m = {my_pe(), builderResult.csrGraph.getNeighbors().size()};
        send(1, m, dest);

    }

    void recieveDegreeSumsAndUpdateStats(checkShufflingEfficiencyVisitmsg m, int sender_rank) {
        
        int pe_number = m.pe_number;
        int sum_of_degree = m.sum_of_degree;

        pe_to_degreesum_map[pe_number] = sum_of_degree;

    }

public:
    CheckShufflingEfficiencySelector(ResultFromBuilder& builderResult, std::unordered_map<int, int64_t>& pe_to_degreesum_map): builderResult(builderResult), pe_to_degreesum_map(pe_to_degreesum_map){

        mb[0].process = [this](checkShufflingEfficiencyVisitmsg pkt, int sender_rank) { this->sendDegreeSumToPE0(pkt, sender_rank); };
        mb[1].process = [this](checkShufflingEfficiencyVisitmsg pkt, int sender_rank) { this->recieveDegreeSumsAndUpdateStats(pkt, sender_rank); };

    }
};
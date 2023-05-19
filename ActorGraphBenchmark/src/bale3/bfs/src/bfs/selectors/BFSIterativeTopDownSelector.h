
#ifndef BFS_ITERATIVE_TOP_DOWN_SELECTOR_H_
#define BFS_ITERATIVE_TOP_DOWN_SELECTOR_H_

typedef struct bfsIterativeTopDownVisitmsg {
	int vto;
	int vfrom;
} bfsIterativeTopDownVisitmsg;

class BFSIterativeTopDownSelector: public hclib::Selector<1, bfsIterativeTopDownVisitmsg> {
    std::vector<int64_t>& nextFrontier;

    int64_t *pred_glob;
    std::vector<bool>& visited;
    CSRGraph* csrGraph;

    void set_visited(int v){
        visited[v] = 1;
    }

    bool test_visited(int v){
        if (visited[v]==1){
            return true;
        }
        return false;
    }

    void process(bfsIterativeTopDownVisitmsg m, int sender_rank) {
        int vlocal = csrGraph->vertex_to_local(m.vto);
        if (!test_visited(vlocal)) {
            set_visited(vlocal);
            nextFrontier.push_back(m.vto);
            pred_glob[vlocal] = m.vfrom;
        }
    }

public:
BFSIterativeTopDownSelector(std::vector<int64_t>& _nextFrontier, int64_t* _pred_glob, std::vector<bool>& _visited, CSRGraph* _csrGraph): nextFrontier(_nextFrontier), pred_glob(_pred_glob), visited(_visited), csrGraph(_csrGraph) {
        mb[0].process = [this](bfsIterativeTopDownVisitmsg pkt, int sender_rank) { this->process(pkt, sender_rank); };
    }
};

#endif  // BFS_ITERATIVE_TOP_DOWN_SELECTOR_H_
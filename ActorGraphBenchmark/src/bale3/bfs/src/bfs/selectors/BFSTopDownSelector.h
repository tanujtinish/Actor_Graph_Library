
#ifndef BFS_TOP_DOWN_SELECTOR_H_
#define BFS_TOP_DOWN_SELECTOR_H_

typedef struct bfsTopDownVisitmsg {
	int vto;
	int vfrom;
} bfsTopDownVisitmsg;

class BFSTopDownSelector: public hclib::Selector<2, bfsTopDownVisitmsg> {
    std::vector<int64_t>& nextFrontier, frontier;
    
    int64_t *pred_glob;
    std::vector<bool>& visited;
    
    CSRGraph* csrGraph;
    std::vector<int> index;
    std::vector<int64_t> neighbors;

    void set_visited(int v){
        visited[v] = 1;
    }

    bool test_visited(int v){
        if (visited[v]==1){
            return true;
        }
        return false;
    }

    void process(bfsTopDownVisitmsg m, int sender_rank) {
        //for all vertices in current level send visit AMs to all neighbours
        for(int i=0;i<frontier.size();i++){
            int vlocal = csrGraph->vertex_to_local(frontier[i]);
            for(int j=index[vlocal];j<index[vlocal+1];j++) {
                int dest_vertex = neighbors[j];
                int vertex_owner = csrGraph->vertex_owner(dest_vertex);
                bfsTopDownVisitmsg m = {dest_vertex, frontier[i]};
                send(1, m, vertex_owner);
            }
        }
        done(1);
    }

     void poll_neighbors(bfsTopDownVisitmsg m, int sender_rank) {
        int vlocal = csrGraph->vertex_to_local(m.vto);
        if (!test_visited(vlocal)) {
            set_visited(vlocal);
            nextFrontier.push_back(m.vto);
            pred_glob[vlocal] = m.vfrom;
        }
    }

public:
BFSTopDownSelector(std::vector<int64_t>& _frontier, std::vector<int64_t>& _nextFrontier, int64_t* _pred_glob, std::vector<bool>& _visited, CSRGraph* _csrGraph): frontier(_frontier), nextFrontier(_nextFrontier), pred_glob(_pred_glob), visited(_visited), csrGraph(_csrGraph) {
        this->index = csrGraph->getIndex();
        this->neighbors = csrGraph->getNeighbors();
        
        mb[0].process = [this](bfsTopDownVisitmsg pkt, int sender_rank) { this->process(pkt, sender_rank); };
        mb[1].process = [this](bfsTopDownVisitmsg pkt, int sender_rank) { this->poll_neighbors(pkt, sender_rank); };

    }
};

#endif  // BFS_TOP_DOWN_SELECTOR_H_
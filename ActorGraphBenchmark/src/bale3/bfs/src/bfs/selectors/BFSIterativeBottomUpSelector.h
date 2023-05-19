#ifndef BFS_ITERATIVE_BOTTOM_UP_SELECTOR_H_
#define BFS_ITERATIVE_BOTTOM_UP_SELECTOR_H_

typedef struct bfsIterativeBottomUpVisitmsg {
	int vto;
    int vfrom;
} bfsIterativeBottomUpVisitmsg;

class BFSIterativeBottomUpSelector: public hclib::Selector<2, bfsIterativeBottomUpVisitmsg> {
    int *nodes_in_level;
    int64_t *pred_glob;
    std::vector<bool>& visited_temp;
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

    void set_visited_temp(int v){
        visited_temp[v] = 1;
    }

    bool test_visited_temp(int v){
        if (visited_temp[v]==1){
            return true;
        }
        return false;
    }

    void check_node_visited(bfsIterativeBottomUpVisitmsg m, int sender_rank) {
            int vlocal = csrGraph->vertex_to_local(m.vto);
            if(test_visited(vlocal)){
                bfsIterativeBottomUpVisitmsg m2 = {m.vfrom, m.vto};
                send(1, m2, sender_rank);
            }
    }

    void add_to_next_level(bfsIterativeBottomUpVisitmsg m, int sender_rank) {
        int vlocal = csrGraph->vertex_to_local(m.vto);
        if (!test_visited_temp(vlocal)) {

            set_visited_temp(vlocal);
            *nodes_in_level = *nodes_in_level + 1;

            pred_glob[vlocal] = m.vfrom;
        }
    }

public:
    BFSIterativeBottomUpSelector(int* _nodes_in_level, int64_t* _pred_glob, std::vector<bool>& _visited, std::vector<bool>& _visited_temp, CSRGraph* _csrGraph): nodes_in_level(_nodes_in_level), pred_glob(_pred_glob), visited(_visited), visited_temp(_visited_temp), csrGraph(_csrGraph) {
        mb[0].process = [this](bfsIterativeBottomUpVisitmsg pkt, int sender_rank) { this->check_node_visited(pkt, sender_rank); };
        mb[1].process = [this](bfsIterativeBottomUpVisitmsg pkt, int sender_rank) { this->add_to_next_level(pkt, sender_rank); };
    }
};

#endif  // BFS_ITERATIVE_BOTTOM_UP_SELECTOR_H_
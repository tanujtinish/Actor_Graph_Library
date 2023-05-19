#ifndef BFS_BOTTOM_UP_SELECTOR_H_
#define BFS_BOTTOM_UP_SELECTOR_H_

typedef struct bfsBottomUpVisitmsg {
	int actorfrom;
    int vto;
	int vfrom;
} bfsBottomUpVisitmsg;

class BFSBottomUpSelector: public hclib::Selector<4, bfsBottomUpVisitmsg> {
    int *nodes_in_level;

    int64_t *pred_glob;
    std::vector<bool>& visited_temp;
    std::vector<bool>& visited;

    CSRGraph* csrGraph;
    std::vector<int> index;
    std::vector<int64_t> neighbors;
    int local_vertices;

    unsigned int *next_neigbour_number;
    int neigbours_batch_number=0;
    int vertices_handled = 0;
    int vertices_to_be_handled = 0;

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

    void print_send_message(int actor_from, int actor_to, int dest_pe, int vertex_from, int vertex_to){
        printf("pe: %d, sending message: (%d, %d, %d) -> (%d, %d, %d)\n", rank, rank, actor_from, vertex_from, dest_pe, actor_to, vertex_to);
        fflush(stdout);
    }

    void print_recieve_message(int actor_from, int actor_to, int from_pe, int vertex_from, int vertex_to){
        printf("pe: %d, recieveing message: (%d, %d, %d) -> (%d, %d, %d)\n", rank, from_pe, actor_from, vertex_from, rank, actor_to, vertex_to);
        fflush(stdout);
    }

    void print_vertex_handled(int vertex, int vertices_handled, int vertices_to_be_handled){
        printf("pe: %d, ver= (%d, %d), vertices_handled= %d, vertices_to_be_handled= %d\n", rank, rank, vertex, vertices_handled, vertices_to_be_handled);
        fflush(stdout);
    }

    void send_to_first_neigbours(bfsBottomUpVisitmsg m, int sender_rank) {

        for (int i = 0; i < local_vertices; i++){

            if (test_visited(i) || index[i]==index[i+1]) {
                continue;
            }
            vertices_to_be_handled++;

            int j = index[i];

            next_neigbour_number[i]=2;

            int vertex = csrGraph->local_to_vertex(i);
            int dest = csrGraph->vertex_owner(neighbors[j]);
            bfsBottomUpVisitmsg m = {0, neighbors[j], vertex};
            
            print_send_message(0, 1, dest, vertex, neighbors[j]);
            
            send(1, m, dest);
        }
        if(vertices_to_be_handled==0){
            printf("pe: %d, vertices_to_be_handled=0 so exiting\n", my_pe());
            fflush(stdout);
            done(1);
        }
    }

    void check_node_visited(bfsBottomUpVisitmsg m, int sender_rank) {

            print_recieve_message(m.actorfrom, 1, sender_rank, m.vfrom, m.vto);

            int vlocal = csrGraph->vertex_to_local(m.vto);
            if(test_visited(vlocal)){
                bfsBottomUpVisitmsg m2 = {1, m.vfrom, m.vto};

                print_send_message(1, 3, sender_rank, m.vto, m.vfrom);

                send(3, m2, sender_rank);   
            }
            else{
                bfsBottomUpVisitmsg m2 = {1, m.vfrom, m.vto};

                print_send_message(1, 2, sender_rank, m.vto, m.vfrom);

                send(2, m2, sender_rank);
            }
    }

    void send_to_next_neigbours(bfsBottomUpVisitmsg m, int sender_rank) {

        print_recieve_message(m.actorfrom, 2, sender_rank, m.vfrom, m.vto);

        int i = csrGraph->vertex_to_local(m.vto);

        if(next_neigbour_number[i] <= index[i+1] - index[i]){
            int j = index[i] + next_neigbour_number[i] -1;

            int dest2 = csrGraph->vertex_owner(neighbors[j]);
            bfsBottomUpVisitmsg m2 = {2, neighbors[j], m.vto};

            print_send_message(2, 1, sender_rank, m.vto, neighbors[j]);           
            
            next_neigbour_number[i]++;
            send(1, m2, dest2);
        }
        else{
            vertices_handled++;
            
            print_vertex_handled(m.vto, vertices_handled, vertices_to_be_handled);
            
            if(vertices_handled == vertices_to_be_handled)
            {
                // done(1);
                done(0);
            }
        }

    }

    void mark_node_visited(bfsBottomUpVisitmsg m, int sender_rank) {

        print_recieve_message(m.actorfrom, 3, sender_rank, m.vfrom, m.vto);

        int vlocal = csrGraph->vertex_to_local(m.vto);
        set_visited_temp(vlocal);
        *nodes_in_level = *nodes_in_level + 1;
        pred_glob[vlocal] = m.vfrom;
        vertices_handled++;
        
        print_vertex_handled(vlocal, vertices_handled, vertices_to_be_handled);
        
        if(vertices_handled == vertices_to_be_handled)
        {
            // done(1);
            done(0);
        }
    }


public:
    BFSBottomUpSelector(int* _nodes_in_level, int64_t* _pred_glob, std::vector<bool>& _visited, std::vector<bool>& _visited_temp, CSRGraph* _csrGraph, int _local_vertices): nodes_in_level(_nodes_in_level), pred_glob(_pred_glob), visited(_visited), visited_temp(_visited_temp), csrGraph(_csrGraph), local_vertices(_local_vertices) {
        
        this->index = csrGraph->getIndex();
        this->neighbors = csrGraph->getNeighbors();
        this->local_vertices = local_vertices;

        next_neigbour_number = (unsigned int*)xmalloc((local_vertices) * sizeof(int));
        for(int i=0; i< local_vertices; i++) 
            next_neigbour_number[i]=1;

        mb[0].process = [this](bfsBottomUpVisitmsg pkt, int sender_rank) { this->send_to_first_neigbours(pkt, sender_rank); };
        mb[1].process = [this](bfsBottomUpVisitmsg pkt, int sender_rank) { this->check_node_visited(pkt,sender_rank); };
        mb[2].process = [this](bfsBottomUpVisitmsg pkt, int sender_rank) { this->send_to_next_neigbours(pkt, sender_rank); };
        mb[3].process = [this](bfsBottomUpVisitmsg pkt, int sender_rank) { this->mark_node_visited(pkt,sender_rank); };

        // mb[0].add_dep_mailboxes({}, {1});
        // mb[1].add_dep_mailboxes({0,2}, {2,3});
        // mb[2].add_dep_mailboxes({1}, {1});
        // mb[3].add_dep_mailboxes({1}, {});
    }
};



#endif  // BFS_BOTTOM_UP_SELECTOR_H_
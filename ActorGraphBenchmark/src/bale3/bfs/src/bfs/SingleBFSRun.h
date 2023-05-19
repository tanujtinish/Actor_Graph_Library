
#ifndef SingleBFSRun_H_
#define SingleBFSRun_H_

#include "./selectors/BFSTopDownSelector.h"
#include "./selectors/BFSBottomUpSelector.h"
#include "./selectors/BFSIterativeTopDownSelector.h"
#include "./selectors/BFSIterativeBottomUpSelector.h"

#include "./enums/BFSTypeEnum.h"

#include "../csr_reference.h"
#include "BFSDataStructure.h"

class SingleBFSRun {

    public:
        SingleBFSRun(ResultFromBuilder builderResult, int64_t *pred, int64_t source) : SingleBFSRun(builderResult, *pred, source, TopDownBFS) {}
        
        SingleBFSRun(ResultFromBuilder builderResult, int64_t *pred, int64_t source, BFSTypeEnum type){

            this->csrGraph = builderResult.csrGraph;
            this->index = csrGraph.getIndex();
            this->neighbors = csrGraph.getNeighbors();

            this->local_vertices = csrGraph.getLocalVertices();
            
            this->visited_size = local_vertices;
            this->visited.resize(visited_size);

            this->visited_temp_size=local_vertices;
            this->visited_temp.resize(visited_temp_size);

            this->pred = pred;
            for(int i=0;i<local_vertices;i++) 
                pred[i]=-1;
            this->type = type;

            this->source = source;

            run_bfs();
            validate_bfs();
            calculate_edge_count_for_teps();
        }

        double getBfsRunTime(){
            return this->bfsMetrics.bfs_run_time;
        }

        double getBfsValidationTime(){
            return this->bfsMetrics.bfs_validate_time;
        }

        double getValidationPassed(){
            return this->validation_passed;
        }

        double getBfsEdgeCount(){
            return this->bfsMetrics.bfs_edge_count;
        }

        double getBfsSecsPerEdge(){
            return this->bfsMetrics.bfs_secs_per_edge;
        }

        BfsMetrics getBfsMetrics(){
            return this->bfsMetrics;
        }

        void run_bfs(){
            double bfs_start = MPI_Wtime();
			if(type == TopDownBFS || type == TopDownIterativeBFS){
				
                if(type == TopDownBFS){
                    if(my_pe() == 0){
                        printf("Running 'Top Down' BFS\n");
                    }
                }
                else{
                    if(my_pe() == 0){
                        printf("Running 'Iteartive Top Down' BFS\n");
                    }
                }
                
                run_bfs_top_down(type);
			}
            else if  (type == BottomUpBFS || type == BottomUpIterativeBFS){
				if(type == TopDownBFS){
                    if(my_pe() == 0){
                        printf("Running 'Bottom UP' BFS\n");
                    }
                }
                else{
                    if(my_pe() == 0){
                        printf("Running 'Iteartive Bottom UP' BFS\n");
                    }
                }
                run_bfs_bottom_up(type);
			}
			else{
                if(my_pe() == 0){
                    printf("Running default 'Top Down UP' BFS\n");
                }
				run_bfs_top_down(TopDownBFS);
			}	
            double bfs_end = MPI_Wtime();
            bfsMetrics.bfs_run_time = bfs_end - bfs_start;
		};

    private:  

        void set_visited(int v){
            visited[csrGraph.vertex_to_local(v)] = 1;
        }
        void set_visitedloc(int v){
            visited[v] = 1;
        }
        bool test_visited(int v){
            if (visited[csrGraph.vertex_to_local(v)]==1){
                return true;
            }
            return false;
        }
        bool test_visitedloc(int v){
            if (visited[v]==1){
                return true;
            }
            return false;
        }
        void clean_visited(){
            std::fill(visited.begin(), visited.end(), 0);
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
        void clean_visited_temp(){
            std::fill(visited_temp.begin(), visited_temp.end(), 0);
        }   

        void validate_bfs(){
            if (rank == 0) fprintf(stderr, "Validating BFS\n");
            double bfs_validate_start = MPI_Wtime();
            
            double bfs_validate_end = MPI_Wtime();
            bfsMetrics.bfs_validate_time = bfs_validate_end - bfs_validate_start;

            validation_passed=true;
        }

        void calculate_edge_count_for_teps() {
            long i,j;
            long edge_count=0;
            for(i=0; i < local_vertices; i++)
                if(pred[i]!=-1) {
                    for(j=index[i]; j<index[i+1]; j++)
                        if(neighbors[j] < csrGraph.local_to_vertex(i))
                            edge_count++;
                }
            aml_long_allsum(&edge_count);

            bfsMetrics.bfs_edge_count = edge_count;
            bfsMetrics.bfs_secs_per_edge = bfsMetrics.bfs_run_time/bfsMetrics.bfs_edge_count;
            
        }

        void run_bfs_top_down(BFSTypeEnum type) {
            long sum=1;
            int it_num=0;

            std::vector<int64_t> frontier,nextFrontier;

            clean_visited();

            if(csrGraph.vertex_owner(source) == rank) {
                pred[csrGraph.vertex_to_local(source)]=source;
                set_visited(source);
                frontier.push_back(VERTEX_LOCAL(source));
            }

            // While there are vertices in current level
            while (sum) {
                if(type == TopDownBFS){
                    BFSTopDownSelector *bfss_ptr = new BFSTopDownSelector(frontier, nextFrontier, pred, visited, &csrGraph);
                    hclib::finish([=] {
                        bfss_ptr->start();
                        bfsTopDownVisitmsg m = {-1, -1};
                        bfss_ptr->send(0, m, my_pe());
                    });
                    lgp_barrier();
                    delete bfss_ptr;
                }
                else{
                    BFSIterativeTopDownSelector *bfss_ptr = new BFSIterativeTopDownSelector(nextFrontier, pred, visited, &csrGraph);
                    hclib::finish([=]() {
                        bfss_ptr->start();
                        
                        //for all vertices in current level send visit AMs to all neighbours
                        for(int i=0;i<frontier.size();i++){
                            int vlocal = csrGraph.vertex_to_local(frontier[i]);
                            for(int j=index[vlocal];j<index[vlocal+1];j++) {
                                int dest_vertex = neighbors[j];
                                int vertex_owner = csrGraph.vertex_owner(dest_vertex);
                                bfsIterativeTopDownVisitmsg m = {dest_vertex, frontier[i]};
                                bfss_ptr->send(0, m, vertex_owner);
                            }
                        }
                        
                        bfss_ptr->done(0);
                    });
                    lgp_barrier();
                    delete bfss_ptr;
                }
                it_num++;
                
                sum=nextFrontier.size();
                aml_long_allsum(&sum);

                frontier = nextFrontier;
                nextFrontier.clear();
            }
            lgp_barrier();
        }

        void run_bfs_bottom_up(BFSTypeEnum type) {
            long sum=1;
            int it_num=0;
            int nodes_in_level = 0;

            clean_visited();

            if(csrGraph.vertex_owner(source) == rank) {
                pred[csrGraph.vertex_to_local(source)]=source;
                set_visited(source);
            }

            // While there are vertices in current level
            while (sum) {

                if(type == BottomUpBFS){
                    BFSBottomUpSelector *bfss_ptr = new BFSBottomUpSelector(&nodes_in_level, pred, visited, visited_temp, &csrGraph, local_vertices);
                    hclib::finish([=]() {
                            bfss_ptr->start();
                            bfsBottomUpVisitmsg m = {-1, -1, -1};
                            bfss_ptr->send(0, m, my_pe());
                    });
                    lgp_barrier();
                    delete bfss_ptr;
                }
                else{
                    BFSIterativeBottomUpSelector *bfss_ptr = new BFSIterativeBottomUpSelector(&nodes_in_level, pred, visited, visited_temp, &csrGraph);
                    hclib::finish([=]() {
                        bfss_ptr->start();
                        
                        for (int i = 0; i < local_vertices; i++) {

                            if (test_visitedloc(i)) {
                                continue;
                            }

                            for(int j=index[i]; j<index[i+1]; j++) {

                                if (test_visitedloc(i)) {
                                    break;
                                }

                                int local_vertex = csrGraph.local_to_vertex(i);
                                int dest_vertex = neighbors[j];
                                int dest = csrGraph.vertex_owner(dest_vertex);
                                bfsIterativeBottomUpVisitmsg m = {dest_vertex, local_vertex};
                                bfss_ptr->send(0, m, dest);
                            }
                        }

                        bfss_ptr->done(0);
                    });
                    lgp_barrier();
                    delete bfss_ptr;
                }
                it_num++;

                sum=nodes_in_level;
                aml_long_allsum(&sum);

                for(int i=0; i<local_vertices; i++){
                    if(test_visited_temp(i)){
                        set_visited(i);
                    }
                }
                clean_visited_temp();

                nodes_in_level=0;
            }
            lgp_barrier();

        }   

        //top down, bottom up
        std::vector<bool> visited;
        int64_t visited_size;

        std::vector<bool> visited_temp;
        int64_t visited_temp_size;

        //graph
        CSRGraph csrGraph;
        std::vector<int> index;
        std::vector<int64_t> neighbors;

        int local_vertices;
        BFSTypeEnum type;

        //BFS Specific
        int64_t source;
        int64_t *pred; //parent result
        BfsMetrics bfsMetrics;

        bool validation_passed;
};

#endif  // SingleBFSRun_H_
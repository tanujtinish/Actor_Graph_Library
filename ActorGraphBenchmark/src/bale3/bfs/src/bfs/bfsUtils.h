#ifndef BFS_UTILS_H_
#define BFS_UTILS_H_

#include "../csr_reference.h"

#include <vector>
#include <unordered_set>

std::unordered_set<int64_t> createBfsRoots(int num_bfs_roots, ResultFromBuilder builderResult){
    std::unordered_set<int64_t> bfs_roots;

    int64_t global_vertices = builderResult.global_vertices;

    int64_t seed = 27491094;
    std::mt19937 rng;
    rng.seed(seed);
    std::uniform_int_distribution<int> udist(0, 255);

    while(bfs_roots.size() < num_bfs_roots){
        
        while(1){
            int64_t root = static_cast<int64_t>(udist(rng));
            if(bfs_roots.find(root)!=bfs_roots.end()){
                continue;
            }
            
            std::vector<int> index = builderResult.csrGraph.getIndex();
            int root_bad = false;
            if(my_pe()==builderResult.csrGraph.vertex_owner(root)) {
                int v_local = builderResult.csrGraph.vertex_to_local(root);
                if( index[v_local] == index[v_local + 1]){
                    root_bad = true;
                }
            }
            lgp_barrier();
            MPI_Allreduce(MPI_IN_PLACE, &root_bad, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            if (root_bad) continue;

            bfs_roots.insert(root);
            break;
        }
    }

    return bfs_roots;
}


Stats getStatisticsFromVector(std::vector<double> x) {
	Stats stats;
    
    double temp;
    int n= x.size();

	/* Compute mean. */
	temp = 0.0;
	for (int i = 0; i < n; ++i) 
        temp += x[i];
	temp /= n;
	stats.s_mean = temp;
	double mean = temp;

	/* Compute std. dev. */
	temp = 0;
	for (int i = 0; i < n; ++i) 
        temp += (x[i] - mean) * (x[i] - mean);
	temp /= n - 1;
    stats.s_std = sqrt(temp);
    
	/* Sort x. */
	std::vector<double> xx = x;
	sort(xx.begin(), xx.end());

	/* Get order statistics. */
	stats.s_minimum = xx[0];
	stats.s_firstquartile = (xx[(n - 1) / 4] + xx[n / 4]) * .5;
	stats.s_median = (xx[(n - 1) / 2] + xx[n / 2]) * .5;
	stats.s_thirdquartile = (xx[n - 1 - (n - 1) / 4] + xx[n - 1 - n / 4]) * .5;
	stats.s_maximum = xx[n - 1];
	
    /* Clean up. */
	xx.clear();
}

#endif  // BFS_UTILS_H_
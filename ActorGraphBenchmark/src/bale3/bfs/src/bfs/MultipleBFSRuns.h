
#ifndef MultipleBFSRuns_H_
#define MultipleBFSRuns_H_

#include "../csr_reference.h"
#include "bfsUtils.h"
#include "BFSDataStructure.h"
#include "./enums/BFSTypeEnum.h"

class MultipleBFSRuns {

    public:
        MultipleBFSRuns(ResultFromBuilder builderResult, int64_t num_bfs_roots, BFSTypeEnum type){

            this->builderResult = builderResult;
            this->num_bfs_roots = num_bfs_roots;
            this->type = type;

            bfsMetricsMultipleRuns.bfs_run_times.resize(num_bfs_roots);
            bfsMetricsMultipleRuns.bfs_validate_times.resize(num_bfs_roots);
            bfsMetricsMultipleRuns.bfs_edge_counts.resize(num_bfs_roots);
            bfsMetricsMultipleRuns.bfs_secs_per_edges.resize(num_bfs_roots);

            bfs_roots = createBfsRoots(num_bfs_roots, builderResult);
        }

        Stats getBfsRunTimeStats(){
            return this->bfs_run_time_stats;
        }

        void printBfsRunTimeStats(){
            fprintf(stdout, "bfs  min_time:                  %g\n", this->bfs_run_time_stats.s_minimum);
            fprintf(stdout, "bfs  firstquartile_time:        %g\n", this->bfs_run_time_stats.s_firstquartile);
            fprintf(stdout, "bfs  median_time:               %g\n", this->bfs_run_time_stats.s_median);
            fprintf(stdout, "bfs  thirdquartile_time:        %g\n", this->bfs_run_time_stats.s_thirdquartile);
            fprintf(stdout, "bfs  max_time:                  %g\n", this->bfs_run_time_stats.s_maximum);
            fprintf(stdout, "bfs  mean_time:                 %g\n", this->bfs_run_time_stats.s_mean);
            fprintf(stdout, "bfs  stddev_time:               %g\n", this->bfs_run_time_stats.s_std);
        }

        Stats getBfsValidationTimeStats(){
            return this->bfs_validation_time_stats;
        }

        void printBfsValidationTimeStats(){
            fprintf(stdout, "bfs  min_validate:              %g\n", this->bfs_validation_time_stats.s_minimum);
            fprintf(stdout, "bfs  firstquartile_validate:    %g\n", this->bfs_validation_time_stats.s_firstquartile);
            fprintf(stdout, "bfs  median_validate:           %g\n", this->bfs_validation_time_stats.s_median);
            fprintf(stdout, "bfs  thirdquartile_validate:    %g\n", this->bfs_validation_time_stats.s_thirdquartile);
            fprintf(stdout, "bfs  max_validate:              %g\n", this->bfs_validation_time_stats.s_maximum);
            fprintf(stdout, "bfs  mean_validate:             %g\n", this->bfs_validation_time_stats.s_mean);
            fprintf(stdout, "bfs  stddev_validate:           %g\n", this->bfs_validation_time_stats.s_std);
        }

        Stats getBfsEdgeCountStats(){
            return this->bfs_edge_count_stats;
        }

        void printBfsEdgeCountStats(){
            fprintf(stdout, "min_nedge:                      %.11g\n", this->bfs_edge_count_stats.s_minimum);
            fprintf(stdout, "firstquartile_nedge:            %.11g\n", this->bfs_edge_count_stats.s_firstquartile);
            fprintf(stdout, "median_nedge:                   %.11g\n", this->bfs_edge_count_stats.s_median);
            fprintf(stdout, "thirdquartile_nedge:            %.11g\n", this->bfs_edge_count_stats.s_thirdquartile);
            fprintf(stdout, "max_nedge:                      %.11g\n", this->bfs_edge_count_stats.s_maximum);
            fprintf(stdout, "mean_nedge:                     %.11g\n", this->bfs_edge_count_stats.s_mean);
            fprintf(stdout, "stddev_nedge:                   %.11g\n", this->bfs_edge_count_stats.s_std);
        }

        Stats getBfsRunsSecsPerEdgeStats(){
            return this->bfs_runs_secs_per_edge_stats;
        }

        void printBfsRunsSecsPerEdgeStats(){
            fprintf(stdout, "bfs  min_TEPS:                  %g\n", 1. / this->bfs_runs_secs_per_edge_stats.s_maximum);
            fprintf(stdout, "bfs  firstquartile_TEPS:        %g\n", 1. / this->bfs_runs_secs_per_edge_stats.s_thirdquartile);
            fprintf(stdout, "bfs  median_TEPS:               %g\n", 1. / this->bfs_runs_secs_per_edge_stats.s_median);
            fprintf(stdout, "bfs  thirdquartile_TEPS:        %g\n", 1. / this->bfs_runs_secs_per_edge_stats.s_firstquartile);
            fprintf(stdout, "bfs  max_TEPS:                  %g\n", 1. / this->bfs_runs_secs_per_edge_stats.s_minimum);
            fprintf(stdout, "bfs  harmonic_mean_TEPS:     !  %g\n", 1. / this->bfs_runs_secs_per_edge_stats.s_mean);
            fprintf(stdout, "bfs  harmonic_stddev_TEPS:      %g\n", this->bfs_runs_secs_per_edge_stats.s_std / (this->bfs_runs_secs_per_edge_stats.s_mean * this->bfs_runs_secs_per_edge_stats.s_mean * sqrt(num_bfs_roots - 1)));

        }

        void run_multiple_bfs(){

            // bfs_roots = createBfsRoots();
            // for (int bfs_root_idx = 0; bfs_root_idx < num_bfs_roots; ++bfs_root_idx) {
			// 	int64_t root = bfs_roots[bfs_root_idx];

			// 	if (rank == 0) fprintf(stderr, "Running BFS %d\n", bfs_root_idx);

			// 	/* Do the actual BFS. */
			// 	SingleBFSRun bfs = SingleBFSRun(builderResult, &pred[0], root, type);
			// 	bfsMetricsMultipleRuns.bfs_run_times[bfs_root_idx] = bfs.getBfsRunTime();
			// 	if (rank == 0) fprintf(stderr, "Time for BFS %d is %f\n", bfs_root_idx, bfsMetricsMultipleRuns.bfs_run_times[bfs_root_idx]);
				
            //     int64_t edge_visit_count = bfs.getBfsEdgeCount();
			// 	bfsMetricsMultipleRuns.bfs_edge_counts[bfs_root_idx] = (double)edge_visit_count;
			// 	if (rank == 0) fprintf(stderr, "TEPS for BFS %d is %g\n", bfs_root_idx, bfsMetricsMultipleRuns.bfs_edge_counts[bfs_root_idx]);

            //     bfsMetricsMultipleRuns.bfs_secs_per_edges[bfs_root_idx] = bfsMetricsMultipleRuns.bfs_run_times[bfs_root_idx] / bfsMetricsMultipleRuns.bfs_edge_counts[bfs_root_idx];
            //     if (rank == 0) fprintf(stderr, "SECS Per Edge for BFS %d is %f\n", bfs_root_idx, bfsMetricsMultipleRuns.bfs_secs_per_edges[bfs_root_idx]);

			// 	/* Validate result. */
            //     int validation_passed = 1;
            //     bfsMetricsMultipleRuns.bfs_validate_times[bfs_root_idx] = bfs.getBfsValidationTime();
            //     if (rank == 0) fprintf(stderr, "Validate time for BFS %d is %f\n", bfs_root_idx, bfsMetricsMultipleRuns.bfs_validate_times[bfs_root_idx]);

            //     if (!bfs.getValidationPassed()) {
            //         validation_passed = 0;
            //         if (rank == 0) fprintf(stderr, "Validation failed for this BFS root; skipping rest.\n");
            //         break;
            //     }
				
			// }

            this->bfs_run_time_stats = getStatisticsFromVector(this->bfsMetricsMultipleRuns.bfs_run_times);
            this->bfs_validation_time_stats = getStatisticsFromVector(this->bfsMetricsMultipleRuns.bfs_validate_times);
            this->bfs_edge_count_stats = getStatisticsFromVector(this->bfsMetricsMultipleRuns.bfs_edge_counts);
            this->bfs_runs_secs_per_edge_stats = getStatisticsFromVector(this->bfsMetricsMultipleRuns.bfs_secs_per_edges);
		};

    private:  
        ResultFromBuilder builderResult;
        int64_t num_bfs_roots;
        BFSTypeEnum type;

        BfsMetricsMultipleRuns bfsMetricsMultipleRuns;
        std::unordered_set<int64_t> bfs_roots;

        Stats bfs_run_time_stats;
        Stats bfs_validation_time_stats;
        Stats bfs_edge_count_stats;
        Stats bfs_runs_secs_per_edge_stats;
};

#endif  // MultipleBFSRuns_H_
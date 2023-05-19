/* Copyright (C) 2010 The Trustees of Indiana University.                  */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */
/*           Anton Korzh                                                   */


/* These need to be before any possible inclusions of stdint.h or inttypes.h.
 * */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include "../generator/make_graph.h"
#include "../generator/utils.h"
#include "aml.h"
#include "common.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#include "./actor_graph_library/generator/generator.h"
#include "./actor_graph_library/builder/builder.h"
#include "./bfs/SingleBFSRun.h"
#include "./bfs/MultipleBFSRuns.h"

#include "selector.h"
#include "libgetput.h"
// #include "bfsUtils.h"

extern "C" {
    extern int isisolated(int64_t v);
}

static int compare_doubles(const void* a, const void* b) {
	double aa = *(const double*)a;
	double bb = *(const double*)b;
	return (aa < bb) ? -1 : (aa == bb) ? 0 : 1;
}

enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
void get_statistics(const double x[], int n, volatile double r[s_LAST]) {
	double temp;
	int i;
	/* Compute mean. */
	temp = 0.0;
	for (i = 0; i < n; ++i) temp += x[i];
	temp /= n;
	r[s_mean] = temp;
	double mean = temp;
	/* Compute std. dev. */
	temp = 0;
	for (i = 0; i < n; ++i) temp += (x[i] - mean) * (x[i] - mean);
	temp /= n - 1;
	r[s_std] = sqrt(temp);
	/* Sort x. */
	double* xx = (double*)xmalloc(n * sizeof(double));
	memcpy(xx, x, n * sizeof(double));
	qsort(xx, n, sizeof(double), compare_doubles);
	/* Get order statistics. */
	r[s_minimum] = xx[0];
	r[s_firstquartile] = (xx[(n - 1) / 4] + xx[n / 4]) * .5;
	r[s_median] = (xx[(n - 1) / 2] + xx[n / 2]) * .5;
	r[s_thirdquartile] = (xx[n - 1 - (n - 1) / 4] + xx[n - 1 - n / 4]) * .5;
	r[s_maximum] = xx[n - 1];
	/* Clean up. */
	free(xx);
}

int main(int argc, char** argv) {
	const char *deps[] = { "system", "bale_actor" };

        aml_init(&argc,&argv); //includes MPI_Init inside
        setup_globals();

	hclib::launch(deps, 2, [=, &argc,&argv] {

		//aml_barrier();

		/* Parse arguments. */
		int SCALE = 16;
		int edgefactor = 16; /* nedges / nvertices, i.e., 2*avg. degree */
		if (argc >= 2) SCALE = atoi(argv[1]);
		if (argc >= 3) edgefactor = atoi(argv[2]);
		if (argc <= 1 || argc >= 4 || SCALE == 0 || edgefactor == 0) {
			if (rank == 0) {
				fprintf(stderr, "Usage: %s SCALE edgefactor\n  SCALE = log_2(# vertices) [integer, required]\n  edgefactor = (# edges) / (# vertices) = .5 * (average vertex degree) [integer, defaults to 16]\n(Random number seed and Kronecker initiator are in main.c)\n", argv[0]);
			}
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		uint64_t seed1 = 2, seed2 = 3;
		
		printf("Rank: %d\n", rank);
		
		const char* filename = getenv("TMPFILE");
		

		/* Make the raw graph edges. */
		/* Get roots for BFS runs, plus maximum vertex with non-zero degree (used by
		* validator). */
		int num_bfs_roots = 64;
		int64_t* bfs_roots = (int64_t*)xmalloc(num_bfs_roots * sizeof(int64_t));
		
		//RMATGraph(tg, nglobalverts, SCALE, seed1, seed2);
		Generator* generator = new Generator(edgefactor, SCALE);
		ResultFromGenerator generatorResult = generator->GenerateEL(RmatGenerator);

		tuple_graph tg = generator->getTG(generatorResult);
		int64_t nglobalverts = (int64_t)(1) << generatorResult.SCALE;

		if (rank == 0) { /* Not an official part of the results */
			fprintf(stderr, "graph_generation:               %f s\n", generatorResult.make_graph_time);
		}

		/* Make user's graph data structure. */
		BuilderBase* builder = new BuilderBase(generatorResult, Cyclic);
		ResultFromBuilder builderResult = builder->buildCSRGraph();
		make_graph_data_structure(&tg);
		if (rank == 0) { /* Not an official part of the results */
			fprintf(stderr, "construction_time:              %f s\n", builderResult.build_graph_time);
		}
		if (rank == 0) { /* Not an official part of the results */
			builderResult.printShufflingEfficiencyStats();
		}
		
		//generate non-isolated roots
		{
			uint64_t counter = 0;
			int bfs_root_idx;
			for (bfs_root_idx = 0; bfs_root_idx < num_bfs_roots; ++bfs_root_idx) {
				int64_t root;
				while (1) {
					double d[2];
					make_random_numbers(2, seed1, seed2, counter, d);
					root = (int64_t)((d[0] + d[1]) * nglobalverts) % nglobalverts;
					counter += 2;
					if (counter > 2 * nglobalverts) break;
					int is_duplicate = 0;
					int i;
					for (i = 0; i < bfs_root_idx; ++i) {
						if (root == bfs_roots[i]) {
							is_duplicate = 1;
							break;
						}
					}
					if (is_duplicate) continue; /* Everyone takes the same path here */
					int root_bad = isisolated(root);
					MPI_Allreduce(MPI_IN_PLACE, &root_bad, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
					if (!root_bad) break;
				}
				bfs_roots[bfs_root_idx] = root;
			}
			num_bfs_roots = bfs_root_idx;


		}
		/* Number of edges visited in each BFS; a double so get_statistics can be
		* used directly. */
		double* edge_counts = (double*)xmalloc(num_bfs_roots * sizeof(double));

		/* Run BFS. */
		int validation_passed = 1;
		double* bfs_times = (double*)xmalloc(num_bfs_roots * sizeof(double));
		double* validate_times = (double*)xmalloc(num_bfs_roots * sizeof(double));
		uint64_t nlocalverts = builderResult.csrGraph.getLocalVertices();
		int64_t* pred = (int64_t*)xMPI_Alloc_mem(nlocalverts * sizeof(int64_t));
		float* shortest = (float*)xMPI_Alloc_mem(nlocalverts * sizeof(float));

		// MultipleBFSRuns multipleBFSRuns = MultipleBFSRuns(builderResult, 5, TopDownIterativeBFS);

		if (!getenv("SKIP_BFS")) {

			int bfs_root_idx,i;
			SingleBFSRun bfs = SingleBFSRun(builderResult, &pred[0], bfs_roots[0], TopDownIterativeBFS);
			if (!getenv("SKIP_VALIDATION")) {
				int64_t nedges=0;
				validate_result(1,&tg, nlocalverts, bfs_roots[0], pred,shortest,NULL);
			}

			for (bfs_root_idx = 0; bfs_root_idx < 5; ++bfs_root_idx) {
				int64_t root = bfs_roots[bfs_root_idx];

				if (rank == 0) fprintf(stderr, "Running BFS %d\n", bfs_root_idx);

				/* Do the actual BFS. */
				bfs = SingleBFSRun(builderResult, &pred[0], root, 2);
				bfs_times[bfs_root_idx] = bfs.getBfsRunTime();
				if (rank == 0) fprintf(stderr, "Time for BFS %d is %f\n", bfs_root_idx, bfs_times[bfs_root_idx]);
				int64_t edge_visit_count = bfs.getBfsEdgeCount();
				edge_counts[bfs_root_idx] = (double)edge_visit_count;
				if (rank == 0) fprintf(stderr, "TEPS for BFS %d is %g\n", bfs_root_idx, edge_visit_count / bfs_times[bfs_root_idx]);

				/* Validate result. */
				if (!getenv("SKIP_VALIDATION")) {
					if (rank == 0) fprintf(stderr, "Validating BFS %d\n", bfs_root_idx);

					double validate_start = MPI_Wtime();
					int validation_passed_one = validate_result(1,&tg, nlocalverts, root, pred,shortest,&edge_visit_count);
					double validate_stop = MPI_Wtime();

					validate_times[bfs_root_idx] = validate_stop - validate_start;
					if (rank == 0) fprintf(stderr, "Validate time for BFS %d is %f\n", bfs_root_idx, validate_times[bfs_root_idx]);

					if (!validation_passed_one) {
						validation_passed = 0;
						if (rank == 0) fprintf(stderr, "Validation failed for this BFS root; skipping rest.\n");
						break;
					}
				} else
					validate_times[bfs_root_idx] = -1;
			}

		}
		MPI_Free_mem(pred);
		free(bfs_roots);
		free_graph_data_structure();

		free(tg.edgememory); tg.edgememory = NULL;
	

		/* Print results. */
		if (rank == 0) {
			if (!validation_passed) {
				fprintf(stdout, "No results printed for invalid run.\n");
			} else {
				int i;
				//for (i = 0; i < num_bfs_roots; ++i) printf(" %g \n",edge_counts[i]);
				fprintf(stdout, "SCALE:                          %d\n", SCALE);
				fprintf(stdout, "edgefactor:                     %d\n", edgefactor);
				fprintf(stdout, "NBFS:                           %d\n", num_bfs_roots);
				fprintf(stdout, "graph_generation:               %g\n", make_graph_time);
				fprintf(stdout, "num_mpi_processes:              %d\n", size);
				fprintf(stdout, "construction_time:              %g\n", data_struct_time);
				volatile double stats[s_LAST];
				get_statistics(bfs_times, num_bfs_roots, stats);
				fprintf(stdout, "bfs  min_time:                  %g\n", stats[s_minimum]);
				fprintf(stdout, "bfs  firstquartile_time:        %g\n", stats[s_firstquartile]);
				fprintf(stdout, "bfs  median_time:               %g\n", stats[s_median]);
				fprintf(stdout, "bfs  thirdquartile_time:        %g\n", stats[s_thirdquartile]);
				fprintf(stdout, "bfs  max_time:                  %g\n", stats[s_maximum]);
				fprintf(stdout, "bfs  mean_time:                 %g\n", stats[s_mean]);
				fprintf(stdout, "bfs  stddev_time:               %g\n", stats[s_std]);

				get_statistics(edge_counts, num_bfs_roots, stats);
				fprintf(stdout, "min_nedge:                      %.11g\n", stats[s_minimum]);
				fprintf(stdout, "firstquartile_nedge:            %.11g\n", stats[s_firstquartile]);
				fprintf(stdout, "median_nedge:                   %.11g\n", stats[s_median]);
				fprintf(stdout, "thirdquartile_nedge:            %.11g\n", stats[s_thirdquartile]);
				fprintf(stdout, "max_nedge:                      %.11g\n", stats[s_maximum]);
				fprintf(stdout, "mean_nedge:                     %.11g\n", stats[s_mean]);
				fprintf(stdout, "stddev_nedge:                   %.11g\n", stats[s_std]);
				double* secs_per_edge = (double*)xmalloc(num_bfs_roots * sizeof(double));
				for (i = 0; i < num_bfs_roots; ++i) secs_per_edge[i] = bfs_times[i] / edge_counts[i];
				get_statistics(secs_per_edge, num_bfs_roots, stats);
				fprintf(stdout, "bfs  min_TEPS:                  %g\n", 1. / stats[s_maximum]);
				fprintf(stdout, "bfs  firstquartile_TEPS:        %g\n", 1. / stats[s_thirdquartile]);
				fprintf(stdout, "bfs  median_TEPS:               %g\n", 1. / stats[s_median]);
				fprintf(stdout, "bfs  thirdquartile_TEPS:        %g\n", 1. / stats[s_firstquartile]);
				fprintf(stdout, "bfs  max_TEPS:                  %g\n", 1. / stats[s_minimum]);
				fprintf(stdout, "bfs  harmonic_mean_TEPS:     !  %g\n", 1. / stats[s_mean]);
				fprintf(stdout, "bfs  harmonic_stddev_TEPS:      %g\n", stats[s_std] / (stats[s_mean] * stats[s_mean] * sqrt(num_bfs_roots - 1)));

				free(secs_per_edge); secs_per_edge = NULL;
				free(edge_counts); edge_counts = NULL;
				get_statistics(validate_times, num_bfs_roots, stats);
				fprintf(stdout, "bfs  min_validate:              %g\n", stats[s_minimum]);
				fprintf(stdout, "bfs  firstquartile_validate:    %g\n", stats[s_firstquartile]);
				fprintf(stdout, "bfs  median_validate:           %g\n", stats[s_median]);
				fprintf(stdout, "bfs  thirdquartile_validate:    %g\n", stats[s_thirdquartile]);
				fprintf(stdout, "bfs  max_validate:              %g\n", stats[s_maximum]);
				fprintf(stdout, "bfs  mean_validate:             %g\n", stats[s_mean]);
				fprintf(stdout, "bfs  stddev_validate:           %g\n", stats[s_std]);
	

			}
		}
		free(bfs_times);
		free(validate_times);

		aml_barrier();
		cleanup_globals();

	});

	return 0;
}
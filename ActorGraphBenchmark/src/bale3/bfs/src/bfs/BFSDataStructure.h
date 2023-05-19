#ifndef BFS_DATA_SRUCTURES_H_
#define BFS_DATA_SRUCTURES_H_

typedef struct BfsMetrics{
    double bfs_run_time;
    double bfs_validate_time;
    double bfs_edge_count;
    double bfs_secs_per_edge;
} BfsMetrics;

typedef struct BfsMetricsMultipleRuns{
    std::vector<double> bfs_run_times;
    std::vector<double> bfs_validate_times;
    std::vector<double> bfs_edge_counts;
    std::vector<double> bfs_secs_per_edges;
} BfsMetricsMultipleRuns;

typedef struct Stats{
    double s_minimum;
    double s_firstquartile;
    double s_median;
    double s_thirdquartile;
    double s_maximum;
    double s_mean;
    double s_std;
    double s_LAST;
} Stats;

#endif  // BFS_DATA_SRUCTURES_H_
#include "../../../common.h"

#include "libgetput.h"
#include "selector.h"

#include "../EdgeListDataStructure.h"

#include <inttypes.h>
#include <random>

typedef struct rmatGeneratorSelectorVisitmsg {
	
} rmatGeneratorSelectorVisitmsg;

class RmatGeneratorSelector: public hclib::Selector<1, rmatGeneratorSelectorVisitmsg> {
    
    int total_pe;
	int SCALE;
    int edgefactor;
    int64_t global_vertices;
    int64_t global_edges;
	int64_t local_edges;

    packed_edge* restrict edgememory; /* NULL if edges are in file */
    int64_t edgememory_size;
    int64_t max_edgememory_size;
    float* weightmemory;

    EdgeList edgeList;
    WEdgeList wEdgeList;
    int64_t edgelist_size;
    int64_t max_edgelist_size;

	static const int64_t kRandSeed = 27491095;

	ResultFromGenerator* generatorResult;

	void saveBackResult() {
        generatorResult->total_pe = total_pe;
        generatorResult->SCALE = SCALE;
        generatorResult->edgefactor = edgefactor;
        generatorResult->local_edges = local_edges;
        generatorResult->global_vertices = global_vertices;
        generatorResult->global_edges = global_edges;

        generatorResult->edgememory = edgememory;
        generatorResult->edgememory_size = edgememory_size;
        generatorResult->max_edgememory_size = max_edgememory_size;
        generatorResult->weightmemory = weightmemory;

        std::vector<Edge>::iterator it1;
        generatorResult->edgeList.clear();
        for(it1=edgeList.begin(); it1!=edgeList.end(); it1++){
            (generatorResult->edgeList).push_back(*it1);
        }

        std::vector<WEdge>::iterator it2;
        generatorResult->wEdgeList.clear();
        for(it2=wEdgeList.begin(); it2!=wEdgeList.end(); it2++){
            (generatorResult->wEdgeList).push_back(*it2);
        }

        generatorResult->edgelist_size = edgelist_size;
        generatorResult->max_edgelist_size = max_edgelist_size;
    }

    // void process(rmatGeneratorSelectorVisitmsg m, int sender_rank) {
    //     uint64_t seed1 = 2, seed2 = 3;

	// 	/* Spread the two 64-bit numbers into five nonzero values in the correct
	// 	* range. */
	// 	uint_fast32_t seed[5];
	// 	make_mrg_seed(seed1, seed2, seed);

	// 	/* As the graph is being generated, also keep a bitmap of vertices with
	// 	* incident edges.  We keep a grid of processes, each row of which has a
	// 	* separate copy of the bitmap (distributed among the processes in the
	// 	* row), and then do an allreduce at the end.  This scheme is used to avoid
	// 	* non-local communication and reading the file separately just to find BFS
	// 	* roots. */
	// 	MPI_Offset nchunks_in_file = (global_edges + FILE_CHUNKSIZE - 1) / FILE_CHUNKSIZE;
	// 	int64_t bitmap_size_in_bytes = int64_min(BITMAPSIZE, (global_vertices + CHAR_BIT - 1) / CHAR_BIT);
	// 	if (bitmap_size_in_bytes * size * CHAR_BIT < global_vertices) {
	// 		bitmap_size_in_bytes = (global_vertices + size * CHAR_BIT - 1) / (size * CHAR_BIT);
	// 	}
	// 	int ranks_per_row = ((global_vertices + CHAR_BIT - 1) / CHAR_BIT + bitmap_size_in_bytes - 1) / bitmap_size_in_bytes;
	// 	int nrows = size / ranks_per_row;
	// 	int my_row = -1, my_col = -1;

	// 	printf("Size: %d\n", size);
	// 	printf("Number of vertices: %d\n", global_vertices);
	// 	printf("Bitmap Size (Bytes): %d\n", bitmap_size_in_bytes);
	// 	printf("Ranks/Row: %d\n", ranks_per_row);
	// 	printf("Number of rows: %d\n", nrows);

	// 	MPI_Comm cart_comm;
	// 	{
	// 		int dims[2] = {size / ranks_per_row, ranks_per_row};
	// 		int periods[2] = {0, 0};
	// 		MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);
	// 	}
	// 	int in_generating_rectangle = 0;
	// 	if (cart_comm != MPI_COMM_NULL) {
	// 		in_generating_rectangle = 1;
	// 		{
	// 			int dims[2], periods[2], coords[2];
	// 			MPI_Cart_get(cart_comm, 2, dims, periods, coords);
	// 			my_row = coords[0];
	// 			my_col = coords[1];
	// 		}
	// 		MPI_Comm this_col;
	// 		MPI_Comm_split(cart_comm, my_col, my_row, &this_col);
	// 		MPI_Comm_free(&cart_comm);
	// 		/* Every rank in a given row creates the same vertices (for updating the
	// 		* bitmap); only one writes them to the file (or final memory buffer). */
	// 		packed_edge* buf = (packed_edge*)xmalloc(FILE_CHUNKSIZE * sizeof(packed_edge));
			
	// 		MPI_Offset block_limit = (nchunks_in_file + nrows - 1) / nrows;
	// 		/* fprintf(stderr, "%d: nchunks_in_file = %" PRId64 ", block_limit = %" PRId64 " in grid of %d rows, %d cols\n", rank, (int64_t)nchunks_in_file, (int64_t)block_limit, nrows, ranks_per_row); */
			
	// 		int my_pos = my_row + my_col * nrows;
	// 		int last_pos = (global_edges % ((int64_t)FILE_CHUNKSIZE * nrows * ranks_per_row) != 0) ?
	// 			(global_edges / FILE_CHUNKSIZE) % (nrows * ranks_per_row) :
	// 			-1;
	// 		int64_t edges_left = global_edges % FILE_CHUNKSIZE;
	// 		int64_t nedges = FILE_CHUNKSIZE * (global_edges / ((int64_t)FILE_CHUNKSIZE * nrows * ranks_per_row)) +
	// 			FILE_CHUNKSIZE * (my_pos < (global_edges / FILE_CHUNKSIZE) % (nrows * ranks_per_row)) +
	// 			(my_pos == last_pos ? edges_left : 0);
	// 		/* fprintf(stderr, "%d: nedges = %" PRId64 " of %" PRId64 "\n", rank, (int64_t)nedges, (int64_t)tg.global_edges); */
	// 		edgememory_size = nedges;
	// 		edgememory = (packed_edge*)xmalloc(nedges * sizeof(packed_edge));

	// 		edgeList.resize(nedges);
	// 		edgelist_size = nedges;

	// 		MPI_Offset block_idx;
	// 		for (block_idx = 0; block_idx < block_limit; ++block_idx) {
	// 			/* fprintf(stderr, "%d: On block %d of %d\n", rank, (int)block_idx, (int)block_limit); */
	// 			MPI_Offset start_edge_index = int64_min(FILE_CHUNKSIZE * (block_idx * nrows + my_row), global_edges);
	// 			MPI_Offset edge_count = int64_min(global_edges - start_edge_index, FILE_CHUNKSIZE);
	// 			packed_edge* actual_buf = (block_idx % ranks_per_row == my_col) ?
	// 				edgememory + FILE_CHUNKSIZE * (block_idx / ranks_per_row) :
	// 				buf;

	// 			/* fprintf(stderr, "%d: My range is [%" PRId64 ", %" PRId64 ") %swriting into index %" PRId64 "\n", rank, (int64_t)start_edge_index, (int64_t)(start_edge_index + edge_count), (my_col == (block_idx % ranks_per_row)) ? "" : "not ", (int64_t)(FILE_CHUNKSIZE * (block_idx / ranks_per_row))); */
	// 			if (block_idx % ranks_per_row == my_col) {
	// 				assert (FILE_CHUNKSIZE * (block_idx / ranks_per_row) + edge_count <= edgememory_size);
	// 			}
				
	// 			if(!addWeights)
	// 				generate_kronecker_range(seed, SCALE, start_edge_index, start_edge_index + edge_count, actual_buf);
	// 			else
	// 			{
	// 				wEdgeList.resize(nedges);
	// 				float* wbuf = (float*)xmalloc(FILE_CHUNKSIZE*sizeof(float));
	// 				weightmemory = (float*)xmalloc(nedges*sizeof(float));
	// 				float* actual_wbuf = (block_idx % ranks_per_row == my_col) ?
	// 					weightmemory + FILE_CHUNKSIZE * (block_idx / ranks_per_row) :
	// 					wbuf;
	// 				generate_kronecker_range(seed, SCALE, start_edge_index, start_edge_index + edge_count, actual_buf);
	// 				// generate_kronecker_range(seed, SCALE, start_edge_index, start_edge_index + edge_count, actual_buf, actual_wbuf);
	// 			}
				
	// 		}
	// 		free(buf);
	// 		MPI_Comm_free(&this_col);
	// 	} else {
	// 		edgememory = NULL;
	// 		edgememory_size = 0;
	// 		if(addWeights)
	// 			weightmemory = NULL;
	// 	}
	// 	MPI_Allreduce(&edgememory_size, &max_edgememory_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);
	// 	MPI_Allreduce(&edgelist_size, &max_edgelist_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);

	// 	saveBackResult();
    // }

	void process(rmatGeneratorSelectorVisitmsg m, int sender_rank){
		const float A = 0.57f, B = 0.19f, C = 0.19f;
		
		total_pe = size;
        int64_t nedges_per_pe = global_edges / total_pe;
        int64_t extraedges = global_edges % total_pe;
        int64_t my_pe = rank;
        if(my_pe < extraedges){
            local_edges = nedges_per_pe + 1;
        }else{
            local_edges = nedges_per_pe;
        }
        //packed_edge* buf = (packed_edge*)xmalloc(FILE_CHUNKSIZE * sizeof(packedtg.edgememory_size = nedges;
        edgememory_size = local_edges;
        edgememory = (packed_edge*)xmalloc(local_edges * sizeof(packed_edge));

        edgeList.resize(local_edges);
        edgelist_size = local_edges;

        // srand(time(NULL));
		std::mt19937 rng;
		std::uniform_real_distribution<float> udist(0, 1.0f);
		rng.seed(kRandSeed + rank);
        for(int m = 0; m < local_edges; m++){
			
			int64_t src = 0, dst = 0;
			for (int depth=0; depth < SCALE; depth++) {
				float rand_point = udist(rng);
				src = src << 1;
				dst = dst << 1;
				if (rand_point < A+B) {
					if (rand_point > A)
						dst++;
				} else {
					src++;
					if (rand_point > A+B+C)
						dst++;
				}
			}

			// printf("Edge %d -> %d generated by rank: %d\n", src, dst, rank);
            packed_edge e;
			Edge(src, dst);
            write_edge(&e, src, dst);
            edgememory[m] = e;

            edgeList[m] = Edge(src, dst);
			
		}
		
		MPI_Allreduce(&edgememory_size, &max_edgememory_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&edgelist_size, &max_edgelist_size, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);

        saveBackResult();
	}

public:
RmatGeneratorSelector(int edgefactor, int SCALE, ResultFromGenerator* generatorResult){

        this->SCALE = SCALE; 
        this->edgefactor = edgefactor; 
        this->global_vertices = (int64_t)(1) << SCALE; 
        this->global_edges = (int64_t)(edgefactor) << SCALE;

		this->generatorResult = generatorResult;

        mb[0].process = [this](rmatGeneratorSelectorVisitmsg pkt, int sender_rank) { this->process(pkt, sender_rank); };
    }
};
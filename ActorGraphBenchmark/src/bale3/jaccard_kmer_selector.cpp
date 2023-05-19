/******************************************************************
//
// 
 *****************************************************************/ 
/*! \file jaccard_kmer_selector.cpp
 * \brief Demo application that calculates jaccard similarity for k-mer sequence graph 
 *
 *      NOTE: this program is created for mxn k-mer matrices
 */

#include <math.h>
#include <shmem.h>
extern "C" {
#include "spmat.h"
}
#include "selector.h"

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

enum MailBoxType {RESPONSE, REQUEST};

typedef struct JaccardPkt {
    int64_t x;
    int64_t pos_row;
    int64_t pos_col;
    int64_t index_u;
} JaccardPkt;

typedef struct DegreePkt {
    int64_t i;
    int64_t src_idx;
} DegreePkt;

class JaccardSelector: public hclib::Selector<2, JaccardPkt> {
public:
    JaccardSelector(sparsemat_t* mat, sparsemat_t* intersection_mat) : mat_(mat), intersection_mat_(intersection_mat) {
        mb[REQUEST].process = [this] (JaccardPkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
        mb[RESPONSE].process = [this] (JaccardPkt pkt, int sender_rank) { 
            this->resp_process(pkt, sender_rank);
        };
    }

private:
    //shared variables
    sparsemat_t* mat_;
    sparsemat_t* intersection_mat_;

    void req_process(JaccardPkt pkg, int sender_rank) {
        JaccardPkt pkg2;
        for (int64_t uk = mat_->loffset[pkg.index_u]; uk < mat_->loffset[pkg.index_u+1]; uk++) {
            if (pkg.x == mat_->lnonzero[uk]) {
                pkg2.pos_row = pkg.pos_row;
                pkg2.pos_col = pkg.pos_col;
                send(RESPONSE, pkg2, pkg.pos_row % THREADS);
            }
        }
    }

    void resp_process(JaccardPkt pkg, int sender_rank) {
        int pos = 0;
        pkg.pos_row = pkg.pos_row / THREADS;
        for (int i = intersection_mat_->loffset[pkg.pos_row]; i < intersection_mat_->loffset[pkg.pos_row + 1]; i++) {
            if (pos == pkg.pos_col) {
                intersection_mat_->lvalue[i]++;
            }
            pos++;
        }
    }

};

class DegreeSelector: public hclib::Selector<2, DegreePkt> {
public:
    DegreeSelector(sparsemat_t* mat, sparsemat_t* kmer_mat) : mat_(mat), kmer_mat_(kmer_mat) {
        mb[REQUEST].process = [this] (DegreePkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
        mb[RESPONSE].process = [this] (DegreePkt pkt, int sender_rank) { 
            this->resp_process(pkt, sender_rank);
        };
    }

private:
    //shared variables
    sparsemat_t* mat_;
    sparsemat_t* kmer_mat_;

    void req_process(DegreePkt pkg, int sender_rank) {
        DegreePkt pkg2;
        pkg2.src_idx = pkg.src_idx;
        pkg2.i = kmer_mat_->loffset[pkg.i + 1] - kmer_mat_->loffset[pkg.i];
        send(RESPONSE, pkg2, sender_rank);
    }

    void resp_process(DegreePkt pkg, int sender_rank) {
        mat_->lnonzero[pkg.src_idx] = pkg.i;
    }

};

double get_edge_degrees(sparsemat_t* L, sparsemat_t* A2) {
    DegreeSelector* degreeSelector = new DegreeSelector(L, A2);

    hclib::finish([=]() {
        degreeSelector->start();
        DegreePkt degPKG;
    
        for (int64_t y = 0; y < L->lnumrows; y++) {
            for (int64_t k = L->loffset[y]; k < L->loffset[y+1]; k++) {
                degPKG.i = L->lnonzero[k] / THREADS;
                degPKG.src_idx = k;
                degreeSelector->send(REQUEST, degPKG, L->lnonzero[k] % THREADS);
            }
        }

        degreeSelector->done(REQUEST);
    });

    return 0;
}

double jaccard_selector(sparsemat_t* Jaccard_mat, sparsemat_t* A2) {
    // Start timing
    double t1 = wall_seconds();
    lgp_barrier();

    JaccardSelector* jacSelector = new JaccardSelector(A2, Jaccard_mat);

    get_edge_degrees(Jaccard_mat, A2);
    lgp_barrier();

    hclib::finish([=]() {
        jacSelector->start();

        JaccardPkt pkg;

        for (int64_t v = 0; v < A2->lnumrows; v++) {             //vertex v (local)
            for (int64_t k = A2->loffset[v]; k < A2->loffset[v+1]; k++) {
                int64_t v_nonzero = A2->lnonzero[k];                     //vertex u (possibly remote)
                int64_t row_num = v * THREADS + MYTHREAD;

                for (int64_t i_rows = row_num; i_rows < A2->numrows; i_rows++) {
                    // calculate intersection
                    pkg.index_u = i_rows / THREADS;
                    pkg.x = v_nonzero;
                    pkg.pos_row = i_rows;
                    pkg.pos_col = row_num;
                    jacSelector->send(REQUEST, pkg, i_rows % THREADS);
                }
            }
        }
        jacSelector->done(REQUEST);
        
    });

    lgp_barrier();

    for (int64_t v = 0; v < Jaccard_mat->lnumrows; v++) {
        int64_t deg_v = A2->loffset[v+1] - A2->loffset[v];
        for (int64_t k = Jaccard_mat->loffset[v]; k < Jaccard_mat->loffset[v+1]; k++) {
            Jaccard_mat->lvalue[k] = (double)Jaccard_mat->lvalue[k] / ((double)Jaccard_mat->lnonzero[k] + (double)deg_v - (double)Jaccard_mat->lvalue[k]);
        }
    }

    lgp_barrier();
    t1 = wall_seconds() - t1;
    return t1;
}


int main(int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {

        int64_t models_mask = 0;//ALL_Models;  // default is running all models
        int64_t l_numrows = 10000;         // number of a rows per thread
        int64_t nz_per_row = 35;           // target number of nonzeros per row (only for Erdos-Renyi)
        int64_t read_graph = 0L;           // read graph from a file
        char filename[64];
        
        double t1;
        int64_t i, j;
        int64_t alg = 0;
        int64_t gen_kron_graph = 0L;
        int kron_graph_mode = 0;
        char * kron_graph_string;
        double erdos_renyi_prob = 0.0;

        int printhelp = 0;
        int opt; 
        while ((opt = getopt(argc, argv, "hM:n:f:a:e:K:")) != -1) {
            switch (opt) {
                case 'h': printhelp = 1; break;
                case 'M': sscanf(optarg,"%ld", &models_mask);  break;
                case 'n': sscanf(optarg,"%ld", &l_numrows); break;
                case 'f': read_graph = 1; sscanf(optarg,"%s", filename); break;

                case 'a': sscanf(optarg,"%ld", &alg); break;
                case 'e': sscanf(optarg,"%lg", &erdos_renyi_prob); break;
                case 'K': gen_kron_graph = 1; kron_graph_string = optarg; break;
                default:  break;
            }
        }

        // if (printhelp) usage(); // Skipping print help
        int64_t numrows = l_numrows * THREADS;
        if (erdos_renyi_prob == 0.0) { // use nz_per_row to get erdos_renyi_prob
            erdos_renyi_prob = (2.0 * (nz_per_row - 1)) / numrows;
            if (erdos_renyi_prob > 1.0) erdos_renyi_prob = 1.0;
        } else {                     // use erdos_renyi_prob to get nz_per_row
            nz_per_row = erdos_renyi_prob * numrows;
        }

        T0_fprintf(stderr,"Running jaccard on %d threads\n", THREADS);
        if (!read_graph && !gen_kron_graph) {
            T0_fprintf(stderr,"Number of rows per thread   (-N)   %ld\n", l_numrows);
            T0_fprintf(stderr,"Erdos Renyi prob (-e)   %g\n", erdos_renyi_prob);
        }

        T0_fprintf(stderr,"Model mask (M) = %ld (should be 1,2,4,8,16 for agi, exstack, exstack2, conveyors, alternates\n", models_mask);  
        T0_fprintf(stderr,"algorithm (a) = %ld (0 for L & L*U, 1 for L & U*L)\n", alg);

        double correct_answer = -1;

        sparsemat_t *A, *L, *U;
/*
        if (read_graph) {
            A = read_matrix_mm_to_dist(filename);
            if (!A) assert(false);
            
            T0_fprintf(stderr,"Reading file %s...\n", filename);
            T0_fprintf(stderr, "A has %ld rows/cols and %ld nonzeros.\n", A->numrows, A->nnz);

            // we should check that A is symmetric!

            if (!is_lower_triangular(A, 0)) { //if A is not lower triangular... make it so.      
                T0_fprintf(stderr, "Assuming symmetric matrix... using lower-triangular portion...\n");
                tril(A, -1);
                L = A;
            } else {
                L = A;
            }

            sort_nonzeros(L);

        } else if (gen_kron_graph) {
            // string should be <mode> # # ... #
            // we will break the string of numbers (#s) into two groups and create
            // two local kronecker graphs out of them.
            int num;
            char* ptr = kron_graph_string;
            int64_t* kron_specs = (int64_t*)calloc(32, sizeof(int64_t *));

            // read the mode
            int ret = sscanf(ptr, "%d ", &kron_graph_mode);
            if (ret == 0) ret = sscanf(ptr, "\"%d ", &kron_graph_mode);
            if (ret == 0) { T0_fprintf(stderr, "ERROR reading kron graph string!\n"); assert(false); }
            T0_fprintf(stderr,"kron string: %s return = %d\n", ptr, ret);
            T0_fprintf(stderr,"kron mode: %d\n", kron_graph_mode);
            ptr += 2;
            int mat, num_ints = 0;
            while (sscanf(ptr, "%d %n", &num, &mat) == 1) {
                T0_fprintf(stderr,"%s %d\n", ptr, mat);
                kron_specs[num_ints++] = num;
                ptr+=mat;
            }

            if (num_ints <= 1) {
                T0_fprintf(stderr, "ERROR: invalid kronecker product string (%s): must contain at least three integers\n", kron_graph_string); 
                assert(false);
            }

            // calculate the number of triangles 
            if (kron_graph_mode == 0) {
                correct_answer = 0.0;
            } else if (kron_graph_mode == 1) {
                correct_answer = 1;
                for (i = 0; i < num_ints; i++)
                    correct_answer *= (3 * kron_specs[i] + 1);
        
                correct_answer *= 1.0 / 6.0;
                double x = 1;
                for (i = 0; i < num_ints; i++) {
                    x *= (kron_specs[i] + 1);
                }

                correct_answer = correct_answer - 0.5 * x + 1.0 / 3.0;
            } else if (kron_graph_mode == 2) {
                correct_answer = (1.0 / 6.0) * pow(4, num_ints) - pow(2.0, (num_ints - 1)) + 1.0 / 3.0;
            }

            correct_answer = round(correct_answer);
            T0_fprintf(stderr, "Pre-calculated answer = %ld\n", (int64_t)correct_answer);

            int64_t half = num_ints / 2;

            L = generate_kronecker_graph(kron_specs, half, &kron_specs[half], num_ints - half, kron_graph_mode);
        } else {

            int n = numrows;
            L = erdos_renyi_random_graph(numrows, 1, UNDIRECTED, NOLOOPS, 12345);
            U = transpose_matrix(L);
            if(!U){T0_printf("ERROR: gen_er_graph_dist: U is NULL!\n"); lgp_global_exit(1);}
            int64_t lnnz = L->lnnz + U->lnnz;
            sparsemat_t * A2 = init_matrix(n, n, lnnz, 0);
            if(!A2){T0_printf("ERROR: gen_er_graph_dist: A2 is NULL!\n"); lgp_global_exit(1);}

            A2->loffset[0] = 0;
            lnnz = 0;
            for(i = 0; i < L->lnumrows; i++){
                int64_t global_row = i*THREADS + MYTHREAD;
                for(j = L->loffset[i]; j < L->loffset[i+1]; j++)
                    A2->lnonzero[lnnz++] = L->lnonzero[j];
                for(j = U->loffset[i]; j < U->loffset[i+1]; j++){
                    if(U->lnonzero[j] != global_row) // don't copy the diagonal element twice!
                        A2->lnonzero[lnnz++] = U->lnonzero[j];
                }
                A2->loffset[i+1] = lnnz;
            }
            lgp_barrier();
            clear_matrix(U);
            clear_matrix(L);
            free(U); free(L);
            // A2 is a full symmetric adjacency matrix
        //}
*/
        lgp_barrier();

        // READ/CREATE k-mer matrix
        sparsemat_t* A2 = read_matrix_mm_to_dist("jaccard_kmer_15000x5000_matrix.mtx");//("jaccard_kmer_matrix.mtx");
        A2 = transpose_matrix(A2);
        lgp_barrier();

        // CREATE jaccard similarity matrix
        sparsemat_t* Jaccard_mat = random_graph(5000, FLAT, DIRECTED_WEIGHTED, LOOPS, 1, 12345); if (MYTHREAD==1) Jaccard_mat->lnonzero[0] = 0; tril(Jaccard_mat, -1); for (int r = 0; r < Jaccard_mat->lnumrows; r++) {for (int rr = Jaccard_mat->loffset[r]; rr < Jaccard_mat->loffset[r+1]; rr++) {Jaccard_mat->lvalue[rr] = 0.0;}}
        //sparsemat_t* Jaccard_mat = read_matrix_mm_to_dist("jaccard_kmer_1000x1000_jaccard_matrix.mtx"); for (int r = 0; r < Jaccard_mat->lnumrows; r++) {for (int rr = Jaccard_mat->loffset[r]; rr < Jaccard_mat->loffset[r+1]; rr++) {Jaccard_mat->lvalue[rr] = 0.0;}}
        //print_matrix(Jaccard_mat);
        lgp_barrier();

        T0_fprintf(stderr, "K-mer Matrix is %ldx%ld and has %ld nonzeros.\n", A2->numcols, A2->numrows, A2->nnz);
        T0_fprintf(stderr, "\nJaccard Similarity Matrix is %ldx%ld and has %ld values.\n", Jaccard_mat->numrows, Jaccard_mat->numrows, Jaccard_mat->nnz);

        T0_fprintf(stderr, "\nRunning Jaccard Similarity K-mers (selector): \n");

        double laptime_jaccard = 0.0;

        // Running selector model for jaccard similarity on k-mer matrix
        laptime_jaccard = jaccard_selector(Jaccard_mat, A2);
        lgp_barrier();

        //T0_fprintf(stderr, "\nJACCARD SIMILARITY MATRIX: \n"); lgp_barrier();
        //print_matrix(Jaccard_mat);
        //printf("\n");

        T0_fprintf(stderr, " %8.5lf seconds\n", laptime_jaccard);

        lgp_barrier();

        lgp_finalize();

    });

    return 0;
}

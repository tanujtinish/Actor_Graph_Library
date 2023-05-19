Graph serialization files


create_mtx_from_isb.cpp  
- outputs mtx file using edge list and vertex list
- compile: g++ create_mtx_from_isb.cpp -o create_mtx_from_isb
- run: ./create_mtx_from_isb <vertex_file_name> <edge_file_name> <num_vertices> <edge_scale> <a_probability> <b_probability> <c_probability>
-   e.g. ./create_mtx_from_isb v.txt e.txt 10 100000 25 25 25
  
 
jaccard_kmer_matrix_generator
- creates a random k-mer mxn matrix and outputs CTF input files and mtx file
- compile: g++ jaccard_kmer_matrix_generator.cpp -o jaccard_kmer_matrix_generator
- run: ./jaccard_kmer_matrix_generator <m_size> <n_size> <P(1)>
-   e.g. ./jaccard_kmer_matrix_generator 4000 1000 0.05

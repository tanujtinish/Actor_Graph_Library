#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
using namespace std;

int main(int argc, char *argv[]) {
 

    if (argc != 4) {printf("Usage: ./jaccard_kmer_matrix_generator <m> <n> <P(1)>\n"); return 1;}
    int m = stoi(argv[1]);
    int n = stoi(argv[2]);
    double p = stoi(argv[3]);

    srand(time(NULL));
    //int threshold = round(RAND_MAX * p);
    int i,j;
    int* matrix = new int[m*n];
    bool atleast1_row = false;
    int col_sums[n] = {0};
    int nnz = 0;
    
    vector<list<int>> index_vals_sequences(n);

    cout << "Creating matrix: " << '\n';
    for (int i=0; i<=m-1; i++){
        atleast1_row = false;
        for (int j=0; j<=n-1; j++){
            int oneOrZero = ((rand() % 100) < p*100) ? 1 : 0;
            matrix[i * n + j] = oneOrZero;
            if (oneOrZero == 1) {col_sums[j]++; nnz++; index_vals_sequences[j].push_front(i+1);}
        }

        // make sure there is atleast one 1 in each row
        while (!atleast1_row) {
            int rand_col = rand() % n;
            if (matrix[i * n + rand_col] == 0) {
                matrix[i * n + rand_col] = 1;
                col_sums[rand_col]++;
                nnz++;
                index_vals_sequences[rand_col].push_front(i+1);
                atleast1_row = true;
            }
        }
    }

    // make sure there is atleast one 1 in each column
    for (j=0; j<=n-1; j++){
        if (col_sums[j] == 0){
            int rand_row = rand() % m;
            matrix[rand_row * n + j] = 1;
            index_vals_sequences[j].push_front(rand_row+1);
            nnz++;
        }
    }

    for (int x = 0; x < index_vals_sequences.size(); x++) {
        index_vals_sequences[x].sort(); //index_vals_sequences[x].unique();
    }
    
    /*// print matrix
    for (i=0; i<=m-1; i++){
        for (j=0; j<=n-1; j++){
            cout << matrix[i * n + j] << " ";
        }
        cout << endl;
    }

    // printing list
    cout << endl;
    for(i = 0; i < index_vals_sequences.size(); i++)
    {
        cout << i << ": ";
        for(auto &j : index_vals_sequences[i])
        {
            cout << j << ' ';
        }
        cout << '\n';
    }*/
    
    cout<< "           finished ..." << '\n' << '\n';

    // create mtx file
    string out_name = "jaccard_kmer_" + to_string(m) + "x" + to_string(n) + "_matrix.mtx";
    ofstream out_file(out_name);
    cout << "Creating Matrix Market Format file: " << '\n';
    if (out_file.is_open()) {
        out_file << "%%MatrixMarket matrix coordinate pattern" << '\n';
        out_file << m << ' ' << n << ' ' << nnz << '\n';

        for (i=0; i<=m-1; i++){
            for (j=0; j<=n-1; j++){
                if (matrix[i * n + j] == 1) {out_file << i+1 << " " << j+1 << endl;}
            }
        }

        out_file.close();
    }
    cout<< "           finished ..." << '\n' << '\n';

    // create CTF files
    vector<string> input_files(n);
    cout << "Creating CTF files: " << '\n';
    cout << "          creating input_*.txt files ..." << '\n';
    for (int col_file_index = 0; col_file_index < n; col_file_index++) {
        string out_name = "input_" + to_string(col_file_index) + ".txt";
        input_files.push_back(out_name);
        ofstream out_file(out_name);
        if (out_file.is_open()) {
            list<int> col_contents = index_vals_sequences[col_file_index];

            for (auto const &l_content: col_contents) {
                out_file << l_content << endl;
            }

            out_file.close();
        }
    }
    cout<< "           finished ..." << '\n';

    out_name = "input_test.txt";
    ofstream out_test_file(out_name);
    cout << "          creating input_test.txt file ..." << '\n';
    if (out_test_file.is_open()) {

        for (auto f_name: input_files) {
            if(!f_name.empty()) out_test_file << f_name << endl;
        }

        out_test_file.close();
    }
    cout<< "           finished ..." << '\n';

    return 0;
}
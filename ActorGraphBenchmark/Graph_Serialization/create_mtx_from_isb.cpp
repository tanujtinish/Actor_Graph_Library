#include <iostream>
#include <regex>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <tuple>
#include <list>
using namespace std;

int main(int argc, char* argv[]) {

    if (argc != 8) {printf("Usage: ./create_mtx_from_isb <vertex_file_name> <edge_file_name> <num_vertices> <edge_scale> <a> <b> <c>\n"); return 1;}
    string vertex_list_file = argv[1];
    string edge_list_file = argv[2];
    int temp_num_vertices = stoi(argv[3]);
    int num_vertices = 1 << stoi(argv[3]);
    string edge_scale = argv[4];
    string a_probability = argv[5];
    string b_probability = argv[6];
    string c_probability = argv[7];

    string edge_line;
    fstream edge_file;
    vector<int> nonzeros;
    cout << "Reading edge file: " << '\n';
    edge_file.open(edge_list_file,ios::in);
    if (edge_file.is_open()){

        while(getline(edge_file, edge_line)){

            auto const regex_edge = regex(": [0-9]+");
            smatch m_edge; 
            regex_search(edge_line, m_edge, regex_edge); 
        
            for (string x : m_edge) {
                auto const regex_edge2 = regex("[0-9]+");
                smatch m_edge2; 
                regex_search(x, m_edge2, regex_edge2); 
                for (string y: m_edge2) {
                    if(!y.empty()) nonzeros.push_back(stoi(y) + 1);
                }
                
            }

        }

        edge_file.close();
    }
    cout<< "           finished ..." << '\n';

    string vertex_line;
    fstream vertex_file;
    vector<int> offset;
    vertex_file.open(vertex_list_file,ios::in);
    cout << "Reading vertex file: " << '\n';
    if (vertex_file.is_open()){

        while(getline(vertex_file, vertex_line)){

            auto const regex_vertex = regex(": [0-9]+");
            smatch m_vertex; 
            regex_search(vertex_line, m_vertex, regex_vertex); 
        
            for (string x : m_vertex) {
                auto const regex_vertex2 = regex("[0-9]+");
                smatch m_vertex2; 
                regex_search(x, m_vertex2, regex_vertex2); 
                for (string y: m_vertex2) {
                    if(!y.empty()) offset.push_back(stoi(y));
                }
                
            }

        }
        offset.push_back(offset.back());

        vertex_file.close();
    }
    cout<< "           finished ..." << '\n';

    vector<int> vertex_u;
    vector<int> vertex_v;
    for (int q = 0; q < num_vertices + 1; q++) {
        int vertex_1 = q+1;
        for (int q_offset = offset[q]; q_offset < offset[q+1]; q_offset++) {
            int vertex_2 = nonzeros[q_offset];
            if (vertex_1 > vertex_2) {vertex_u.push_back(vertex_1); vertex_v.push_back(vertex_2);}
            else {vertex_u.push_back(vertex_2); vertex_v.push_back(vertex_1);}
        }
    }

    string out_name = "isb_tricount_" + to_string(temp_num_vertices) + "_" + edge_scale + "_" + a_probability + "_" + b_probability + "_" + c_probability + ".mtx";
    ofstream out_file(out_name);
    cout << "Creating Matrix Market Format file: " << '\n';
    if (out_file.is_open()) {

        out_file << "%%MatrixMarket matrix coordinate pattern" << '\n';
        out_file << num_vertices << ' ' << num_vertices << ' ' << vertex_u.size() << '\n';

        for (int h = 0; h < vertex_u.size(); h++) {
            out_file << to_string(vertex_u[h]) << ' ' << to_string(vertex_v[h]) << '\n';
        }

        out_file.close();
    }
    cout<< "           finished ..." << '\n';

    return 0;
}



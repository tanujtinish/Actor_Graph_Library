#ifndef EDGE_LIST_DATA_STRUCTURE
#define EDGE_LIST_DATA_STRUCTURE

#include <vector>

// Used to hold node & weight, with another node it makes a weighted edge
template <typename NodeID_, typename WeightT_>
struct NodeWeight {
  NodeID_ v;
  WeightT_ w;
  NodeWeight() {}
  NodeWeight(NodeID_ v) : v(v), w(1) {}
  NodeWeight(NodeID_ v, WeightT_ w) : v(v), w(w) {}

  bool operator< (const NodeWeight& rhs) const {
    return v == rhs.v ? w < rhs.w : v < rhs.v;
  }

  // doesn't check WeightT_s, needed to remove duplicate edges
  bool operator== (const NodeWeight& rhs) const {
    return v == rhs.v;
  }

  // doesn't check WeightT_s, needed to remove self edges
  bool operator== (const NodeID_& rhs) const {
    return v == rhs;
  }

  operator NodeID_() {
    return v;
  }
};

template <typename NodeID_, typename WeightT_>
std::ostream& operator<<(std::ostream& os,
                         const NodeWeight<NodeID_, WeightT_>& nw) {
  os << nw.v << " " << nw.w;
  return os;
}

template <typename NodeID_, typename WeightT_>
std::istream& operator>>(std::istream& is, NodeWeight<NodeID_, WeightT_>& nw) {
  is >> nw.v >> nw.w;
  return is;
}



// Syntatic sugar for an edge
template <typename SrcT, typename DstT = SrcT>
struct EdgePair {
  SrcT u;
  DstT v;

  EdgePair() {}

  EdgePair(SrcT u, DstT v) : u(u), v(v) {}

  bool operator< (const EdgePair& rhs) const {
    return u == rhs.u ? v < rhs.v : u < rhs.u;
  }

  bool operator== (const EdgePair& rhs) const {
    return (u == rhs.u) && (v == rhs.v);
  }
};

typedef EdgePair<int64_t, int64_t> Edge;
typedef EdgePair<int64_t, NodeWeight<int64_t, int64_t>> WEdge;
typedef std::vector<Edge> EdgeList;
typedef std::vector<WEdge> WEdgeList;

class ResultFromGenerator {
	public:
    int total_pe;

    int SCALE;
		int edgefactor;
    int64_t local_edges;
    int64_t global_vertices;
    int64_t global_edges;

    packed_edge* restrict edgememory; /* NULL if edges are in file */
    int64_t edgememory_size;
    int64_t max_edgememory_size;
    float* weightmemory;

    EdgeList edgeList;
    WEdgeList wEdgeList;
    int64_t edgelist_size;
    int64_t max_edgelist_size;

    double make_graph_time;

    ResultFromGenerator(){

    }
    
    ResultFromGenerator(const ResultFromGenerator& other){
      total_pe= other.total_pe;
      SCALE= other.SCALE;
      edgefactor= other.edgefactor;
      local_edges= other.local_edges;
      global_vertices= other.global_vertices;
      global_edges= other.global_edges;
      edgememory= other.edgememory;
      edgememory_size= other.edgememory_size;
      max_edgememory_size= other.max_edgememory_size;
      weightmemory= other.weightmemory;
      edgeList= other.edgeList;
      wEdgeList= other.wEdgeList;
      edgelist_size= other.edgelist_size;
      max_edgelist_size= other.max_edgelist_size;
    }
    
    ResultFromGenerator operator=(const ResultFromGenerator& other) {
      total_pe= other.total_pe;
      SCALE= other.SCALE;
      edgefactor= other.edgefactor;
      local_edges= other.local_edges;
      global_vertices= other.global_vertices;
      global_edges= other.global_edges;
      edgememory= other.edgememory;
      edgememory_size= other.edgememory_size;
      max_edgememory_size= other.max_edgememory_size;
      weightmemory= other.weightmemory;
      edgeList= other.edgeList;
      wEdgeList= other.wEdgeList;
      edgelist_size= other.edgelist_size;
      max_edgelist_size= other.max_edgelist_size;
        
        return *this;
    }
};

#endif  // RESULT_FROM_GENERATOR_SELECTOR_
/**
 * @file poisson.cpp
 * Test script for treating the Graph as a MTL Matrix
 * and solving a Poisson equation.
 *
 * @brief Reads in two files specified on the command line.
 * First file: 3D Points (one per line) defined by three doubles.
 * Second file: Eges (one per line) defined by 2 indices into the point list
 *              of the first file.
 *
 * Launches an SFML_Viewer to visualize the solution.
 */

#include <fstream>

#include "CME212/SFML_Viewer.hpp"
#include "CME212/Util.hpp"
#include "CME212/Point.hpp"
#include "CME212/BoundingBox.hpp"
#include <math.h>

#include "Graph.hpp"

// HW3: YOUR CODE HERE
// Define a GraphSymmetricMatrix that maps
// your Graph concept to MTL's Matrix concept. This shouldn't need to copy or
// modify Graph at all!
using GraphType = Graph<char,char>;  //<  DUMMY Placeholder
using NodeType  = typename GraphType::node_type;

/** Remove all the nodes in graph @a g whose posiiton is within Box3D @a bb.
 * @param[in,out] g  The Graph to remove nodes from
 * @param[in]    bb  The BoundingBox, all nodes inside this box will be removed
 * @post For all i, 0 <= i < @a g.num_nodes(),
 *        not bb.contains(g.node(i).position())
 */
void remove_box(GraphType& g, const Box3D& bb) {
  // HW3: YOUR CODE HERE
  (void) g; (void) bb;   //< Quiet compiler
  for (auto it = g.node_begin(); it != g.node_end(); ++it){
    auto n = *it;
    if bb.contains(n.position()){
      g.remove_node(n);
    }
  }
  //return;
}

class GraphSymetricMatrix{
  public:
    GraphSymmetricMatrix(GraphType& g) : g_(g) {}

  /** Get the dimension of the matrix */
  std::size_t get_dim() const{
    return g_.num_nodes();
  }

  // L(i,j), Discrete Matrix Approximating Laplace Operator
  int L(NodeType i, NodeType j){
    if (i == j){
      return -i.degree();
    }
    else if ( g_.has_edge(i,j) || g_.has_edge(j,i) ) {
      return 1;
    }
    else {
      return 0;
    }
  }

  // A(i,j), Linear System of Equations
  int A(NodeType i, NodeType j){
    if (i == j) && (g_BCs(i) != -1){
      return 1;
    }
    else if (i != j) && ( (g_BCs(i) != -1) || (g_BCs(j) != -1) ){
      return 0;
    }
    else {
      return L(i,j);
    }
  }

  /** Helper function to perfom multiplication. Allows for delayed 
   *  evaluation of results.
   *  Assign :: apply(a, b) resolves to an assignment operation such as 
   *     a += b, a -= b, or a = b.
   *  @pre @a size(v) == size(w) */
  template <typename VectorIn, typename VectorOut, typename Assign>
  void mult (const VectorIn& v, VectorOut& w, Assign) const {
      assert(size(v) == size(w));
      double temp = 0;
      for (auto nit = g_.node_begin(); nit != g_.node_end(); ++nit){
        auto i = *nit;
        for (auto eit = i.edge_begin; eit != i.edge_end(); ++eit){
          e = *eit;
          j = e.node2();
          temp += A(i,j)*v[j.index()];
        }
        Assign::apply(w[i.index()], temp);
      }
  }

  /** Matvec forward to MTL's lazy mat_cvec_multiplier oeprator */
  template <typename Vector> 
  mtl::vec::mat_cvec_multiplier<IdentityMatrix, Vector>
  operator*(const Vector& v) const {
    return {*this, v};
  }

  private:
    // Empty
    GraphType& g_;
    // g_.num_nodes() is our n_

};



inline std::size_t size(const SymmetricsMatrix& M){
  return M.get_dim()*M.get_dim();
}

inline std::size_t num_rows(const SymmetricMatrix& M){
  return M.get_dim();
}

inline std::size_t num_cols(const SymmetricMatrix& M){
  return M.get_dim();
}



/** boundary conditions g(x) */
int g_BCs(const NodeType n){
  // first check if on boundary
  BoundingBox thisbox = Box3D(Point(-0.6,-0.2,-1), Point( 0.6, 0.2,1));
  if (norm_inf(n.position()) == 1){
    return 0;
  }
  else if (norm_inf(n.position() - Point(0.6,0.6,0)) < 0.2) || (norm_inf(n.position() - Point(-0.6,0.6,0)) < 0.2) || (norm_inf(n.position() - Point(0.6,-0.6,0)) < 0.2) || (norm_inf(n.position() - Point(-0.6,-0.6,0)) < 0.2){
    return -0.2;
  }
  else if thisbox.contains(n.position()) {
    return 1;
  }
  // if not on boundary then use forcing function
  else {
    return -1;
  }
}

/** forcing function */
int forcing_function(NodeType n){
  return 5*cos( norm_1(x.position()) );
}





int main(int argc, char** argv)
{
  // Check arguments
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " NODES_FILE TETS_FILE\n";
    exit(1);
  }

  // Define an empty Graph
  GraphType graph;

  {
    // Create a nodes_file from the first input argument
    std::ifstream nodes_file(argv[1]);
    // Interpret each line of the nodes_file as a 3D Point and add to the Graph
    std::vector<NodeType> node_vec;
    Point p;
    while (CME212::getline_parsed(nodes_file, p))
      node_vec.push_back(graph.add_node(2*p - Point(1,1,0)));

    // Create a tets_file from the second input argument
    std::ifstream tets_file(argv[2]);
    // Interpret each line of the tets_file as four ints which refer to nodes
    std::array<int,4> t;
    while (CME212::getline_parsed(tets_file, t)) {
      graph.add_edge(node_vec[t[0]], node_vec[t[1]]);
      graph.add_edge(node_vec[t[0]], node_vec[t[2]]);
      graph.add_edge(node_vec[t[1]], node_vec[t[3]]);
      graph.add_edge(node_vec[t[2]], node_vec[t[3]]);
    }
  }

  // Get the edge length, should be the same for each edge
  auto it = graph.edge_begin();
  assert(it != graph.edge_end());
  double h = norm((*it).node1().position() - (*it).node2().position());

  // Make holes in our Graph
  remove_box(graph, Box3D(Point(-0.8+h,-0.8+h,-1), Point(-0.4-h,-0.4-h,1)));
  remove_box(graph, Box3D(Point( 0.4+h,-0.8+h,-1), Point( 0.8-h,-0.4-h,1)));
  remove_box(graph, Box3D(Point(-0.8+h, 0.4+h,-1), Point(-0.4-h, 0.8-h,1)));
  remove_box(graph, Box3D(Point( 0.4+h, 0.4+h,-1), Point( 0.8-h, 0.8-h,1)));
  remove_box(graph, Box3D(Point(-0.6+h,-0.2+h,-1), Point( 0.6-h, 0.2-h,1)));

  // HW3: YOUR CODE HERE
  // Define b using the graph, f, and g.
  // Construct the GraphSymmetricMatrix A using the graph
  // Solve Au = b using MTL.


  return 0;
}

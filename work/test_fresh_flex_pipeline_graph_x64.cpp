#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "graph_exact_equal.h"
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=3)throw std::runtime_error("combined and sequential graphs required");auto combined=loadGraph(argv[1]),sequential=loadGraph(argv[2]);validateGraph(combined);validateGraph(sequential);if(!graphExactEqualCandidate(combined,sequential))throw std::runtime_error("combined timeline/flex graph differs from sequential passes");std::cout<<"timeline_fresh_flex_complete_graph_sequential_equivalence=passed elements="<<combined.elements.size()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

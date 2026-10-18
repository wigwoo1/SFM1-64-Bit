#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_transform_control_default.h"
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);Graph g;g.strings={"DmeTransformControl","control","defaultPosition"};g.elements.resize(1);g.elements[0].type=0;g.elements[0].name=1;
for(unsigned i=0;i<1024;++i){g.elements[0].attributes.clear();if(i%4){Value v;v.wire.resize(12);std::array<float,3>a{};for(unsigned c=0;c<3;++c)a[c]=float(int((i*(13+c*4))%521)-260)/16.f;std::memcpy(v.wire.data(),a.data(),12);g.elements[0].attributes.push_back({2,10,{v}});}auto actual=graphSelectedControlDefaultPositionCandidate(g,0,{0,0,0});std::array<float,3>expected{};f.read(reinterpret_cast<char*>(expected.data()),12);if(!f||std::memcmp(actual.data(),expected.data(),12))throw std::runtime_error("native defaultPosition mismatch");}
char trailing;if(f.read(&trailing,1))throw std::runtime_error("trailing oracle bytes");g.elements[0].attributes[0].type=13;bool rejected=false;try{graphSelectedControlDefaultPositionCandidate(g,0,{0,0,0});}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("malformed default accepted");std::cout<<"native_default_position_matches=1024 malformed_rejected=1"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}

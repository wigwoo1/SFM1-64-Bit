#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "graph_ik_parent_frame.h"
int main(){try{Graph g;g.strings={"DmeDag","node","disableOverride","overridePos","overrideRot","overrideParent"};g.elements.resize(3);for(auto&e:g.elements){e.type=0;e.name=1;}
auto flag=[&](uint32_t name,char value){Value v;v.wire={value};return Attribute{name,4,{v}};};
Value ref;ref.reference=2;g.elements[0].attributes={flag(2,0),flag(3,1),flag(4,1),Attribute{5,1,{ref}}};
GraphTransformHierarchy h(g);h.ordinaryParents[0]=1;auto ordinary=h.identity(),overrideFrame=h.identity();ordinary[0][3]=3;overrideFrame[1][3]=7;h.cache[1]=ordinary;h.cache[2]=overrideFrame;
unsigned checks=0;for(unsigned mask=0;mask<4;++mask){g.elements[0].attributes[1].values[0].wire[0]=char(mask&1);g.elements[0].attributes[2].values[0].wire[0]=char((mask>>1)&1);
for(char disable:{char(0),char(1),char(2),char(-1)}){g.elements[0].attributes[0].values[0].wire[0]=disable;
if(h.parent(0,true)!=(disable||!(mask&1)?1u:2u)||h.parent(0,false)!=(disable||!(mask&2)?1u:2u))throw std::runtime_error("parent gate mismatch");++checks;
auto frame=graphIkParentFrameCandidate(g,h,0);if(disable||mask==0){if(frame!=ordinary)throw std::runtime_error("disabled frame mismatch");}else if(mask==3){if(frame!=overrideFrame)throw std::runtime_error("enabled frame mismatch");}++checks;}}
g.elements[0].attributes.erase(g.elements[0].attributes.begin());if(h.overridesDisabled(0))throw std::runtime_error("missing flag default");++checks;
g.elements[0].attributes.insert(g.elements[0].attributes.begin(),flag(2,0));g.elements[0].attributes[0].type=3;bool rejected=false;try{h.parent(0,true);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("malformed flag accepted");++checks;
std::cout<<"dag_override_gate_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}

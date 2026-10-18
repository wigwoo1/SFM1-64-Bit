#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_model_illumination_dag_candidate.h"
#include <iostream>
int main(){try{
 Graph g;g.strings={"DmeGameModel","DmeDag","DmElement","illumPositionDag","children","model","dag1","dag2"};g.elements={{0,5,{},{}},{1,6,{},{}},{1,7,{},{}},{2,5,{},{}}};auto isDag=[&](uint32_t id){return g.elements.at(id).type==1;};Value ref;ref.reference=1;Attribute illumination{3,1,{ref}};Value child;child.reference=2;Attribute children{4,15,{child}};g.elements[0].attributes={children};
 auto r=sessionModelIlluminationDagCandidate(g,0,isDag);if(r.attributePresent||r.dag||r.firstChildAttempted)throw std::runtime_error("absent attribute treated as null");
 g.elements[0].attributes.push_back(illumination);r=sessionModelIlluminationDagCandidate(g,0,isDag);if(!r.attributePresent||r.dag!=1u||r.firstChildAttempted)throw std::runtime_error("explicit DAG not preferred");
 g.elements[0].attributes[1].values[0].reference=UINT32_MAX;r=sessionModelIlluminationDagCandidate(g,0,isDag);if(r.dag!=2u||!r.firstChildAttempted)throw std::runtime_error("null reference child-zero fallback");
 g.elements[0].attributes[1].values[0].reference=3;r=sessionModelIlluminationDagCandidate(g,0,isDag);if(r.dag!=2u)throw std::runtime_error("non-DAG reference fallback");
 g.elements[0].attributes[0].values.insert(g.elements[0].attributes[0].values.begin(),Value{});r=sessionModelIlluminationDagCandidate(g,0,isDag);if(r.dag||!r.firstChildAttempted)throw std::runtime_error("skipped null first child");
 g.elements[0].attributes[0].values.clear();r=sessionModelIlluminationDagCandidate(g,0,isDag);if(r.dag||r.firstChildAttempted)throw std::runtime_error("empty child array fallback");
 unsigned guards=0;auto reject=[&](auto action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}if(!failed)throw std::runtime_error("invalid binding accepted");++guards;};
 g.elements[0].attributes[1].type=3;reject([&]{sessionModelIlluminationDagCandidate(g,0,isDag);});g.elements[0].attributes[1].type=1;g.elements[0].attributes[1].values.clear();reject([&]{sessionModelIlluminationDagCandidate(g,0,isDag);});g.elements[0].attributes[1].values.push_back(Value{});g.elements[0].attributes[1].values[0].reference=99;reject([&]{sessionModelIlluminationDagCandidate(g,0,isDag);});g.elements[0].attributes[1].values[0].reference=UINT32_MAX;g.elements[0].attributes[0].type=1;reject([&]{sessionModelIlluminationDagCandidate(g,0,isDag);});reject([&]{sessionModelIlluminationDagCandidate(g,1,isDag);});
 if(guards!=5)throw std::runtime_error("guard count");std::cout<<"PASS absent/null/non-DAG distinction, explicit precedence, first-child-only fallback and five malformed-schema/reference guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

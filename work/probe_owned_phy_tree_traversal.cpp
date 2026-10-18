#include "physics_tree_traversal_test_fixture.h"
#include <fstream>
#include <iostream>
#include <iterator>
// Test-only leaf replay: the actual original leaf poststate is supplied by the
// capture. This verifies owned traversal, not owned leaf collision or a tracer.
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned AMD64");if(argc!=5)throw std::runtime_error("PHY captures tree-output walk-output required");std::ifstream phy(argv[1],std::ios::binary),input(argv[2],std::ios::binary);std::ofstream treeOutput(argv[3],std::ios::binary),output(argv[4],std::ios::binary);if(!phy||!input||!treeOutput||!output)throw std::runtime_error("tree replay streams");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(phy)),{});const SourcePhyCompactGeometryCandidate geometry(b,true);if(geometry.solids.size()!=1||geometry.collisionTrees.size()!=1)throw std::runtime_error("tree replay requires one solid");const auto&tree=geometry.collisionTrees[0];treeWrite(treeOutput,tree);size_t cases=0,leaves=0;
    while(input.peek()!=std::char_traits<char>::eof()){
        uint32_t query,count;supportGet(input,query);auto state=treeReadState(input);const auto initial=state;supportGet(input,count);if(count>tree.size())throw std::runtime_error("tree replay event count");struct Event{uint32_t node;SourcePhysicsTreeStateCandidate before,after;};std::vector<Event>events(count);for(auto&e:events){supportGet(input,e.node);e.before=treeReadState(input);e.after=treeReadState(input);}const auto expected=treeReadState(input);size_t at=0;std::ostringstream results(std::ios::binary);
        sourcePhysicsTreeTraversalCandidate(tree,state,[&](uint32_t node,uint32_t ledge,SourcePhysicsTreeStateCandidate&s){if(at>=events.size()||events[at].node!=node||ledge>=geometry.solids[0].size()||!treeSameState(s,events[at].before))throw std::runtime_error("owned traversal order/prestate mismatch");supportPut(results,node);treeWriteState(results,s);s=events[at++].after;treeWriteState(results,s);});if(at!=events.size()||!treeSameState(state,expected))throw std::runtime_error("owned traversal final/count mismatch");supportPut(output,query);treeWriteState(output,initial);supportPut(output,count);output<<results.str();treeWriteState(output,state);++cases;leaves+=at;
    }treeOutput.flush();output.flush();if(!input.eof()||!treeOutput||!output)throw std::runtime_error("tree replay IO");std::cout<<"PASS owned tree replay cases="<<cases<<" leaves="<<leaves<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

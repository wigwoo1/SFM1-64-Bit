#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
#include <stdexcept>
struct OperatorDependencyNode{uintptr_t identity=0;std::vector<size_t> edges;};
struct OperatorDependencyOrder{std::vector<uintptr_t> identities;bool cycle=false;};
// 0x103998F0 postorder DFS, followed by reversal in 0x103999F0.
inline OperatorDependencyOrder orderOperatorDependencies(const std::vector<OperatorDependencyNode>&nodes,const std::vector<size_t>&roots){
 OperatorDependencyOrder result;std::vector<uint8_t> state(nodes.size());
 struct Frame{size_t index,nextEdge;};std::vector<Frame> stack;
 auto enter=[&](size_t index)->bool{
  if(index>=nodes.size())throw std::runtime_error("dependency node index out of range");
  if(state[index]){if(state[index]==1)result.cycle=true;return false;}
  state[index]=1;stack.push_back({index,0});return true;
 };
 // Preserve recursive edge/root order without consuming the process call stack.
 for(auto root:roots){if(!enter(root))continue;
  while(!stack.empty()){
   const size_t index=stack.back().index;
   if(stack.back().nextEdge<nodes[index].edges.size()){
    const size_t edge=nodes[index].edges[stack.back().nextEdge++];enter(edge);
   }else{result.identities.push_back(nodes[index].identity);state[index]=2;stack.pop_back();}
  }
 }
 std::reverse(result.identities.begin(),result.identities.end());return result;
}

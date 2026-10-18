#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
// Brush phase only. Displacements and BSP traversal are deliberately separate.
// surfaceExcludes(index) implements the native any-surface flag0x80 rule.
template<class State,class SurfaceExcludes,class TraceBrush>
void sourceLeafBrushPolicyCandidate(const std::vector<uint32_t>&ordered,const std::vector<uint32_t>&contents,
 std::vector<uint32_t>&visited,uint32_t generation,uint32_t mask,State&state,
 SurfaceExcludes surfaceExcludes,TraceBrush traceBrush){
 if(visited.size()!=contents.size())throw std::runtime_error("brush visit table mismatch");
 for(uint32_t index:ordered)if(index>=contents.size())throw std::runtime_error("leaf brush index out of range");
 for(uint32_t index:ordered){
  if(visited[index]==generation)continue;visited[index]=generation;
  const uint32_t overlap=contents[index]&mask;if(!overlap)continue;
  if(overlap==0x80&&(mask&0x2000)&&surfaceExcludes(index))continue;
  traceBrush(index,state);if(state.fraction==0)break;
 }
}

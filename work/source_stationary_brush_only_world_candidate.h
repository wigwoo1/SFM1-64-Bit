#pragma once
#include "source_stationary_leaf_selection_candidate.h"
#include "source_stationary_brush_candidate.h"
// Stationary zero-extent brush-only world query. Displacements excluded.
inline void sourceStationaryBrushOnlyWorldCandidate(const SourceBspBrushOnlyTraceCandidate&world,
 const std::array<float,3>&point,uint32_t mask,SourceOptimizedBoxLineStateCandidate&state){
 const auto selection=sourceStationaryLeafSelectionCandidate(world,point,{1,1,1});auto result=state;std::vector<uint8_t>visited(world.data.brushes.size());bool anyNonSolidLeaf=false;
 for(auto leaf:selection.leaves){
  if(!(world.data.leafContents.at(leaf)&1))anyNonSolidLeaf=true;
  for(auto index:world.data.leafBrushes.at(leaf)){if(visited.at(index))continue;visited[index]=1;if(!(world.data.brushes.at(index).contents&mask))continue;sourceStationaryBrushCandidate(world.data,world.records,index,point,result);if(result.fraction==0)break;}
  if(result.allSolid)break;
 }
 if(!anyNonSolidLeaf){result.fraction=0;result.leftSolid=1;result.allSolid=true;result.startSolid=true;}state=result;
}

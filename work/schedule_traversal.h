#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
struct ScheduleNode {uint32_t operation;std::vector<std::vector<uint32_t>> edgeGroups;};
struct ScheduleResult {bool cycle=false;std::vector<uint32_t> order;};
// Recovered 0x103998F0 postorder plus 0x103999F0 final reversal.
inline ScheduleResult traverseSchedule(const std::vector<ScheduleNode>&nodes,const std::vector<uint32_t>&roots){
 ScheduleResult r;std::vector<uint8_t> state(nodes.size(),0);
 struct Frame{uint32_t node;size_t group=0,edge=0;};std::vector<Frame> stack;
 auto enter=[&](uint32_t i){if(i>=nodes.size())throw std::runtime_error("bad schedule node");
  if(state[i]){if(state[i]==1)r.cycle=true;return false;}state[i]=1;stack.push_back({i});return true;};
 for(auto root:roots){if(!enter(root))continue;while(!stack.empty()){
  auto&frame=stack.back();const auto&groups=nodes[frame.node].edgeGroups;
  while(frame.group<groups.size()&&frame.edge==groups[frame.group].size()){++frame.group;frame.edge=0;}
  if(frame.group==groups.size()){r.order.push_back(nodes[frame.node].operation);state[frame.node]=2;stack.pop_back();}
  else{const uint32_t next=groups[frame.group][frame.edge++];enter(next);}
 }}std::reverse(r.order.begin(),r.order.end());return r;
}

#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
struct OperatorRootAttribute{uint16_t flags=0;bool excluded=false;std::vector<size_t> operators;};
// 0x103996E0: operator-dirty roots first; then table-order dirty attributes;
// unique nodes. Attribute flags are cleared even for excluded producer nodes.
inline std::vector<size_t> selectOperatorRoots(const std::vector<bool>&active,std::vector<OperatorRootAttribute>&attributes){
 std::vector<size_t> roots;std::vector<bool> selected(active.size());
 auto add=[&](size_t index){if(index>=selected.size())throw std::runtime_error("root operator index out of range");if(!selected[index]){selected[index]=true;roots.push_back(index);}};
 for(size_t i=0;i<active.size();++i)if(active[i])add(i);
 for(auto&attribute:attributes){if(!attribute.excluded&&(attribute.flags&0x4000))for(auto index:attribute.operators)add(index);attribute.flags&=0xbfff;}
 return roots;
}

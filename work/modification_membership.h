#pragma once
#include <cstdint>
#include <vector>
struct ModificationMember{uint32_t channel=0xffffffff;uint32_t references=0;};
// Resolution callback reports native handle liveness. First vacant slot is
// reused, but an existing matching member anywhere takes precedence.
template<class Alive> inline size_t addModificationMember(std::vector<ModificationMember>&members,uint32_t channel,Alive alive){
 size_t vacant=members.size();
 for(size_t i=0;i<members.size();++i){
  if(members[i].channel==channel){++members[i].references;return i;}
  if(vacant==members.size()&&!alive(members[i].channel))vacant=i;
 }
 if(vacant==members.size())members.push_back({});
 members[vacant]={channel,1};return vacant;
}
inline bool modificationNeedsOverlay(size_t layers){return layers==1;}
// 0x104D1658 decrements with 32-bit wrap; native signed count >=1 stays.
// Caller performs log cleanup before the member handle is cleared.
template<class Cleanup> inline bool releaseModificationMember(std::vector<ModificationMember>&members,int32_t index,Cleanup cleanup){
 if(index<0||static_cast<size_t>(index)>=members.size())return false;
 auto&member=members[index];--member.references;
 if(member.references!=0&&member.references<0x80000000u)return false;
 cleanup(member.channel);member.channel=0xffffffff;return true;
}

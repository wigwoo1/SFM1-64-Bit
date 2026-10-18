#include "source_leaf_displacement_policy_candidate.h"
#include <iostream>
int main(){try{
 const std::array<float,3>lo{-1,-1,-1},hi{1,1,1},start{0,0,0},delta{0,0,1},inverse{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),1};
 if(!sourceDisplacementBoundsGateCandidate(lo,hi,start,delta,inverse)||sourceDisplacementBoundsGateCandidate(lo,hi,{1.001f,0,0},delta,inverse)||!sourceDisplacementBoundsGateCandidate(lo,hi,{1,0,0},delta,inverse))throw std::runtime_error("analytical bounds gate/parallel endpoint rule");
 std::vector<SourceLeafDisplacementCullCandidate>culls={{{-1,-1,-1},{1,1,1},0,0},{{-1,-1,-1},{1,1,1},1,0}};
 std::vector<uint32_t>visits(1);std::vector<uint32_t>calls;SourceLeafDisplacementStateCandidate state;
 auto miss=[&](uint32_t id,auto&){calls.push_back(id);};
 sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{0,1},culls,visits,7,1,true,false,state,start,delta,inverse,miss);
 if(calls!=std::vector<uint32_t>{1}||visits[0]!=7)throw std::runtime_error("mask before shared visit mark");
 calls.clear();visits[0]=0;state={};culls[1].mins={10,-1,-1};culls[1].maxs={11,1,1};
 sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1},culls,visits,7,1,true,false,state,start,delta,inverse,miss);
 if(!calls.empty()||visits[0]!=7)throw std::runtime_error("visit before rejected bounds");culls[1].mins=lo;culls[1].maxs=hi;
 visits[0]=0;state={};state.startSolid=true;
 sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1},culls,visits,7,1,true,true,state,start,delta,inverse,miss);
 if(!calls.empty()||visits[0]!=0)throw std::runtime_error("nonempty brush startSolid gate");
 sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1},culls,visits,7,1,true,false,state,start,delta,inverse,miss);
 if(calls!=std::vector<uint32_t>{1}||visits[0]!=7)throw std::runtime_error("zero-brush startSolid bypass");
 calls.clear();visits[0]=7;state={};sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1,1},culls,visits,7,1,false,false,state,start,delta,inverse,miss);
 if(calls!=std::vector<uint32_t>{1,1}||visits[0]!=7)throw std::runtime_error("nonswept duplicate visit behavior");
 calls.clear();visits[0]=0;state={};sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1,1},culls,visits,7,1,false,false,state,start,delta,inverse,[&](uint32_t id,auto&s){calls.push_back(id);s.fraction=0;s.displacementHit=true;s.normal={0,0,1};});
 if(calls.size()!=1||state.fraction!=0||!state.startSolid||!state.allSolid)throw std::runtime_error("fraction-zero stop and post adjustment");
 state={};state.fraction=.5f;state.displacementHit=true;state.normal={0,0,1};calls.clear();sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1},culls,visits,7,0,true,false,state,start,delta,inverse,miss);
 if(!calls.empty()||state.fraction!=.5f||!state.startSolid||!state.allSolid)throw std::runtime_error("masked phase still runs post helper");
 state={};state.displacementHit=true;state.normal={0,0,1};sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{},culls,visits,7,0,true,false,state,start,delta,inverse,miss);if(state.startSolid||state.allSolid)throw std::runtime_error("empty displacement span must skip post helper");
 state={};visits[0]=0;bool rejected=false;try{sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1},culls,visits,7,1,true,false,state,start,delta,inverse,[](uint32_t,auto&s){s.fraction=.25f;throw std::runtime_error("controlled callback failure");});}catch(const std::runtime_error&){rejected=true;}if(!rejected||state.fraction!=1||visits[0]!=0)throw std::runtime_error("callback failure transaction");
 rejected=false;try{sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{2},culls,visits,7,1,true,false,state,start,delta,inverse,miss);}catch(const std::runtime_error&){rejected=true;}if(!rejected||state.fraction!=1||visits[0]!=0)throw std::runtime_error("invalid reference transaction");
 culls[1].visitIndex=1;rejected=false;try{sourceLeafDisplacementPolicyCandidate(std::vector<uint32_t>{1},culls,visits,7,1,true,false,state,start,delta,inverse,miss);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("invalid visit table accepted");
 rejected=false;try{sourceDisplacementBoundsGateCandidate(lo,hi,start,delta,inverse,-1);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("negative padding accepted");
 std::cout<<"PASS analytical bounds, mask/mark/order, brush-dependent solid gate, duplicate/zero/post behavior, failure transactions and guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

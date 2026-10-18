from pathlib import Path
root=Path(__file__).parent
s=(root/'session_expression_chain_pass.h').read_text()
body=s[s.index(' auto profileStart='):]
prepare=body[:body.index(' SessionExpressionChainPassResultCandidate result;')]
prepare=prepare.replace('auto plan=planSessionExpressionChainsCandidate','plan=planSessionExpressionChainsCandidate',1)
prepare=prepare.replace('if(equivalentConstantChannelWritersCandidate(runtime.graph,it->second))it=plan.conflictingWriters.erase(it);','if(equivalentConstantChannelWritersCandidate(runtime.graph,it->second)){equivalentWriters.push_back(it->second);it=plan.conflictingWriters.erase(it);}',1)
prepare=prepare.replace(' std::map<uint32_t,std::vector<uint32_t>>playFrames;std::set<uint32_t>expressions,copies;','')
prepare=prepare.replace(' std::map<uint32_t,RuntimeCachedFloatPlayBinding>plays;std::map<uint32_t,RuntimeCachedFloatCopyBinding>copyBindings;','')
prepare=prepare.replace(' std::set<uint32_t>packIds(packColors.begin(),packColors.end());',' packIds.insert(packColors.begin(),packColors.end());')
loop=body[body.index(' for(auto id:plan.schedule.order)'):body.index(' }auto operationEnd=')+2]
prefix='''#pragma once
#include "session_expression_chain_pass.h"
// Caller keeps runtime alive and invalidates/rebuilds before structural, endpoint,
// expression-dependency or path edits. Value-only sampling retains bindings.
class PreparedSessionExpressionChainPassCandidate {
 SessionRuntime&runtime;SessionExpressionChainPlanCandidate plan;
 std::map<uint32_t,std::vector<uint32_t>>playFrames;
 std::set<uint32_t>expressions,copies,packIds;
 std::map<uint32_t,RuntimeCachedFloatPlayBinding>plays;
 std::map<uint32_t,RuntimeCachedFloatCopyBinding>copyBindings;
 std::vector<std::vector<uint32_t>>equivalentWriters;
 bool valid=true;double preparationMilliseconds_=0;
public:
 PreparedSessionExpressionChainPassCandidate(SessionRuntime&runtimeValue,const std::vector<SessionExpressionChainCandidate>&chains,const std::map<uint32_t,std::vector<uint32_t>>&typedPaths={},const std::vector<uint32_t>&colorCopies={},const std::vector<uint32_t>&packColors={}):runtime(runtimeValue){
'''
prepare+='\n (void)planEnd;preparationMilliseconds_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-profileStart).count();\n }\n'
step=''' void invalidate(){valid=false;}
 double preparationMilliseconds()const{return preparationMilliseconds_;}
 SessionExpressionChainPassResultCandidate step(int32_t rootTick){
 if(!valid)throw std::runtime_error("prepared timeline invalidated; rebuild required");
 auto start=std::chrono::steady_clock::now();
 for(const auto&writers:equivalentWriters)if(!equivalentConstantChannelWritersCandidate(runtime.graph,writers))throw std::runtime_error("prepared timeline writer equivalence changed; rebuild required");
 SessionExpressionChainPassResultCandidate result;
'''+loop+'''
 result.operationMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return result;
 }
};
'''
target=root/'prepared_session_expression_chain_pass.h'
assert not target.exists()
target.write_text(prefix+prepare+step)

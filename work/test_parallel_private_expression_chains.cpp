#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_expression_chain_pass.h"
#include "parallel_private_expression_chains.h"
#include "graph_exact_equal.h"
static uint32_t floatBits(float value){uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("fixture required");auto graph=loadGraph(argv[1]);auto serial=graph;
 auto roots=hierarchyRefs(graph,graph.elements.at(0),"activeClip",1);auto discovery=discoverSessionExpressionChainsCandidate(graph,roots.at(0));
 size_t comparisons=0,chains=0,signedZeroFixtures=0;PrivateExpressionChainWorkerPoolCandidate pool(4);
 for(const auto&chain:discovery.chains){ChannelBinding binding(graph,chain.play);float value=1;auto tick=channelLocalTime(graph,chain.frames,0,false);
  if(sampleSessionFloatPlayCandidate(graph,binding,tick,value)&&value==0){uint32_t negativeZeroBits=0x80000000u;float negativeZero;std::memcpy(&negativeZero,&negativeZeroBits,4);setFloat(*runtimeAttribute(graph,chain.expression,binding.destination.attribute),0,negativeZero);++signedZeroFixtures;}
 }
 serial=graph;
 for(int32_t tick:{-10000,0,10000}){
  std::vector<PrivateExpressionChainJobCandidate>jobs;
  for(const auto&chain:discovery.chains)jobs.push_back({ChannelBinding(graph,chain.play),chain.expression,channelLocalTime(graph,chain.frames,tick,false)});
  const auto single=samplePrivateExpressionChainsCandidate(graph,jobs,1);
  auto reusedJobs=jobs;
  for(auto&job:reusedJobs){if(!sampleSessionFloatPlayCandidate(graph,job.play,job.tick,job.sampledInput))throw std::runtime_error("reused sample fixture failed");job.hasSampledInput=true;}
  const auto reused=samplePrivateExpressionChainsCandidate(graph,reusedJobs,4);
  const auto pooled=pool.sample(graph,reusedJobs);
  if(pool.workerCount()!=4)throw std::runtime_error("retained worker count changed");
  for(size_t i=0;i<jobs.size();++i){if(pooled[i].error)std::rethrow_exception(pooled[i].error);if(floatBits(pooled[i].sample.expression.result)!=floatBits(reused[i].sample.expression.result)||floatBits(pooled[i].sample.input)!=floatBits(reused[i].sample.input)||pooled[i].sample.expression.success!=reused[i].sample.expression.success)throw std::runtime_error("retained chain pool mismatch");}
  for(size_t i=0;i<jobs.size();++i){if(single[i].error)std::rethrow_exception(single[i].error);if(reused[i].error)std::rethrow_exception(reused[i].error);if(single[i].sample.expression.success!=reused[i].sample.expression.success||floatBits(single[i].sample.input)!=floatBits(reused[i].sample.input)||floatBits(single[i].sample.expression.result)!=floatBits(reused[i].sample.expression.result))throw std::runtime_error("reused scalar sample mismatch");}
  auto mixed=jobs;auto invalid=jobs.at(0);invalid.expression=uint32_t(graph.elements.size());mixed.insert(mixed.begin(),invalid);mixed.push_back(jobs.at(0));
  const auto errors=samplePrivateExpressionChainsCandidate(graph,mixed,4);
  const auto pooledErrors=pool.sample(graph,mixed);
  if(!pooledErrors.at(0).error||pooledErrors.back().error)throw std::runtime_error("retained pool error isolation failed");
  const auto recovered=pool.sample(graph,reusedJobs);
  if(recovered.at(0).error||floatBits(recovered.at(0).sample.expression.result)!=floatBits(reused.at(0).sample.expression.result))throw std::runtime_error("retained pool recovery failed");
  if(!errors.at(0).error||errors.back().error||floatBits(errors.back().sample.expression.result)!=floatBits(single.at(0).sample.expression.result))throw std::runtime_error("private chain error isolation failed");
  for(size_t workers:{4u,16u}){
   const auto parallel=samplePrivateExpressionChainsCandidate(graph,jobs,workers);
   for(size_t i=0;i<jobs.size();++i){
    const auto&a=single[i];const auto&b=parallel[i];if(a.error)std::rethrow_exception(a.error);if(b.error)std::rethrow_exception(b.error);
    if(a.sample.playSuccess!=b.sample.playSuccess||a.sample.expression.success!=b.sample.expression.success||floatBits(a.sample.input)!=floatBits(b.sample.input)||floatBits(a.sample.expression.result)!=floatBits(b.sample.expression.result))throw std::runtime_error("private parallel mismatch");
   }
  }
  for(size_t i=0;i<jobs.size();++i){const auto&job=jobs[i];const auto&sample=single[i].sample;
   auto*input=runtimeAttribute(serial,job.expression,job.play.destination.attribute);auto*output=runtimeAttribute(serial,job.expression,"result");const auto savedInput=input->values;const auto savedOutput=output->values;
   bool played=executeSessionFloatPlay(serial,job.play,job.tick);bool expressed=played?executeSessionExpression(serial,job.expression):false;
   if(played!=sample.playSuccess||expressed!=sample.expression.success||(played&&(floatBits(wireFloat(input->values.at(0)))!=floatBits(sample.input)||floatBits(wireFloat(output->values.at(0)))!=floatBits(sample.expression.result))))throw std::runtime_error("private chain serial mismatch");
   input->values=savedInput;output->values=savedOutput;++comparisons;
  }
  if(!graphExactEqualCandidate(graph,serial))throw std::runtime_error("isolated chain restoration failed");chains=jobs.size();
 }
 if(!chains)throw std::runtime_error("empty chain fixture");
 if(!signedZeroFixtures)throw std::runtime_error("signed zero retention not exercised");
 for(size_t workers:{0u,65u}){bool rejected=false;try{samplePrivateExpressionChainsCandidate(graph,{},workers);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("invalid chain worker count accepted");}
 if(!samplePrivateExpressionChainsCandidate(graph,{},4).empty())throw std::runtime_error("empty chain batch failed");
 if(!pool.sample(graph,{}).empty())throw std::runtime_error("empty retained chain batch failed");
 std::cout<<"private_chains="<<chains<<" signed_zero_inputs="<<signedZeroFixtures<<" ticks=3 serial_comparisons="<<comparisons<<" worker_equivalence=1,4,16 full_graph_restoration=pass scope=frozen_graph_isolated_chains\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

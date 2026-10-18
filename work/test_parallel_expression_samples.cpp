#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "parallel_expression_sample_batch.h"
static uint32_t bits(float value){uint32_t result;std::memcpy(&result,&value,4);return result;}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: test_parallel_expression_samples session.dmx");
 auto graph=loadGraph(argv[1]);std::vector<uint32_t> jobs;std::vector<uint32_t> outputs;
 for(uint32_t i=0;i<graph.elements.size();++i)if(graph.strings.at(graph.elements[i].type)=="DmeExpressionOperator"){
  jobs.push_back(i);outputs.push_back(bits(wireFloat(runtimeAttribute(graph,i,"result")->values.at(0))));
 }
 if(jobs.empty())throw std::runtime_error("no expression fixtures");
 const auto serial=sampleExpressionBatchCandidate(graph,jobs,1);
 for(size_t workers:{2u,4u,16u,64u}){
  const auto parallel=sampleExpressionBatchCandidate(graph,jobs,workers);
  for(size_t i=0;i<jobs.size();++i){
   if(serial[i].error)std::rethrow_exception(serial[i].error);
   if(parallel[i].error)std::rethrow_exception(parallel[i].error);
   if(serial[i].sample.success!=parallel[i].sample.success||bits(serial[i].sample.result)!=bits(parallel[i].sample.result))throw std::runtime_error("parallel expression mismatch");
   if(bits(wireFloat(runtimeAttribute(graph,jobs[i],"result")->values.at(0)))!=outputs[i])throw std::runtime_error("sampling wrote output");
  }
 }
 auto mixed=jobs;mixed.insert(mixed.begin(),uint32_t(graph.elements.size()));mixed.push_back(jobs[0]);
 const auto errors=sampleExpressionBatchCandidate(graph,mixed,4);
 if(!errors[0].error||errors.back().error||bits(errors.back().sample.result)!=bits(serial[0].sample.result))throw std::runtime_error("ordered error/result isolation failed");
 for(size_t workers:{0u,65u}){bool rejected=false;try{sampleExpressionBatchCandidate(graph,jobs,workers);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("invalid worker count accepted");}
 if(!sampleExpressionBatchCandidate(graph,{},4).empty())throw std::runtime_error("empty batch failed");
 std::cout<<"expressions="<<jobs.size()<<" worker_equivalence=2,4,16,64 immutable_outputs=pass ordered_error_isolation=pass scope=frozen_saved_graph_no_commits\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}

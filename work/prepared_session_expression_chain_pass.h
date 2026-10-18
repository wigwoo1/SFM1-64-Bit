#pragma once
#include "session_expression_chain_pass.h"
#include "parallel_scalar_sample_batch.h"
#include "scalar_sampling_worker_pool.h"
#include "vector_sampling_worker_pool.h"
#include "scalar_sampling_read_guard.h"
#include "parallel_expression_sample_batch.h"
#include "parallel_private_expression_chains.h"
struct PreparedTimelineOperationProfileCandidate {
 std::array<uint64_t,4>counts{};std::array<double,4>milliseconds{};
 double playTimeMappingMilliseconds=0,playEvaluationMilliseconds=0;
 double workerPreflightMilliseconds=0,workerBatchMilliseconds=0,orderedExecutionMilliseconds=0;uint64_t workerJobs=0,workerScalarJobs=0,workerVectorJobs=0;
};
// Caller keeps runtime alive and invalidates/rebuilds before structural, endpoint,
// expression-dependency or path edits. Value-only sampling retains bindings.
class PreparedSessionExpressionChainPassCandidate {
 SessionRuntime&runtime;SessionExpressionChainPlanCandidate plan;
 std::map<uint32_t,std::vector<uint32_t>>playFrames;
 std::set<uint32_t>expressions,copies,packIds,stableScalarReads;
 std::map<uint32_t,RuntimeCachedFloatPlayBinding>plays;
 std::map<uint32_t,RuntimeCachedFloatCopyBinding>copyBindings;
 std::map<uint32_t,std::vector<AttributeIdentity>>operationWrites;
 std::map<uint32_t,uint32_t>stableExpressionPlays;
 size_t privateChainJobs_=0;
 std::unique_ptr<PrivateExpressionChainWorkerPoolCandidate>privatePool;
 std::vector<std::vector<uint32_t>>equivalentWriters;
 std::unique_ptr<ScalarSamplingWorkerPoolCandidate>scalarPool;std::unique_ptr<VectorSamplingWorkerPoolCandidate>vectorPool;
 bool valid=true;double preparationMilliseconds_=0;
 uint64_t bindingRevisionAtPreparation;size_t sampledJobs_=0,vectorSampledJobs_=0,expressionSampledJobs_=0,maxExpressionBatch_=0;
public:
 PreparedSessionExpressionChainPassCandidate(SessionRuntime&runtimeValue,const std::vector<SessionExpressionChainCandidate>&chains,const std::map<uint32_t,std::vector<uint32_t>>&typedPaths={},const std::vector<uint32_t>&colorCopies={},const std::vector<uint32_t>&packColors={}):runtime(runtimeValue),bindingRevisionAtPreparation(runtimeValue.bindingRevision){
 auto profileStart=std::chrono::steady_clock::now();plan=planSessionExpressionChainsCandidate(runtime.graph,chains);
 for(const auto&item:typedPaths){ChannelBinding binding(runtime.graph,item.first);if(binding.mode!=3||(binding.destination.scalarType!=3&&binding.destination.scalarType!=10&&binding.destination.scalarType!=13))throw std::runtime_error("unsupported additional typed channel");plan.operations.push_back(channelOperation(item.first,binding));}
 for(auto id:colorCopies){ChannelBinding binding(runtime.graph,id);if(binding.mode!=1||binding.source.scalarType!=8||binding.destination.scalarType!=8)throw std::runtime_error("unsupported additional color copy");plan.operations.push_back(channelOperation(id,binding));}
 for(auto id:packColors)plan.operations.push_back(packColorOperationCandidate(runtime.graph,id));
 if(!typedPaths.empty()||!colorCopies.empty()||!packColors.empty()){
  std::set<uint32_t>ids;std::map<AttributeIdentity,std::vector<uint32_t>>writers;plan.conflictingWriters.clear();
  for(const auto&operation:plan.operations){if(!ids.insert(operation.id).second)throw std::runtime_error("duplicate combined operation");for(const auto&output:operation.outputs)writers[output].push_back(operation.id);}
  for(const auto&item:writers)if(item.second.size()>1)plan.conflictingWriters.emplace(item);
  auto nodes=attributeDependencies(plan.operations);std::vector<uint32_t>roots;for(uint32_t i=0;i<nodes.size();++i)roots.push_back(i);plan.schedule=traverseSchedule(nodes,roots);
 }
 for(auto it=plan.conflictingWriters.begin();it!=plan.conflictingWriters.end();){if(equivalentConstantChannelWritersCandidate(runtime.graph,it->second)){equivalentWriters.push_back(it->second);it=plan.conflictingWriters.erase(it);}else ++it;}
 if(plan.schedule.cycle||!plan.conflictingWriters.empty())throw std::runtime_error("unresolved combined chain dependency policy");

 for(const auto&chain:chains){auto inserted=playFrames.emplace(chain.play,chain.frames);if(!inserted.second&&inserted.first->second!=chain.frames)throw std::runtime_error("conflicting channel time paths");expressions.insert(chain.expression);copies.insert(chain.copy);}
 for(const auto&item:typedPaths)if(!playFrames.emplace(item).second)throw std::runtime_error("duplicate channel time path");
 copies.insert(colorCopies.begin(),colorCopies.end());

 for(const auto&item:playFrames)plays.emplace(item.first,RuntimeCachedFloatPlayBinding(runtime.graph,item.first,runtime.elements,runtime.attributes));
 for(auto id:copies)copyBindings.emplace(id,RuntimeCachedFloatCopyBinding(runtime.graph,id,runtime.elements,runtime.attributes));
 packIds.insert(packColors.begin(),packColors.end());
 std::map<AttributeIdentity,std::vector<uint32_t>>writers;for(const auto&operation:plan.operations)for(const auto&output:operation.outputs)writers[output].push_back(operation.id);
 for(const auto&item:plays){if(item.second.saved.destination.scalarType!=3&&item.second.saved.destination.scalarType!=10)continue;auto reads=scalarSamplingReadsCandidate(runtime.graph,item.first,item.second.saved);for(auto frame:playFrames.at(item.first))for(const auto&a:runtime.graph.elements.at(frame).attributes)reads.insert({frame,runtime.graph.strings.at(a.name)});if(scalarSamplingReadSetStableCandidate(item.first,reads,writers))stableScalarReads.insert(item.first);}
 // Admission depends on structural read/write identities; callers rebuild on such edits.
 for(const auto&operation:plan.operations)operationWrites.emplace(operation.id,operation.outputs);
 for(const auto&chain:chains){
  const auto&destination=plays.at(chain.play).saved.destination;
  if(!stableScalarReads.count(chain.play)||destination.scalarType!=3||destination.element!=chain.expression||destination.index!=0||destination.attribute=="result")continue;
  bool stable=true;
  for(const auto&a:runtime.graph.elements.at(chain.expression).attributes){
   auto name=runtime.graph.strings.at(a.name);if(name=="result")continue;
   auto found=writers.find({chain.expression,name});if(found!=writers.end())for(auto writer:found->second)if(name!=destination.attribute||writer!=chain.play)stable=false;
  }
  if(stable){auto inserted=stableExpressionPlays.emplace(chain.expression,chain.play);if(!inserted.second&&inserted.first->second!=chain.play)stableExpressionPlays.erase(chain.expression);}
 }

 preparationMilliseconds_=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-profileStart).count();
 }
 size_t vectorSampledJobs()const{return vectorSampledJobs_;}
 size_t expressionSampledJobs()const{return expressionSampledJobs_;}
 size_t privateChainJobs()const{return privateChainJobs_;}
 size_t maxExpressionBatch()const{return maxExpressionBatch_;}
 size_t sampledJobs()const{return sampledJobs_;}
 void invalidate(){valid=false;}
 double preparationMilliseconds()const{return preparationMilliseconds_;}
 SessionExpressionChainPassResultCandidate step(int32_t rootTick,PreparedTimelineOperationProfileCandidate*profile=nullptr,size_t scalarWorkers=0,size_t vectorWorkers=0,size_t expressionWorkers=0){
 if(!valid||runtime.bindingRevision!=bindingRevisionAtPreparation)throw std::runtime_error("prepared timeline invalidated; rebuild required");
 if(scalarWorkers>64||vectorWorkers>64||expressionWorkers>64||scalarWorkers+vectorWorkers+expressionWorkers>64)throw std::runtime_error("combined sampling worker budget exceeds 64");
 auto start=std::chrono::steady_clock::now();
 for(const auto&writers:equivalentWriters)if(!equivalentConstantChannelWritersCandidate(runtime.graph,writers))throw std::runtime_error("prepared timeline writer equivalence changed; rebuild required");
 std::vector<ScalarSampleJobCandidate>jobs;std::map<uint32_t,size_t>sampleIndices;
 if(scalarWorkers){
  if(scalarWorkers>64)throw std::runtime_error("scalar worker count must be 1..64");

  for(const auto&item:plays){auto id=item.first;const auto&b=item.second;if(b.saved.destination.scalarType!=3)continue;
   try{
    if(!stableScalarReads.count(id))continue;
    if(!runtime.elements.resolve(runtime.graph,b.channelHandle)||!runtime.elements.resolve(runtime.graph,b.destinationHandle)||!runtime.elements.resolve(runtime.graph,b.logHandle)||!cachedPlayMetadataCurrent(runtime.graph,b))continue;
    bool layersLive=true;for(auto layer:b.layerHandles)if(!runtime.elements.resolve(runtime.graph,layer))layersLive=false;if(!layersLive)continue;
    auto destination=runtime.attributes.resolve(runtime.graph,runtime.elements,b.destinationAttribute);if(!destination||(destination->type!=3&&destination->type!=17)||b.saved.destination.index<0||size_t(b.saved.destination.index)>=destination->values.size())continue;
    if(b.saved.source.present()){auto source=runtime.attributes.resolve(runtime.graph,runtime.elements,b.sourceAttribute);if(!runtime.elements.resolve(runtime.graph,b.sourceHandle)||!source||(source->type!=3&&source->type!=17)||b.saved.source.index<0||size_t(b.saved.source.index)>=source->values.size())continue;}
    auto tick=channelLocalTime(runtime.graph,playFrames.at(id),rootTick,false);sampleIndices.emplace(id,jobs.size());jobs.push_back({b.saved,tick});
   }catch(const std::bad_alloc&){throw;}catch(...){/* Preserve serial validation/error position for unadmitted jobs. */}
  }
 }
 std::vector<VectorSampleJobCandidate>vectorJobs;std::map<uint32_t,size_t>vectorSampleIndices;
 if(vectorWorkers){
  if(vectorWorkers>64)throw std::runtime_error("vector worker count must be 1..64");

  for(const auto&item:plays){auto id=item.first;const auto&b=item.second;if(b.saved.destination.scalarType!=10)continue;
   try{
    if(!stableScalarReads.count(id))continue;
    if(!runtime.elements.resolve(runtime.graph,b.channelHandle)||!runtime.elements.resolve(runtime.graph,b.destinationHandle)||!runtime.elements.resolve(runtime.graph,b.logHandle)||!cachedPlayMetadataCurrent(runtime.graph,b))continue;
    bool layersLive=true;for(auto layer:b.layerHandles)if(!runtime.elements.resolve(runtime.graph,layer))layersLive=false;if(!layersLive)continue;
    auto destination=runtime.attributes.resolve(runtime.graph,runtime.elements,b.destinationAttribute);if(!destination||(destination->type!=10&&destination->type!=24)||b.saved.destination.index<0||size_t(b.saved.destination.index)>=destination->values.size())continue;
    auto tick=channelLocalTime(runtime.graph,playFrames.at(id),rootTick,false);vectorSampleIndices.emplace(id,vectorJobs.size());vectorJobs.push_back({b.saved,tick});
   }catch(const std::bad_alloc&){throw;}catch(...){/* Preserve serial validation/error position for unadmitted jobs. */}
  }
 }
 std::chrono::steady_clock::time_point preflightEnd;if(profile)preflightEnd=std::chrono::steady_clock::now();if(profile&&(scalarWorkers||vectorWorkers))profile->workerPreflightMilliseconds+=std::chrono::duration<double,std::milli>(preflightEnd-start).count();
 // Release inactive/replaced pools before new allocation to preserve total budget.
 if(scalarPool&&scalarPool->workerCount()!=scalarWorkers)scalarPool.reset();if(vectorPool&&vectorPool->workerCount()!=vectorWorkers)vectorPool.reset();if(privatePool&&privatePool->workerCount()!=expressionWorkers)privatePool.reset();
 sampledJobs_=jobs.size();if(scalarWorkers&&(!scalarPool||scalarPool->workerCount()!=scalarWorkers))scalarPool=std::make_unique<ScalarSamplingWorkerPoolCandidate>(scalarWorkers);auto samples=scalarWorkers?scalarPool->sample(runtime.graph,jobs):std::vector<ScalarSampleResultCandidate>{};
 vectorSampledJobs_=vectorJobs.size();if(vectorWorkers&&(!vectorPool||vectorPool->workerCount()!=vectorWorkers))vectorPool=std::make_unique<VectorSamplingWorkerPoolCandidate>(vectorWorkers);auto vectorSamples=vectorWorkers?vectorPool->sample(runtime.graph,vectorJobs):std::vector<VectorSampleResultCandidate>{};
 std::vector<PrivateExpressionChainJobCandidate>privateJobs;std::map<uint32_t,size_t>privateIndices;
 if(expressionWorkers)for(const auto&item:stableExpressionPlays){auto found=sampleIndices.find(item.second);if(found==sampleIndices.end())continue;const auto&sample=samples.at(found->second);if(sample.error||!sample.success)continue;float input;std::memcpy(&input,&sample.bits,4);const auto&job=jobs.at(found->second);privateIndices.emplace(item.first,privateJobs.size());privateJobs.push_back({job.binding,item.first,job.tick,true,input});}
 if(expressionWorkers&&!privateJobs.empty()&&!privatePool)privatePool=std::make_unique<PrivateExpressionChainWorkerPoolCandidate>(expressionWorkers);
 auto privateSamples=privatePool?privatePool->sample(runtime.graph,privateJobs):std::vector<PrivateExpressionChainResultCandidate>{};privateChainJobs_=privateJobs.size();
 std::chrono::steady_clock::time_point batchEnd;if(profile)batchEnd=std::chrono::steady_clock::now();if(profile&&(scalarWorkers||vectorWorkers)){profile->workerBatchMilliseconds+=std::chrono::duration<double,std::milli>(batchEnd-preflightEnd).count();profile->workerScalarJobs+=jobs.size();profile->workerVectorJobs+=vectorJobs.size();profile->workerJobs+=jobs.size()+vectorJobs.size();}
 SessionExpressionChainPassResultCandidate result;
 expressionSampledJobs_=0;maxExpressionBatch_=0;
 std::vector<ExpressionSampleResultCandidate>expressionSamples;std::map<size_t,size_t>expressionSampleIndices;size_t expressionBatchEnd=0;
 for(size_t orderIndex=0;orderIndex<plan.schedule.order.size();++orderIndex){auto id=plan.schedule.order[orderIndex];bool success=false;unsigned category=0;std::chrono::steady_clock::time_point operationStart;if(profile)operationStart=std::chrono::steady_clock::now();auto play=plays.find(id);
  auto sampled=sampleIndices.find(id);
  if(sampled!=sampleIndices.end()){const auto&sample=samples.at(sampled->second);if(sample.error)std::rethrow_exception(sample.error);success=sample.success;if(success){float value;std::memcpy(&value,&sample.bits,4);const auto&destination=play->second.saved.destination;runtimeWriteFloat(runtime.graph,destination.element,destination.attribute,destination.index,value,&result.changes);}}
  else if(vectorSampleIndices.count(id)){const auto&sample=vectorSamples.at(vectorSampleIndices.at(id));if(sample.error)std::rethrow_exception(sample.error);success=sample.success;if(success){std::array<float,3>value;std::memcpy(value.data(),sample.bits.data(),12);runtimeWriteVector(runtime.graph,play->second.saved.destination,value,&result.changes);}}
  else if(play!=plays.end()){
   std::chrono::steady_clock::time_point mappingStart,mappingEnd;if(profile)mappingStart=std::chrono::steady_clock::now();
   auto localTick=channelLocalTime(runtime.graph,playFrames.at(id),rootTick,false);if(profile)mappingEnd=std::chrono::steady_clock::now();
   success=executeRuntimeTypedPlay(runtime.graph,runtime.elements,runtime.attributes,play->second,localTick,&result.changes);
   if(profile){auto evaluationEnd=std::chrono::steady_clock::now();profile->playTimeMappingMilliseconds+=std::chrono::duration<double,std::milli>(mappingEnd-mappingStart).count();profile->playEvaluationMilliseconds+=std::chrono::duration<double,std::milli>(evaluationEnd-mappingEnd).count();}
  }
  else if(expressions.count(id)){category=1;
   if(privateIndices.count(id)){
    if(runtime.elements.resolve(runtime.graph,runtime.elements.graphHandles.at(id))){const auto&sample=privateSamples.at(privateIndices.at(id));if(sample.error)std::rethrow_exception(sample.error);if(sample.sample.playSuccess){runtimeWriteFloat(runtime.graph,id,"result",0,sample.sample.expression.result,&result.changes);success=sample.sample.expression.success;}}
   }else if(expressionWorkers){
    if(orderIndex>=expressionBatchEnd){
     expressionBatchEnd=orderIndex;expressionSampleIndices.clear();
     std::vector<uint32_t>expressionJobs;
     std::set<AttributeIdentity>interveningWrites;
     // Sampling can cross other operations only when their declared writes do
     // not affect this expression's reads. Include formula/schema attributes
     // conservatively; expression outputs are committed at their original index.
     while(expressionBatchEnd<plan.schedule.order.size()){
      auto candidate=plan.schedule.order[expressionBatchEnd];
      if(expressions.count(candidate)){
       bool stable=true;for(const auto&a:runtime.graph.elements.at(candidate).attributes){auto name=runtime.graph.strings.at(a.name);if(name!="result"&&interveningWrites.count({candidate,name})){stable=false;break;}}
       if(!stable)break;
       expressionSampleIndices.emplace(expressionBatchEnd,expressionJobs.size());expressionJobs.push_back(candidate);
      }
      const auto&outputs=operationWrites.at(candidate);interveningWrites.insert(outputs.begin(),outputs.end());++expressionBatchEnd;
     }
     // A retained private pool already consumes the expression worker budget.
     // Unsupported look-ahead fallback stays on the caller in that case.
     expressionSamples=sampleExpressionBatchCandidate(runtime.graph,expressionJobs,privatePool?1:expressionWorkers);
     expressionSampledJobs_+=expressionJobs.size();maxExpressionBatch_=std::max(maxExpressionBatch_,expressionJobs.size());
    }
    if(runtime.elements.resolve(runtime.graph,runtime.elements.graphHandles.at(id))){
     const auto&sample=expressionSamples.at(expressionSampleIndices.at(orderIndex));if(sample.error)std::rethrow_exception(sample.error);
     runtimeWriteFloat(runtime.graph,id,"result",0,sample.sample.result,&result.changes);success=sample.sample.success;
    }
   }else success=executeRuntimeExpression(runtime.graph,runtime.elements,runtime.elements.graphHandles.at(id),&result.changes);
  }
  else if(packIds.count(id)){category=2;if(runtime.elements.resolve(runtime.graph,runtime.elements.graphHandles.at(id)))success=executeSessionPackColorCandidate(runtime.graph,id,&result.changes);}
  else if(copies.count(id)){category=3;success=executeRuntimeTypedCopy(runtime.graph,runtime.elements,runtime.attributes,copyBindings.at(id),&result.changes);}
  if(!success)throw std::runtime_error("combined expression chain operation failed");if(profile){++profile->counts[category];profile->milliseconds[category]+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-operationStart).count();}result.executed.push_back(id);
 }
 if(profile)profile->orderedExecutionMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-batchEnd).count();
 result.operationMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return result;
 }
};

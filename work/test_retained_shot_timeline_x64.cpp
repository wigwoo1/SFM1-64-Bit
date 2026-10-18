#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_shot_timeline_candidate.h"
#include "session_color_timeline_subset.h"
#include <iostream>
bool sameGraph(const Graph&a,const Graph&b){
 if(a.header!=b.header||a.strings!=b.strings||a.elements.size()!=b.elements.size())return false;
 for(size_t i=0;i<a.elements.size();++i){auto&x=a.elements[i];auto&y=b.elements[i];if(x.type!=y.type||x.name!=y.name||x.id!=y.id||x.attributes.size()!=y.attributes.size())return false;
  for(size_t j=0;j<x.attributes.size();++j){auto&u=x.attributes[j];auto&v=y.attributes[j];if(u.name!=v.name||u.type!=v.type||u.values.size()!=v.values.size())return false;for(size_t k=0;k<u.values.size();++k){auto&p=u.values[k];auto&q=v.values[k];if(p.wire!=q.wire||p.text!=q.text||p.reference!=q.reference||p.stringIndex!=q.stringIndex)return false;}}
 }return true;
}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("session required");auto baseline=loadGraph(argv[1]);RetainedShotTimelineCandidate serial(baseline,2237),parallel(baseline,2237);size_t frames=0,rebuilds=0,jobs=0;
 for(auto tick:{870833,875833,880833,870833,885000,890000,880833}){
  auto selected=selectFilmShotCandidate(baseline,2237,tick);auto fresh=evaluateSessionColorTimelineSubsetCandidate(baseline,selected.parentTick,selected.clip);
  auto a=serial.step(tick);auto b=parallel.step(tick,2,2,4);
  if(!sameGraph(serial.graph(),fresh.graph)||!sameGraph(parallel.graph(),serial.graph()))throw std::runtime_error("retained shot graph differs from fresh serial evaluation");
  if(a.shot.clip!=selected.clip||b.shot.localTick!=selected.localTick||a.rebuilt!=b.rebuilt)throw std::runtime_error("selected-shot state mismatch");
  ++frames;rebuilds+=a.rebuilt;jobs+=b.scalarJobs+b.vectorJobs;
  std::cout<<"tick="<<tick<<" shot="<<a.shot.clip<<" rebuilt="<<a.rebuilt<<" executed="<<a.executed<<" scalar_jobs="<<b.scalarJobs<<" vector_jobs="<<b.vectorJobs<<" fresh_graph_equal=1\n";
 }
 if(rebuilds!=3||!jobs)throw std::runtime_error("retention or parallel sampling absent");
 auto overlap=parallel.step(1174550,2,2,4);auto serialOverlap=serial.step(1174550);if(overlap.shot.clip!=15477||serialOverlap.shot.clip!=15477||!sameGraph(parallel.graph(),serial.graph()))throw std::runtime_error("native-order overlap selection mismatch");
 parallel.step(880833,2,2,4);
 bool rejected=false;try{parallel.step(INT32_MAX,2,2,4);}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("timeline gap admitted");
 auto repeat=parallel.step(880833,2,2,4);if(repeat.rebuilt)throw std::runtime_error("rejected selection destroyed retained shot");
 auto before=parallel.graph();bool budgetRejected=false;try{parallel.step(880833,2,2,61);}catch(const std::runtime_error&){budgetRejected=true;}if(!budgetRejected||!sameGraph(before,parallel.graph()))throw std::runtime_error("expression budget rejection mutated shot");
 std::cout<<"retained selected-shot timeline passed frames="<<frames<<" rebuilds="<<rebuilds<<" sampled_jobs="<<jobs<<" backwards_seek=verified shot_transition=baseline_reset native_scheduler=unfinished\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "session_model_illumination_dag_candidate.h"
#include <iomanip>
#include <iostream>
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 audit required");if(argc!=2)throw std::runtime_error("session.dmx required");auto graph=loadGraph(argv[1]);const auto root=sessionRenderRootCandidate(graph);RetainedShotTimelineCandidate timeline(std::move(graph),root);const auto step=evaluateLiveSessionSceneCandidate(timeline,50000,1280,720);const auto&g=timeline.graph();auto binding=readFilmSceneRenderBindingCandidate(g,step.scene.shot);GraphTransformHierarchy hierarchy(g,binding.scene);
 std::set<uint32_t>scope;std::vector<uint32_t>pending{binding.scene};while(!pending.empty()){auto id=pending.back();pending.pop_back();if(!scope.insert(id).second)continue;for(auto child:hierarchyRefs(g,g.elements.at(id),"children",15))pending.push_back(child);}
 size_t selected=0,missing=0;std::cout<<std::setprecision(9)<<"tick=50000 shot="<<step.scene.shot<<" scene="<<binding.scene<<" models="<<step.scene.models.size()<<'\n';
 for(const auto&model:step.scene.models){auto result=sessionModelIlluminationDagCandidate(g,model.id,[&](uint32_t id){const auto&type=g.strings.at(g.elements.at(id).type);if(type=="DmeDag")return true;throw std::runtime_error("audit type ancestry UNVERIFIED for "+type);});std::cout<<"model="<<model.id<<" asset="<<model.placement.asset<<" attribute_present="<<result.attributePresent<<" first_child_attempted="<<result.firstChildAttempted;
  if(result.dag){const auto id=*result.dag;if(!scope.count(id))throw std::runtime_error("external illumination DAG scope UNVERIFIED");const auto matrix=hierarchy.world(id);std::array<float,3>point{matrix[0][3],matrix[1][3],matrix[2][3]};for(float v:point)if(!std::isfinite(v))throw std::runtime_error("nonfinite diagnostic illumination point");std::cout<<" dag="<<id<<" point="<<point[0]<<','<<point[1]<<','<<point[2];++selected;}else{std::cout<<" dag=none";++missing;}
  std::cout<<'\n';
 }
 std::cout<<"PASS selected="<<selected<<" missing="<<missing<<" graph_hierarchy_positions=diagnostic native_session_binding=UNVERIFIED renderer_changed=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

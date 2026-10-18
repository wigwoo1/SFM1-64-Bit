#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "retained_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#include "session_timeline_frames_candidate.h"
#include "retained_film_track_candidate.h"
#include "gameinfo_mounts_candidate.h"
#include "read_binary_asset_candidate.h"
#include <iostream>
#include <cstring>

static AnimationTimeFrame clipFrame(const Graph&g,uint32_t clip,uint32_t&frameId){
 auto refs=hierarchyRefs(g,g.elements.at(clip),"timeFrame",1);if(refs.size()!=1)throw std::runtime_error("benchmark shot timeframe missing");frameId=refs.front();return AnimationTimeFrame(g,g.elements.at(frameId));
}
static bool sameValue(const Value&a,const Value&b){return a.wire==b.wire&&a.reference==b.reference&&a.text==b.text;}
static void samePreparedGeometry(Graph graph,uint32_t root,const std::vector<std::filesystem::path>&mounts,int32_t tick,const std::vector<SceneGpuVertexCandidate>&expectedVertices,const std::vector<uint32_t>&expectedIndices){
 RetainedSessionSceneCandidate scene(std::move(graph),root,mounts,4,true,true);auto frame=scene.sample(tick);
 if(frame.upload.vertices.size()!=expectedVertices.size()||frame.upload.indices!=expectedIndices)throw std::runtime_error("reopened scene geometry counts or indices differ from source session");
 for(size_t i=0;i<expectedVertices.size();++i)if(std::memcmp(&frame.upload.vertices[i],&expectedVertices[i],sizeof(SceneGpuVertexCandidate)))throw std::runtime_error("reopened scene vertex differs from source session at index "+std::to_string(i));
}
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("usage: test_mtt_soldier_editor_roundtrip INPUT.dmx SAVED_COPY.dmx gameinfo.txt");
 auto original=loadGraph(argv[1]);auto saved=loadGraph(argv[2]);
 const auto originalRoot=sessionRenderRootCandidate(original),savedRoot=sessionRenderRootCandidate(saved);
 if(original.elements.size()!=saved.elements.size()||original.strings!=saved.strings||originalRoot!=savedRoot)throw std::runtime_error("save copy changed session structure or string table");
 const auto frames=sessionTimelineFramesCandidate(original,originalRoot);if(frames.size()<=119)throw std::runtime_error("benchmark has no frame 119");const int32_t tick=frames.tick(119);
 RetainedFilmTrackCandidate sourceTrack(original,originalRoot),savedTrack(saved,savedRoot);auto sourceShot=sourceTrack.select(tick),savedShot=savedTrack.select(tick);
 if(!sourceShot||!savedShot||sourceShot->clip!=savedShot->clip)throw std::runtime_error("edited copy no longer selects the same benchmark shot");
 uint32_t sourceFrameId=UINT32_MAX,savedFrameId=UINT32_MAX;const auto before=clipFrame(original,sourceShot->clip,sourceFrameId),after=clipFrame(saved,savedShot->clip,savedFrameId);
 if(sourceFrameId!=savedFrameId)throw std::runtime_error("benchmark shot identity changed");const int64_t delta=int64_t(sourceShot->parentTick)-before.start;
 if(delta<=0||after.start!=sourceShot->parentTick||after.duration!=before.duration-delta||after.offset!=before.offset+delta)throw std::runtime_error("saved copy does not contain the expected frame-119 shot trim");
 size_t changedValues=0;for(size_t e=0;e<original.elements.size();++e){const auto&a=original.elements[e];const auto&b=saved.elements[e];if(a.id!=b.id||a.type!=b.type||a.name!=b.name||a.attributes.size()!=b.attributes.size())throw std::runtime_error("save copy changed an unrelated DMX element");for(size_t n=0;n<a.attributes.size();++n){const auto&x=a.attributes[n];const auto&y=b.attributes[n];if(x.name!=y.name||x.type!=y.type||x.values.size()!=y.values.size())throw std::runtime_error("save copy changed DMX attribute shape");for(size_t v=0;v<x.values.size();++v)if(!sameValue(x.values[v],y.values[v])){++changedValues;if(e!=sourceFrameId||x.values.size()!=1)throw std::runtime_error("save copy changed data outside the edited shot timeframe");const auto&name=original.strings.at(x.name);if(name!="start"&&name!="duration"&&name!="offset")throw std::runtime_error("save copy changed an unexpected shot field");}}}
 if(changedValues!=3)throw std::runtime_error("expected exactly three shot trim values to change");
 auto bytes=readBinaryAssetCandidate(argv[3]);std::string gameinfo(bytes.begin(),bytes.end());auto content=std::filesystem::path(argv[3]).parent_path();auto mounts=gameinfoMountsCandidate(gameinfo,content,content.parent_path());
 std::vector<SceneGpuVertexCandidate>vertices;std::vector<uint32_t>indices;{
  RetainedSessionSceneCandidate scene(std::move(original),originalRoot,mounts,4,true,true);auto frame=scene.sample(tick);vertices=std::move(frame.upload.vertices);indices=std::move(frame.upload.indices);
 }
 samePreparedGeometry(std::move(saved),savedRoot,mounts,tick,vertices,indices);
 std::cout<<"mtt_editor_save_copy_roundtrip=passed frame=119 tick="<<tick<<" shot_trim=start,duration,offset unrelated_dmx_values_changed=0 prepared_geometry_exact=1 vertices="<<vertices.size()<<" indices="<<indices.size()<<" rendered_pixels=unverified\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#define SFM64_EDITOR_HDR2_PROFILE_ENTRY baseHdr2ProfileFixtureMain
#include "test_editor_hdr2_profile.cpp"
#include "session_timeline_document_candidate.h"
#include <limits>

static Value exposureFloat(float x){Value v;v.wire.resize(4);std::memcpy(v.wire.data(),&x,4);return v;}
static Value exposureInt(int32_t x){Value v;v.wire.resize(4);std::memcpy(v.wire.data(),&x,4);return v;}
static Value exposureRef(uint32_t x){Value v;v.reference=x;return v;}
static Value exposureText(Graph& g,const char* x){Value v;v.text=x;v.stringIndex=internString(g,x);return v;}
static void exposurePut(Graph& g,uint32_t id,const char* key,uint8_t type,std::vector<Value> values){
 const auto name=internString(g,key);auto& attributes=g.elements.at(id).attributes;
 auto it=std::find_if(attributes.begin(),attributes.end(),[&](const auto& a){return a.name==name;});
 Attribute next{name,type,std::move(values)};if(it==attributes.end())attributes.push_back(std::move(next));else *it=std::move(next);
}
static uint32_t exposureElement(Graph& g,const char* type){Element e{};e.type=internString(g,type);e.name=e.type;const auto id=uint32_t(g.elements.size()),guid=id+1;std::memcpy(e.id.data(),&guid,4);g.elements.push_back(e);return id;}
static Graph exposurePortable(Graph g){
 g.header="<!-- dmx encoding binary 5 format sfm_session 20 -->";
 std::vector<uint32_t> order{5};for(uint32_t n=0;n<g.elements.size();++n)if(n!=5)order.push_back(n);
 std::vector<uint32_t> remap(order.size());for(uint32_t n=0;n<order.size();++n)remap[order[n]]=n;
 const auto previous=g.elements;for(size_t n=0;n<order.size();++n)g.elements[n]=previous[order[n]];
 for(auto& e:g.elements)for(auto& a:e.attributes)for(auto& v:a.values){if((a.type==1||a.type==15)&&v.reference<remap.size())v.reference=remap[v.reference];if(a.type==5)v.stringIndex=internString(g,v.text);}
 return g;
}
static void exposureAnimate(Graph& g){
 const auto group=exposureElement(g,"DmeTrackGroup"),track=exposureElement(g,"DmeTrack"),clip=exposureElement(g,"DmeChannelsClip"),frame=exposureElement(g,"DmeTimeFrame");
 exposurePut(g,0,"trackGroups",15,{exposureRef(group)});exposurePut(g,group,"tracks",15,{exposureRef(track)});exposurePut(g,track,"children",15,{exposureRef(clip)});exposurePut(g,clip,"timeFrame",1,{exposureRef(frame)});
 exposurePut(g,track,"clipType",2,{exposureInt(0)});
 for(auto id:{4u,frame}){exposurePut(g,id,"start",7,{exposureInt(0)});exposurePut(g,id,"duration",7,{exposureInt(30000)});exposurePut(g,id,"offset",7,{exposureInt(0)});exposurePut(g,id,"scale",3,{exposureFloat(1)});}
 std::vector<Value> channels;
 for(uint32_t target:{2u,7u}){
  const auto channel=exposureElement(g,"DmeChannel"),log=exposureElement(g,"DmeFloatLog"),layer=exposureElement(g,"DmeFloatLogLayer");channels.push_back(exposureRef(channel));
  exposurePut(g,channel,"mode",2,{exposureInt(3)});exposurePut(g,channel,"fromElement",1,{exposureRef(UINT32_MAX)});exposurePut(g,channel,"fromAttribute",5,{exposureText(g,"")});exposurePut(g,channel,"fromIndex",2,{exposureInt(0)});
  exposurePut(g,channel,"toElement",1,{exposureRef(target)});exposurePut(g,channel,"toAttribute",5,{exposureText(g,"toneMapScale")});exposurePut(g,channel,"toIndex",2,{exposureInt(0)});exposurePut(g,channel,"log",1,{exposureRef(log)});
  exposurePut(g,log,"layers",15,{exposureRef(layer)});exposurePut(g,log,"defaultvalue",3,{exposureFloat(1)});Value flag;flag.wire={0};exposurePut(g,log,"usedefaultvalue",4,{flag});
  exposurePut(g,layer,"times",21,{exposureInt(0),exposureInt(10000),exposureInt(20000)});const float factor=target==2?1.f:.5f;exposurePut(g,layer,"values",17,{exposureFloat(factor),exposureFloat(factor*2),exposureFloat(factor*4)});
 }
 exposurePut(g,clip,"channels",15,std::move(channels));
}
int main(int argc,char** argv){try{
 check(argc==3||argc==4,"fresh fixture directory, generated VTF and optional disposable benchmark copy");
 check(baseHdr2ProfileFixtureMain(3,argv)==0,"existing explicit-profile regression");
 const std::filesystem::path root=argv[1];auto g=profileGraph("owned");const auto saved=g;
 auto initial=readSessionExposureCandidate(g,0,2);check(initial.product==1&&!initial.sessionAttribute&&!initial.cameraAttribute,"missing exposure defaults");
 exposurePut(g,7,"toneMapScale",3,{exposureFloat(.5f)});exposurePut(g,2,"toneMapScale",3,{exposureFloat(2)});
 auto read=[&]{return readSessionExposureCandidate(g,0,2);};auto first=read();check(first.sessionScale==.5f&&first.cameraScale==2&&first.product==1&&first.renderSettings==7,"session/camera input product");
 auto other=g.elements[2];other.id[0]=9;g.elements.push_back(other);exposurePut(g,8,"toneMapScale",3,{exposureFloat(4)});check(readSessionExposureCandidate(g,0,8).product==2&&read().camera==2,"active camera identity");
 size_t refused=0;auto bad=[&](const auto& edit){auto corrupt=g;edit(corrupt);refuses([&]{readSessionExposureCandidate(corrupt,0,2);},"invalid exposure graph accepted");++refused;};
 for(float value:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})bad([&](auto& x){exposurePut(x,2,"toneMapScale",3,{exposureFloat(value)});});
 bad([](auto& x){exposurePut(x,2,"toneMapScale",2,{exposureInt(1)});});bad([](auto& x){exposurePut(x,2,"toneMapScale",3,{});});bad([](auto& x){Value v;v.wire={1};exposurePut(x,2,"toneMapScale",3,{v});});
 bad([](auto& x){x.elements[2].attributes.push_back(*namedAttribute(x,x.elements[2],"toneMapScale"));});bad([](auto& x){x.elements.push_back(x.elements[5]);});
 bad([](auto& x){exposurePut(x,6,"renderSettings",1,{exposureRef(999999)});});
 bad([](auto& x){exposurePut(x,7,"toneMapScale",3,{exposureFloat(std::numeric_limits<float>::max())});});
 bad([](auto& x){exposurePut(x,2,"toneMapScale",3,{exposureFloat(std::numeric_limits<float>::min())});exposurePut(x,7,"toneMapScale",3,{exposureFloat(std::numeric_limits<float>::min())});});
 for(const auto* key:{"settings","renderSettings"}){auto x=g;const auto id=std::string(key)=="settings"?5u:6u;exposurePut(x,id,key,1,{exposureRef(UINT32_MAX)});check(readSessionExposureCandidate(x,0,2).product==2,"null settings fallback");}
 auto modeText=profileText();modeText.replace(modeText.find("exposure 1"),10,"exposure session_camera");std::istringstream input(modeText);const auto mode=readEditorHdr2ProfileCandidate(input);std::istringstream manualInput(profileText());const auto manual=readEditorHdr2ProfileCandidate(manualInput);check(mode->usesSessionExposure()&&mode->identity()!=manual->identity(),"exposure source missing from profile identity");
 {std::ofstream f(root/"session-camera.profile");f<<modeText;}
 // Use the existing portable generated session so camera geometry and source
 // files are identical to the actual editor's controlled output fixtures.
 auto portable=loadGraph((root/"owned.dmx").string().c_str());exposurePut(portable,7,"toneMapScale",3,{exposureFloat(.5f)});exposurePut(portable,3,"toneMapScale",3,{exposureFloat(2)});
 // portable order is [session,film,dag,camera,transform,time,settings,render].
 check(readSessionExposureCandidate(portable,1,3).product==1,"portable exposure identity");
 SessionTimelineDocumentCandidate doc(portable);check(!doc.dirty(),"new document dirty");check(doc.setScalarFloat(3,"toneMapScale",4)&&doc.dirty(),"camera exposure document edit");check(readSessionExposureCandidate(doc.graph(),1,3).product==2,"edited camera exposure");check(doc.undo()==1&&!doc.dirty()&&readSessionExposureCandidate(doc.graph(),1,3).product==1,"exposure undo");check(doc.redo()==1&&doc.dirty(),"exposure redo");check(!doc.saveCopy(nullptr)&&doc.dirty(),"cancel changed exposure dirty state");
 const auto copy=root/"exposure-copy.dmx";doc.saveCopy(copy.string().c_str());check(!doc.dirty()&&readSessionExposureCandidate(loadGraph(copy.string().c_str()),1,3).product==2,"save copy exposure reload");
 auto invalidSaved=portable;exposurePut(invalidSaved,7,"toneMapScale",3,{exposureFloat(0)});saveGraph(invalidSaved,(root/"exposure-invalid.dmx").string().c_str());
 // Add keys on both inputs. Read the evaluated graph, never the saved baseline.
 exposureAnimate(g);auto animated=exposurePortable(g);
 // Retain the portable fixture's already-tested camera transform for GPU checks.
 animated.elements[4]=portable.elements[4];
 const auto animatedPath=root/"exposure-animated.dmx";saveGraph(animated,animatedPath.string().c_str());
 RetainedSessionSceneCandidate owner(animated,1,{root},1,false,false,mode);std::shared_ptr<const SceneHdr2PreparedFrameCandidate> kept;
 for(const auto& row:std::vector<std::pair<int32_t,float>>{{0,.5f},{10000,2.f},{20000,8.f},{0,.5f},{20000,8.f},{10000,2.f}}){const auto frame=owner.samplePlayback(row.first,64,32);check(frame.sessionExposure&&frame.sessionExposure->camera==3&&frame.sessionExposure->product==row.second&&frame.hdr2->plan()->policy.exposure==row.second,"evaluated retained exposure/seek");if(!kept)kept=frame.hdr2;}
 check(kept->plan()->policy.exposure==.5f&&mode->policy().exposure==1,"later frame mutated retained exposure/profile");
 auto switched=animated;exposurePut(switched,1,"camera",1,{exposureRef(8)});owner.replaceSession(switched,1);check(owner.sample(0,64,32).sessionExposure->camera==8&&owner.sample(0,64,32).hdr2->plan()->policy.exposure==2,"camera switch/reload exposure");owner.replaceSession(animated,1);
 FilmTimelineStateCandidate state;const auto blank=owner.sampleTimeline(30000,64,32,state);check(state==FilmTimelineStateCandidate::Gap&&!blank.sessionExposure&&blank.hdr2->plan()->policy.exposure==1&&blank.hdr2->plan()->faces.empty(),"blank retained stale exposure");check(owner.sample(0,64,32).hdr2->plan()->policy.exposure==.5f,"blank restore exposure");
 auto invalid=animated;exposurePut(invalid,7,"toneMapScale",3,{exposureFloat(0)}); // Remove animation of this input so the bad saved scalar survives evaluation.
 const auto group=hierarchyRefs(invalid,invalid.elements[1],"trackGroups",15).front();const auto track=hierarchyRefs(invalid,invalid.elements[group],"tracks",15).front();const auto clip=hierarchyRefs(invalid,invalid.elements[track],"children",15).front();auto channels=hierarchyRefs(invalid,invalid.elements[clip],"channels",15);exposurePut(invalid,clip,"channels",15,{exposureRef(channels.front())});owner.replaceSession(invalid,1);refuses([&]{owner.sample(0,64,32);},"unsupported evaluated exposure accepted");check(kept->plan()->policy.exposure==.5f,"failed preparation changed prior frame");
 RetainedSessionSceneCandidate numeric(invalid,1,{root},1,false,false,manual);check(numeric.sample(0,64,32).hdr2->plan()->policy.exposure==1,"numeric override inspected session scale");
 check(graphExactEqualCandidate(animated,loadGraph(animatedPath.string().c_str())),"sampling modified fixture input");check(graphExactEqualCandidate(saved,profileGraph("owned")),"baseline helper graph changed");
 if(argc==4){const auto benchmark=loadGraph(argv[3]);const auto film=sessionRenderRootCandidate(benchmark);RetainedShotTimelineCandidate timeline(benchmark,film);for(int32_t tick:{49583,50000,49583}){const auto step=evaluateLiveSessionSceneCandidate(timeline,tick,512,288,0,0,0);const auto e=readSessionExposureCandidate(timeline.graph(),film,step.scene.camera);std::cout<<"benchmark_tick="<<tick<<" camera="<<e.camera<<" session="<<e.sessionScale<<" camera_scale="<<e.cameraScale<<" product="<<e.product<<'\n';}check(graphExactEqualCandidate(benchmark,loadGraph(argv[3])),"benchmark copy changed");}
 std::cout<<"PASS session_exposure=1 rejected_graphs="<<refused<<" animation_seek=6 camera_switch=1 blank_restore=1 numeric_override=1 document_edit_undo_redo_cancel_save_copy_reload=1 original_runtime=ABSENT\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

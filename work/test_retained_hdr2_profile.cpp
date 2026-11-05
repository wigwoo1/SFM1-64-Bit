#define SFM64_HDR2_PLAN_NO_MAIN
#include "test_scene_hdr2_lightmap.cpp"
#include "retained_session_scene_candidate.h"
#include "graph_exact_equal.h"
#include <future>

static Graph profileGraph(const char* map){
 Graph g;
 auto name=[&](const char* text){auto it=std::find(g.strings.begin(),g.strings.end(),text);if(it!=g.strings.end())return uint32_t(it-g.strings.begin());g.strings.emplace_back(text);return uint32_t(g.strings.size()-1);};
 auto element=[&](const char* type){Element e{};e.type=name(type);e.name=e.type;e.id[0]=char(g.elements.size()+1);g.elements.push_back(e);};
 for(const char* type:{"DmeFilmClip","DmeDag","DmeCamera","DmeTransform","DmeTimeFrame","DmElement","DmElement","DmElement"})element(type);
 auto ref=[](uint32_t id){Value v;v.reference=id;return v;};
 auto words=[](std::initializer_list<float> values){Value v;v.wire.resize(values.size()*4);std::memcpy(v.wire.data(),values.begin(),v.wire.size());return v;};
 auto integer=[](int32_t n){Value v;v.wire.resize(4);std::memcpy(v.wire.data(),&n,4);return v;};
 auto attr=[&](uint32_t id,const char* key,uint8_t type,Value v){g.elements.at(id).attributes.push_back({name(key),type,{std::move(v)}});};
 attr(0,"camera",1,ref(2));attr(0,"scene",1,ref(1));attr(0,"timeFrame",1,ref(4));Value m;m.text=map;attr(0,"mapname",5,m);
 attr(2,"transform",1,ref(3));attr(2,"fieldOfView",3,words({60}));attr(2,"znear",3,words({1}));attr(2,"zfar",3,words({100}));Value flag;flag.wire={0};attr(2,"ortho",4,flag);
 attr(3,"position",10,words({-10,0,0}));attr(3,"orientation",13,words({0,0,0,1}));
 attr(4,"start",7,integer(0));attr(4,"duration",7,integer(10000));attr(4,"offset",7,integer(0));attr(4,"scale",3,words({1}));
 attr(5,"activeClip",1,ref(0));attr(5,"settings",1,ref(6));attr(6,"renderSettings",1,ref(7));attr(7,"frameRate",3,words({24}));return g;
}
static void sameFrame(const PreparedSessionSceneCandidate& a,const PreparedSessionSceneCandidate& b){
 check(a.camera.rows==b.camera.rows&&a.materialAlpha==b.materialAlpha&&a.upload.indices==b.upload.indices&&a.upload.vertices.size()==b.upload.vertices.size(),"legacy/profile frame comparison");
 for(size_t n=0;n<a.upload.vertices.size();++n){const auto&x=a.upload.vertices[n];const auto&y=b.upload.vertices[n];check(x.position==y.position&&x.normal==y.normal&&x.uv==y.uv&&x.displacementAlpha==y.displacementAlpha,"frame geometry changed");}
}
#ifndef SFM64_RETAINED_HDR2_PROFILE_NO_MAIN
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"native x64 required");
 static_assert(!std::is_copy_assignable<SceneHdr2ProfileCandidate>::value,"profile must be immutable");
 static_assert(!std::is_copy_assignable<SceneHdr2PreparedFrameCandidate>::value,"frame pair must be immutable");
 check(argc==3,"fresh fixture directory and owned VTF required");const std::filesystem::path root=argv[1];check(std::filesystem::create_directory(root),"fresh fixture directory");std::filesystem::create_directories(root/"maps");std::filesystem::create_directories(root/"materials/owned");putFile(root/"materials/owned/pixel.vtf",readBinaryAssetCandidate(argv[2]));
 for(const char* n:{"mat","other"}){std::ofstream f(root/"materials"/(std::string(n)+".vmt"));f<<"LightmappedGeneric { $basetexture owned/pixel }";}
 auto bytes=threeFaces();
 // The shared CPU-plan fixture only exercised HDR: its model spans three
 // faces while its old LDR table has one. Supply three valid LDR triangles
 // here so omitted-profile comparisons test valid, differing topology.
 const auto ldrAt=get32(bytes,8+7*16);std::vector<uint8_t> ldr(4*56);
 for(size_t i=1;i<4;++i)std::copy_n(bytes.data()+ldrAt+56,56,ldr.data()+i*56);
 replace(bytes,7,ldr,1);putFile(root/"maps/a.bsp",bytes);putFile(root/"maps/b.bsp",bytes);
 auto invalid=bytes;u32(invalid,12+58*16,0);putFile(root/"maps/invalid.bsp",invalid);
 auto unsupported=bytes;const auto at=get32(unsupported,8+58*16);for(size_t i=1;i<4;++i){unsupported[at+i*56+16]=255;u32(unsupported,at+i*56+20,uint32_t(-1));}putFile(root/"maps/unsupported.bsp",unsupported);
 SourceHdr2ScenePolicyCandidate policy{2,113,.5f,{.5f,.75f,1,1},0,2};
 auto profile=std::make_shared<const SceneHdr2ProfileCandidate>(policy,1u,uint8_t(0),SceneHdr2FilterCandidate::Linear);auto identical=SceneHdr2ProfileCandidate(policy,1,0,SceneHdr2FilterCandidate::Linear);
 check(profile->identity()==identical.identity()&&profile->dataset()==SourceBspLightmapSelectionCandidate::Hdr,"stable explicit identity/dataset");
 const auto identity=profile->identity();policy.styleScale=4;check(profile->policy().styleScale==.5f&&profile->identity()==identity,"profile borrowed mutable caller policy");policy=profile->policy();
 size_t different=0;for(int i=0;i<10;++i){auto p=policy;uint32_t pad=1;uint8_t initial=0;auto filter=SceneHdr2FilterCandidate::Linear;if(i==0)p.encoding=36;if(i==1)p.styleScale=.25f;if(i>=2&&i<=4)p.instance[size_t(i-2)]=.25f;if(i==5)p.configurationByte49=2;if(i==6)p.exposure=1;if(i==7)pad=0;if(i==8)initial=0xa5;if(i==9)filter=SceneHdr2FilterCandidate::Nearest;check(SceneHdr2ProfileCandidate(p,pad,initial,filter).identity()!=identity,"profile identity omitted policy field");++different;}
 size_t refused=0;for(int i=0;i<8;++i){auto p=policy;auto filter=SceneHdr2FilterCandidate::Linear;uint32_t pad=1;if(i==0)p.selectedHdrType=1;if(i==1)p.encoding=99;if(i==2)p.styleScale=-1;if(i==3)p.instance[3]=.5f;if(i==4)p.instance[0]=std::numeric_limits<float>::quiet_NaN();if(i==5)p.exposure=0;if(i==6)pad=16384;if(i==7)filter=SceneHdr2FilterCandidate(99);refuses([&]{SceneHdr2ProfileCandidate rejected(p,pad,0,filter);},"invalid profile accepted");++refused;}
 refuses([&]{profile->page(32768,1);},"page padding overflow accepted");refuses([&]{profile->page(0,1);},"zero face accepted");
 const auto graph=profileGraph("a");const auto saved=graph;
 RetainedSessionSceneCandidate legacy(graph,0,{root},1),explicitLegacy(graph,0,{root},1,false,false,{});
 auto old=legacy.sample(0,64,32);sameFrame(old,explicitLegacy.sample(0,64,32));check(!old.hdr2&&old.upload.bspLightmaps[0].snapshot->lightmaps->faceLump()==7,"omitted profile changed legacy");
 std::optional<PreparedSessionSceneCandidate> kept;std::weak_ptr<const SourceBspLightmapSnapshotCandidate> lifetime;std::weak_ptr<const SceneHdr2ProfileCandidate> profileLife=profile;
 {
  RetainedSessionSceneCandidate owner(graph,0,{root},1,false,false,profile);auto first=owner.samplePlayback(0,64,32);check(first.hdr2&&first.hdr2->profile()==profile&&first.hdr2->plan()->faces.size()==3&&first.hdr2->plan()->opaque.size()==3,"actual retained session did not produce profile/plan");
  check(first.upload.bspLightmaps.at(0).snapshot->lightmaps->faceLump()==58&&first.upload.bspLightmaps[0].snapshot->lightmaps->lightingLump()==53&&first.upload.vertices.size()!=old.upload.vertices.size(),"actual session used wrong geometry dataset");
  const auto& plan=*first.hdr2->plan();check(plan.policy.styleScale==.5f&&plan.policy.exposure==2&&plan.faces[0].page.width==4&&plan.faces[0].page.x==1,"profile/plan policy mismatch");
  check(plan.faces[0].bytes[(1*4+1)*8]==0&&plan.faces[0].bytes[(1*4+1)*8+1]==0x38&&plan.faces[0].bytes[(1*4+1)*8+7]==0x3c,"literal retained half texel");
  auto later=std::async(std::launch::async,[&]{return owner.samplePlayback(416,64,32);}).get();sameFrame(first,later);check(later.upload.bspLightmaps[0].snapshot==first.upload.bspLightmaps[0].snapshot&&later.hdr2->profile()==profile,"worker frame lost immutable source/profile");
  owner.replaceSession(profileGraph("b"),0);auto reload=owner.sample(0,64,32);sameFrame(first,reload);check(reload.upload.bspLightmaps[0].snapshot!=first.upload.bspLightmaps[0].snapshot&&reload.hdr2->profile()==profile,"map reload mixed cache/source/profile");
  const auto oldPlan=first.hdr2;const auto oldBytes=plan.faces[0].bytes;
  owner.replaceSession(profileGraph("invalid"),0);refuses([&]{owner.sample(0,64,32);},"strict selected HDR silently fell back");check(first.hdr2==oldPlan&&first.hdr2->plan()->faces[0].bytes==oldBytes,"failed sample modified published frame");
  owner.replaceSession(profileGraph("unsupported"),0);auto allFallback=owner.sample(0,64,32);check(allFallback.hdr2&&allFallback.hdr2->plan()->faces.empty()&&!allFallback.hdr2->plan()->opaque.empty(),"all-unadmitted plan must preserve legacy draws");for(const auto&batch:allFallback.hdr2->plan()->opaque)check(batch.faceResource==SIZE_MAX&&!batch.unadmittedReason.empty(),"unadmitted batch reason missing");
  FilmTimelineStateCandidate state;auto blank=owner.sampleTimeline(10000,64,32,state);check(state==FilmTimelineStateCandidate::Gap&&blank.hdr2&&blank.hdr2->profile()==profile&&blank.hdr2->plan()->faces.empty()&&blank.hdr2->plan()->opaque.empty()&&blank.upload.vertices.empty(),"blank frame stale HDR plan");
  owner.replaceSession(graph,0);auto restored=owner.sample(0,64,32);sameFrame(first,restored);check(restored.hdr2->profile()==profile&&first.hdr2->plan()->faces[0].bytes==oldBytes,"seek/reload recovery altered old frame");
  // A failed final plan allocation/publication must leave the caller's prior
  // immutable packet intact. Each stage is injected independently.
  std::vector<std::string> stages;SceneHdr2PreparedFrameCandidate::prepare(first,profile,[&](const char*s){stages.emplace_back(s);});
  for(bool allocation:{false,true})for(size_t fail=0;fail<stages.size();++fail){size_t n=0;auto committed=first.hdr2;refuses([&]{auto next=SceneHdr2PreparedFrameCandidate::prepare(first,profile,[&](const char*){if(n++==fail){if(allocation)throw std::bad_alloc();throw std::runtime_error("owned failure");}});committed=std::move(next);},"frame plan failure not propagated");check(committed==first.hdr2&&committed->plan()->faces[0].bytes==oldBytes,"failed frame pair replaced previous packet");++refused;}
  // Confirm the session's final decoration is included before plan publication.
  owner.addSceneDecorationCandidate([](const auto&,const auto&,const auto&,const auto&,auto& packet){packet.upload.vertexShading.at(0).sourceVertex=UINT32_MAX;});refuses([&]{owner.sample(0,64,32);},"plan was prepared before decoration");
  lifetime=first.upload.bspLightmaps[0].snapshot;kept=std::move(first);
 }
 check(graphExactEqualCandidate(graph,saved),"input session graph mutated");profile.reset();check(!profileLife.expired()&&!lifetime.expired()&&kept->hdr2->plan()->faces[0].bytes.size()==128,"frame failed to retain policy/source after owner retirement");kept.reset();check(profileLife.expired()&&lifetime.expired(),"frame/source/profile release failed");
 check(readBinaryAssetCandidate(root/"maps/a.bsp")==bytes,"source fixture modified");
 std::cout<<"PASS retained_hdr2_profile=1 selected_geometry_data=58/53 omitted_profile_legacy=1 immutable_frame_pair=1 policy_identity_variants="<<different<<" failure_refusals="<<refused<<" worker_seek_reload=1 unsupported_blank=1 post_decoration_plan=1 source_lifetime=1 original_runtime=ABSENT GPU_profile=NOT_CONNECTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#endif

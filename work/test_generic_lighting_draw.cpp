#include "generic_lighting_draw_candidate.h"
#include "vcs6_shader_bytecode_candidate.h"
#include "native_shader_program_selection_candidate.h"
#include "d3d9_shader_constants_candidate.h"
#include "generic_ambient_constants_candidate.h"
#include "generic_lighting_constants_record_candidate.h"
#include "generic_lighting_metadata_candidate.h"
#include "generic_light_float_upload_candidate.h"
#include "generic_lighting_upload_bundle_candidate.h"
#include "generic_single_draw_preparation_candidate.h"
#include "generic_parallel_draw_preparation_candidate.h"
#include "retained_shader_constants_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <chrono>
int main(){try{
 static_assert(sizeof(void*)==8,"x64 required");
 std::ifstream file(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_vs20.vcs)",std::ios::binary);
 if(!file)throw std::runtime_error("cache open");
 RetainedVcs6ShaderCacheCandidate cache(std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),{}));
 std::ifstream pixelFile(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_ps20.vcs)",std::ios::binary);
 if(!pixelFile)throw std::runtime_error("pixel cache open");
 RetainedVcs6ShaderCacheCandidate pixelCache(std::vector<uint8_t>((std::istreambuf_iterator<char>(pixelFile)),{}));
 NativeShaderProgramSelectionCandidate pairSelection(cache,0,pixelCache,0);unsigned pairs=0;
 GenericAmbientStateCandidate uploadAmbient;std::array<uint32_t,24> ambientWords{};
 for(unsigned word=0;word<24;++word)ambientWords[word]=0x3e800000u+word*0x10000u;
 uploadAmbient.set(ambientWords);
 auto ambientPlan=genericAmbientConstantsCandidate(*cache.shader(0,2),uploadAmbient);
 if(ambientPlan.uploads.size()!=3)throw std::runtime_error("ambient upload cardinality");
 for(unsigned axis=0;axis<3;++axis){const auto& upload=ambientPlan.uploads[axis];
  if(upload.pixel||upload.index!=21+axis*2||upload.words.size()!=8)throw std::runtime_error("ambient upload metadata");
  for(unsigned side=0;side<2;++side){
   for(unsigned channel=0;channel<3;++channel)if(upload.words[side*4+channel]!=ambientWords[(axis*2+side)*4+channel])throw std::runtime_error("ambient upload RGB");
   if(upload.words[side*4+3]!=0x3f800000u)throw std::runtime_error("ambient upload fourth component");
  }}
 RetainedShaderConstantPlanCandidate ambientCache(256,32);unsigned ambientCallbacks=0;
 if(ambientCache.apply(ambientPlan,[&](const auto&){++ambientCallbacks;})!=3||ambientCache.apply(ambientPlan,[&](const auto&){++ambientCallbacks;})!=0||ambientCallbacks!=3)throw std::runtime_error("ambient upload reuse");
 std::cout<<"generic_ambient_metadata_uploads=3 retained_repeat_uploads=0\n";
 {GenericLightingConstantsRecordCandidate previous;previous.words.fill(0x49875312u);
 GenericLightArrayCandidate lights;std::array<uint32_t,88> source{};source[0]=2;source[1]=0x3f800000u;
 lights.set(1,2,source,[]{});std::vector<uint32_t> unused(20,0x3f800000u);
 auto litProgram=cache.shader(0,10);
 bool lightInfoFound=false;
 for(const auto& entry:d3d9ShaderConstantsCandidate(*litProgram))if(entry.name=="cLightInfo"){lightInfoFound=entry.registerSet==2&&entry.index==27&&entry.count==5;}
 if(!lightInfoFound)throw std::runtime_error("installed one-light shader metadata");
 auto bundle=genericLightingUploadBundleCandidate(*litProgram,uploadAmbient,lights,2,false,unused,previous);
 if(bundle.floats.uploads.size()!=5||bundle.metadata.size()!=2||bundle.floats.uploads[3].index!=27||bundle.floats.uploads[4].index!=32||bundle.record.words[25]!=previous.words[25])throw std::runtime_error("lighting bundle preparation");
 RetainedShaderConstantPlanCandidate floats(256,32);RetainedVertexMetadataCandidate metadata(16,16);
 unsigned floatCallbacks=0,metadataCallbacks=0;
 if(floats.apply(bundle.floats,[&](const auto&){++floatCallbacks;})!=5||metadata.apply(bundle.metadata,[&](const auto&){++metadataCallbacks;})!=2||floats.apply(bundle.floats,[&](const auto&){++floatCallbacks;})!=0||metadata.apply(bundle.metadata,[&](const auto&){++metadataCallbacks;})!=0)throw std::runtime_error("lighting bundle cache reuse");
 if(floatCallbacks!=5||metadataCallbacks!=2)throw std::runtime_error("lighting bundle callbacks");
 auto before=previous.words;
 try{genericLightingUploadBundleCandidate(*litProgram,uploadAmbient,lights,2,false,{},previous);throw std::runtime_error("lighting bundle short baseline accepted");}catch(const std::invalid_argument&){}
 try{genericLightingUploadBundleCandidate(*cache.shader(0,2),uploadAmbient,lights,2,false,unused,previous);throw std::runtime_error("lighting bundle absent light metadata accepted");}catch(const std::invalid_argument&){}
 lights.set(2,2,source,[]{});
 auto twoLightBundle=genericLightingUploadBundleCandidate(*cache.shader(0,18),uploadAmbient,lights,2,false,{},previous);
 if(twoLightBundle.floats.uploads.size()!=4||twoLightBundle.floats.uploads.back().words.size()!=40)throw std::runtime_error("installed two-light shader bundle");
 NativeShaderProgramSelectionCandidate preparedSelection(cache,0,pixelCache,0);
 const uint32_t preparedCommands[]{12,18,11,0,0};
 GenericLightingUploadBundleCandidate selectedBundle;
 auto selectedPrograms=preparedSelection.executePrepared(preparedCommands,5,[&](const auto& programs){selectedBundle=genericLightingUploadBundleCandidate(*programs.vertex,uploadAmbient,lights,2,false,{},previous);});
 if(selectedPrograms.vertex.get()!=cache.shader(0,18).get()||selectedBundle.record.words[24]!=2||preparedSelection.state().vertex!=18)throw std::runtime_error("selected lighting preparation");
 auto beforeSelection=preparedSelection.state();lights.set(1,2,source,[]{});
 const uint32_t failedCommands[]{12,10,11,0,0};
 try{preparedSelection.executePrepared(failedCommands,5,[&](const auto& programs){genericLightingUploadBundleCandidate(*programs.vertex,uploadAmbient,lights,2,false,{},previous);});throw std::runtime_error("failed lighting preparation accepted");}catch(const std::invalid_argument&){}
 if(preparedSelection.state().vertex!=beforeSelection.vertex||preparedSelection.state().pixel!=beforeSelection.pixel)throw std::runtime_error("lighting failure changed selection");
 std::cout<<"shader_lighting_preparation_commit=passed failed_preparation_preserves_selection=1\n";
 lights.set(2,2,source,[]{});std::array<uint32_t,18> zeroBaseline{};
 auto preparedDraw=genericSingleDrawPreparationCandidate(preparedSelection,uploadAmbient,lights,zeroBaseline,0,0,0,0,false,2,false,{},previous);
 if(preparedDraw.vertexDynamic!=18||preparedDraw.pixelDynamic!=0||preparedDraw.lighting.record.words[24]!=2||preparedDraw.programs.vertex.get()!=cache.shader(0,18).get())throw std::runtime_error("single draw derived preparation");
 beforeSelection=preparedSelection.state();lights.set(4,4,source,[]{});
 try{genericSingleDrawPreparationCandidate(preparedSelection,uploadAmbient,lights,zeroBaseline,0,0,0,0,false,4,false,{},previous);throw std::runtime_error("single draw invalid domain accepted");}catch(const std::invalid_argument&){}
 if(preparedSelection.state().vertex!=beforeSelection.vertex||preparedSelection.state().pixel!=beforeSelection.pixel)throw std::runtime_error("single draw invalid domain changed state");
 std::cout<<"single_draw_derived_shader_and_lighting=passed invalid_domain_preserves_selection=1\n";
 {std::vector<GenericDrawPreparationRequestCandidate> requests(32);
 for(unsigned n=0;n<requests.size();++n){auto& r=requests[n];r.ambient=uploadAmbient;r.lights.set(1+n%2,2,source,[]{});r.configuredCount=2;r.unusedWords=unused;r.previous=previous;}
 auto serial=genericParallelDrawPreparationCandidate(cache,0,pixelCache,0,requests,1);
 auto parallel=genericParallelDrawPreparationCandidate(cache,0,pixelCache,0,requests,4);
 auto coldLoad=[](const char* path){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("cold shader open");return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)),{});};
 RetainedVcs6ShaderCacheCandidate coldVertex(coldLoad(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_vs20.vcs)"));
 RetainedVcs6ShaderCacheCandidate coldPixel(coldLoad(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_ps20.vcs)"));
 auto coldParallel=genericParallelDrawPreparationCandidate(coldVertex,0,coldPixel,0,requests,4);
 for(size_t n=0;n<requests.size();++n){
  if(serial[n].vertexDynamic!=parallel[n].vertexDynamic||serial[n].pixelDynamic!=parallel[n].pixelDynamic||serial[n].programs.vertex.get()!=parallel[n].programs.vertex.get()||serial[n].programs.pixel.get()!=parallel[n].programs.pixel.get()||serial[n].lighting.record.words!=parallel[n].lighting.record.words||serial[n].lighting.floats.uploads.size()!=parallel[n].lighting.floats.uploads.size())throw std::runtime_error("parallel draw identity");
  for(size_t u=0;u<serial[n].lighting.floats.uploads.size();++u){const auto& a=serial[n].lighting.floats.uploads[u];const auto& b=parallel[n].lighting.floats.uploads[u];if(a.index!=b.index||a.pixel!=b.pixel||a.words!=b.words)throw std::runtime_error("parallel draw words");}
  if(*coldParallel[n].programs.vertex!=*serial[n].programs.vertex||*coldParallel[n].programs.pixel!=*serial[n].programs.pixel||coldParallel[n].lighting.record.words!=serial[n].lighting.record.words)throw std::runtime_error("cold parallel preparation");
  if(coldParallel[n].lighting.metadata.size()!=serial[n].lighting.metadata.size())throw std::runtime_error("cold metadata size");
  for(size_t u=0;u<serial[n].lighting.metadata.size();++u){const auto& a=serial[n].lighting.metadata[u];const auto& b=coldParallel[n].lighting.metadata[u];if(a.kind!=b.kind||a.index!=b.index||a.force!=b.force||a.words!=b.words)throw std::runtime_error("cold metadata words");}
 }
 std::vector<GenericDrawPreparationRequestCandidate> benchmarkRequests;
 benchmarkRequests.reserve(4096);for(unsigned n=0;n<4096;++n)benchmarkRequests.push_back(requests[n%requests.size()]);
 auto measure=[&](unsigned workers){std::vector<int64_t> samples;
  for(unsigned trial=0;trial<5;++trial){const auto start=std::chrono::steady_clock::now();
   auto prepared=genericParallelDrawPreparationCandidate(cache,0,pixelCache,0,benchmarkRequests,workers);
   const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
   if(prepared.size()!=4096||prepared.back().vertexDynamic!=18)throw std::runtime_error("benchmark result");samples.push_back(elapsed);
  }
  std::sort(samples.begin(),samples.end());return samples[2];};
 const auto serialMicros=measure(1),parallelMicros=measure(4);
 std::cout<<"draw_preparation_benchmark_requests=4096 trials=5 serial_median_us="<<serialMicros<<" parallel4_median_us="<<parallelMicros<<'\n';
 requests[9].lights.count=4;
 try{genericParallelDrawPreparationCandidate(cache,0,pixelCache,0,requests,4);throw std::runtime_error("parallel invalid request accepted");}catch(const std::invalid_argument&){}
 requests[9].lights.count=2;
 auto retry=genericParallelDrawPreparationCandidate(cache,0,pixelCache,0,requests,4);
 if(retry.size()!=32||retry[9].vertexDynamic!=18)throw std::runtime_error("parallel retry after failure");
 std::cout<<"parallel_cold_shader_and_metadata_exact=32 retry_after_failure=passed\n";
 std::cout<<"parallel_draw_preparation_exact=32 workers=4 failure_propagation=passed\n";}
 if(previous.words!=before)throw std::runtime_error("lighting bundle caller mutation");
 std::cout<<"lighting_upload_bundle_float_ranges=5 metadata_ranges=2 repeated_uploads=0\n";}
 {GenericLightingConstantsRecordCandidate constantRecord;constantRecord.words.fill(0x4ab19872u);
 GenericLightArrayCandidate retainedLights;std::array<uint32_t,88> lightWords{};
 for(unsigned word=0;word<88;++word)lightWords[word]=0x3f000000u+word*0x100u;
 for(unsigned light=0;light<4;++light)lightWords[light*22]=light%3+1;
 unsigned recordCases=0;
 for(int32_t count : {4,3,2,1,0,-1}){
  retainedLights.set(count,4,lightWords,[]{});
  auto previous=constantRecord.words;constantRecord.update(uploadAmbient,retainedLights);
  const auto cube=genericAmbientExpansionCandidate(uploadAmbient.packed);
  if(!std::equal(cube.begin(),cube.end(),constantRecord.words.begin())||constantRecord.words[24]!=static_cast<uint32_t>(count)||constantRecord.words[129]!=static_cast<uint32_t>(count))throw std::runtime_error("lighting record composition");
  for(unsigned word=25;word<49;++word)if(constantRecord.words[word]!=previous[word])throw std::runtime_error("lighting record unrelated fields");
  for(unsigned word=49+unsigned((std::max)(count,0))*20;word<129;++word)if(constantRecord.words[word]!=previous[word])throw std::runtime_error("lighting record unused lights");
  for(unsigned light=0;light<unsigned((std::max)(count,0));++light){
   const unsigned source=light*22,dest=49+light*20;
   if(constantRecord.words[dest]!=lightWords[source+1]||constantRecord.words[dest+3]!=(lightWords[source]==2?0x3f800000u:0)||constantRecord.words[dest+7]!=(lightWords[source]==3?0x3f800000u:0))throw std::runtime_error("lighting record active light");
  }
  ++recordCases;
 }
 auto previousRecord=constantRecord.words;retainedLights.count=5;
 try{constantRecord.update(uploadAmbient,retainedLights);throw std::runtime_error("lighting record capacity accepted");}catch(const std::invalid_argument&){}
 if(constantRecord.words!=previousRecord)throw std::runtime_error("lighting record failure mutation");
 std::cout<<"retained_lighting_constant_records="<<recordCases<<" capacity_failure_preserves_record=1\n";
 auto metadata=genericLightingMetadataCandidate(constantRecord);
 if(metadata.size()!=2||metadata[0].words!=std::vector<uint32_t>({0xffffffffu,0,1,0})||metadata[1].words!=std::vector<uint32_t>({0,0,0,0}))throw std::runtime_error("lighting metadata words");
 RetainedVertexMetadataCandidate metadataCache(16,16);unsigned callbacks=0;
 auto upload=[&](const auto&){++callbacks;};
 if(metadataCache.apply(metadata,upload)!=2||metadataCache.apply(metadata,upload)!=0)throw std::runtime_error("metadata reuse");
 metadata[1].force=true;if(metadataCache.apply(metadata,upload)!=1)throw std::runtime_error("metadata force");metadata[1].force=false;
 metadataCache.invalidate();if(metadataCache.apply(metadata,upload)!=2||callbacks!=5)throw std::runtime_error("metadata invalidation");
 auto malformed=metadata;malformed[1].index=16;
 try{metadataCache.apply(malformed,upload);throw std::runtime_error("metadata invalid range accepted");}catch(const std::invalid_argument&){}
 if(callbacks!=5)throw std::runtime_error("metadata prevalidation");
 metadata[0].words[0]=2;
 try{metadataCache.apply(metadata,[](const auto&){throw std::runtime_error("injected device failure");});throw std::invalid_argument("metadata failure missing");}catch(const std::runtime_error&){}
 if(metadataCache.apply(metadata,upload)!=1||metadataCache.apply(metadata,upload)!=0)throw std::runtime_error("metadata failure commit");
 std::cout<<"lighting_metadata_initial_uploads=2 repeat_uploads=0 force_and_failure_checks=passed\n";}
 {GenericLightingConstantsRecordCandidate record;std::vector<uint32_t> baseline(80);
 for(unsigned n=0;n<80;++n)baseline[n]=0x3f000000u+n;
 unsigned cases=0;
 for(int32_t count=0;count<=4;++count)for(int32_t configured=0;configured<=4;++configured)for(bool skip : {false,true}){
  record.words[24]=static_cast<uint32_t>(count);
  auto plan=genericLightFloatUploadCandidate(record,configured,skip,baseline);
  const size_t expected=(count>0?1:0)+(!skip&&count<configured?1:0);
  if(plan.uploads.size()!=expected)throw std::runtime_error("light float plan cardinality");
  if(count>0&&(plan.uploads[0].index!=27||plan.uploads[0].words.size()!=size_t(count)*20))throw std::runtime_error("active float light range");
  if(!skip&&count<configured){const auto& fill=plan.uploads.back();
   if(fill.index!=27+uint32_t(count)*5||fill.words.size()!=size_t(configured-count)*20||!std::equal(fill.words.begin(),fill.words.end(),baseline.begin()))throw std::runtime_error("unused float light range");}
  RetainedShaderConstantPlanCandidate lightConstantCache(256,32);unsigned callbacks=0;
  if(lightConstantCache.apply(plan,[&](const auto&){++callbacks;})!=expected||lightConstantCache.apply(plan,[&](const auto&){++callbacks;})!=0||callbacks!=expected)throw std::runtime_error("light float reuse");
  ++cases;
 }
 record.words[24]=0;
 try{genericLightFloatUploadCandidate(record,4,false,{});throw std::runtime_error("short unused light source accepted");}catch(const std::invalid_argument&){}
 record.words[24]=0xffffffffu;
 try{genericLightFloatUploadCandidate(record,4,false,baseline);throw std::runtime_error("negative light upload accepted");}catch(const std::invalid_argument&){}
 std::cout<<"light_float_upload_plans="<<cases<<" repeat_uploads=0 source_bounds_checks=passed\n";}
 std::ifstream pixelBFile(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_ps20b.vcs)",std::ios::binary);
 if(!pixelBFile)throw std::runtime_error("pixel2b cache open");
 RetainedVcs6ShaderCacheCandidate pixelBCache(std::vector<uint8_t>((std::istreambuf_iterator<char>(pixelBFile)),{}));
 NativeShaderProgramSelectionCandidate pairBSelection(cache,0,pixelBCache,0);unsigned pairsB=0,missingB=0;
 std::array<uint32_t,13> flashlightVertexFields{};flashlightVertexFields[3]=1;
 // Explicit material flag value keeps the constructor's implicit final word0.
 auto flashlightPixelFields=genericPs20bStaticInitialCandidate(0x80000000u);
 flashlightPixelFields[size_t(GenericPs20bStaticFieldCandidate::Flashlight)]=1;
 auto flashlightVertexStatic=genericVs20StaticComboCandidate(flashlightVertexFields);
 auto flashlightPixelStatic=genericPs20bStaticComboCandidate(flashlightPixelFields);
 if(flashlightVertexStatic!=384||flashlightPixelStatic!=1024)throw std::runtime_error("flashlight static fields");
 NativeShaderProgramSelectionCandidate flashlightSelection(cache,flashlightVertexStatic,pixelBCache,flashlightPixelStatic);
 unsigned flashlightPairs=0,flashlightAbsent=0;
 std::array<bool,48> seen{};
 for(uint32_t a=0;a<2;++a)for(uint32_t b=0;b<2;++b)for(uint32_t c=0;c<2;++c)
 for(uint32_t lights=0;lights<3;++lights)for(uint32_t water=0;water<2;++water){
  auto index=genericVs20DynamicComboCheckedCandidate({a,b,c,0,lights,water});
  if(index>=48||seen[index])throw std::runtime_error("checked combo collision");seen[index]=true;
 }
 if(std::find(seen.begin(),seen.end(),false)!=seen.end())throw std::runtime_error("checked combo incomplete");
 if(genericVs20DynamicComboCandidate({0,0,0,0,3,0})!=genericVs20DynamicComboCandidate({0,0,0,0,0,1}))throw std::runtime_error("raw collision fixture");
 std::array<uint32_t,18> baseline{};unsigned present=0,missing=0,cases=0,domainRejected=0,inRangeUnsafe=0;
 for(unsigned ambientOn=0;ambientOn<2;++ambientOn)for(int32_t count=0;count<=4;++count)
 for(uint32_t compression=0;compression<2;++compression)for(int32_t bones=0;bones<2;++bones)
 for(uint8_t flow=0;flow<2;++flow)for(int32_t query:{0,2}){
  GenericAmbientStateCandidate ambient;std::array<uint32_t,24> supplied{};if(ambientOn)supplied[0]=0x3f800000;ambient.set(supplied);
  GenericLightArrayCandidate lights;std::array<uint32_t,88> lightWords{};unsigned flushes=0;lights.set(count,4,lightWords,[&]{++flushes;});
  RetainedGenericLightStateCandidate retained;retained.replace({genericSingleLightRecordCandidate(ambient,lights)},baseline);auto state=retained.query();
  if(flushes!=1||state.lightCount!=count||state.firstFlag!=bool(ambientOn)||state.secondFlag)throw std::runtime_error("lighting producer composition");
  auto fields=genericLightingDynamicFieldsCandidate(state,compression,bones,flow,query);
  auto dynamic=genericVs20DynamicComboCandidate(fields);
  bool rejected=false;
  try{if(genericVs20DynamicComboCheckedCandidate(fields)!=dynamic)throw std::runtime_error("checked selector mismatch");}
  catch(const std::invalid_argument&){rejected=true;++domainRejected;if(dynamic<48)++inRangeUnsafe;}
  if(rejected!=(count>2&&flow==0))throw std::runtime_error("checked field domain");
  if(!rejected){
   // Explicit water-write input for this diagnostic, not recovered opacity policy.
   uint32_t pixel=genericPs20DynamicComboCandidate(query==2,query);
   uint32_t commands[]{12,dynamic,11,pixel,0};auto pair=pairSelection.execute(commands,5);
   if(pair.vertex.get()!=cache.shader(0,dynamic).get()||pair.pixel.get()!=pixelCache.shader(0,pixel).get())throw std::runtime_error("lighting shader pair");++pairs;
   for(bool shadows:{false,true}){
    auto pixelB=genericPs20bDynamicComboCandidate(shadows,query==2,query);
    uint32_t commandsB[]{12,dynamic,11,pixelB,0};
    auto beforeB=pairBSelection.state();
    try{auto pairB=pairBSelection.execute(commandsB,5);
     if(pairB.vertex.get()!=cache.shader(0,dynamic).get()||pairB.pixel.get()!=pixelBCache.shader(0,pixelB).get())throw std::runtime_error("lighting shader2b pair");++pairsB;
    }catch(const std::runtime_error& error){if(std::string(error.what())!="VCS6 dynamic combo absent"||!shadows)throw;
     if(pairBSelection.state().vertex!=beforeB.vertex||pairBSelection.state().pixel!=beforeB.pixel)throw std::runtime_error("absent shader2b selection mutation");++missingB;}
    auto beforeFlash=flashlightSelection.state();
    try{auto flashlightPair=flashlightSelection.execute(commandsB,5);
     if(flashlightPair.vertex.get()!=cache.shader(flashlightVertexStatic,dynamic).get()||flashlightPair.pixel.get()!=pixelBCache.shader(flashlightPixelStatic,pixelB).get())throw std::runtime_error("flashlight pair identity");++flashlightPairs;
    }catch(const std::runtime_error& error){if(std::string(error.what())!="VCS6 dynamic combo absent")throw;
     if(flashlightSelection.state().vertex!=beforeFlash.vertex||flashlightSelection.state().pixel!=beforeFlash.pixel)throw std::runtime_error("absent flashlight selection mutation");++flashlightAbsent;}
   }
  }
  // Explicit static choice; automatic material static-field derivation remains incomplete.
  std::shared_ptr<const std::vector<uint8_t>> program;
  try{program=cache.shader(0,dynamic);}
  catch(const std::runtime_error& error){
   if(std::string(error.what())!="VCS6 combo range"||dynamic<48||count<3||flow!=0||query!=2)throw;
   std::cout<<"unresolved ambient="<<ambientOn<<" lights="<<count<<" compression="<<compression<<" bones="<<bones<<" flow="<<unsigned(flow)<<" query="<<query<<" dynamic="<<dynamic<<" reason="<<error.what()<<'\n';
   ++missing;
  }
  if(program){if(program->size()<8||((*program)[3]!=0xff)||((*program)[2]!=0xfe))throw std::runtime_error("wrong shader stage");++present;}
  ++cases;
 }
 if(present!=144||missing!=16||cases!=160||domainRejected!=32||inRangeUnsafe!=16||pairs!=128||pairsB!=128||missingB!=128||flashlightPairs!=256||flashlightAbsent!=0)throw std::runtime_error("lighting cache coverage");
 std::cout<<"installed_lighting_shader_pairs="<<pairs<<'\n';
 std::cout<<"installed_lighting_shader2b_pairs="<<pairsB<<" shader2b_absent="<<missingB<<'\n';
 std::cout<<"installed_flashlight_shader_pairs="<<flashlightPairs<<" flashlight_absent="<<flashlightAbsent<<'\n';
 std::cout<<"checked_domain_rejections="<<domainRejected<<" in_range_unsafe="<<inRangeUnsafe<<'\n';
 std::cout<<"lighting_draw_state_cases="<<cases<<" installed_programs_present="<<present<<" absent_or_out_of_range="<<missing<<" decoded_blocks="<<cache.decodedBlocks()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

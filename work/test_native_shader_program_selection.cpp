#include "native_shader_program_selection_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <cstring>
#include "native_shader_command_plan_candidate.h"
#include "native_unlit_shader_draw_inputs_candidate.h"
#include "retained_shader_constants_candidate.h"
static auto load(const char* path){std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("cache open");return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),{});}
int main(){try{
 static_assert(sizeof(void*)==8,"x64 required");
 RetainedVcs6ShaderCacheCandidate vertex(load(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\unlitgeneric_vs20.vcs)"));
 RetainedVcs6ShaderCacheCandidate pixel(load(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\unlitgeneric_notexture_ps20.vcs)"));
 NativeShaderProgramSelectionCandidate selection(vertex,0,pixel,0);
 uint32_t commands[]{12,4,11,0,0};auto programs=selection.execute(commands,5);
 auto constants=nativeUnlitDiagnosticConstantsCandidate(*programs.vertex);
 std::vector<uint32_t> stream{12,4,11,0};
 for(const auto& constant:constants){stream.insert(stream.end(),{4,constant.index,static_cast<uint32_t>(constant.values.size()/4)});
  for(float value:constant.values){uint32_t bits;std::memcpy(&bits,&value,4);stream.push_back(bits);}}
 stream.push_back(0);auto plan=nativeShaderCommandPlanCandidate(stream,256,32);
 if(plan.uploads.size()!=constants.size())throw std::runtime_error("constant command cardinality");
 for(size_t i=0;i<constants.size();++i){auto& upload=plan.uploads[i];auto& expected=constants[i];
  if(upload.pixel||upload.index!=expected.index||upload.words.size()*4!=expected.values.size()*sizeof(float)||std::memcmp(upload.words.data(),expected.values.data(),upload.words.size()*4))throw std::runtime_error("installed shader constant command mismatch");}
 auto planned=selection.execute(plan.selections.data(),plan.selections.size());
 RetainedShaderConstantPlanCandidate constantCache(256,32);size_t callbacks=0;
 if(constantCache.apply(plan,[&](const auto&){++callbacks;})!=7||constantCache.apply(plan,[&](const auto&){++callbacks;})!=0||callbacks!=7)throw std::runtime_error("retained mixed plan reuse");
 auto badPlan=plan;badPlan.uploads.push_back({false,256,{1,2,3,4}});constantCache.invalidate();bool planRejected=false;
 try{constantCache.apply(badPlan,[&](const auto&){++callbacks;});}catch(const std::runtime_error&){planRejected=true;}
 if(!planRejected||callbacks!=7)throw std::runtime_error("bad final upload reached callback");
 if(constantCache.apply(plan,[&](const auto&){++callbacks;})!=7)throw std::runtime_error("plan invalidation refresh");
 if(planned.vertex.get()!=programs.vertex.get()||planned.pixel.get()!=programs.pixel.get())throw std::runtime_error("mixed buffer selection mismatch");
 unsigned malformed=0;
 for(auto bad:std::vector<std::vector<uint32_t>>{{4,0,1,0},{3,32,1,0,0,0,0,0},{4,UINT32_MAX,1,0,0,0,0,0},{4,0,UINT32_MAX,0},{7,0,0},{4,0,0},{0,4}}){bool failed=false;
  try{nativeShaderCommandPlanCandidate(bad,256,32);}catch(const std::runtime_error&){failed=true;}
  if(!failed)throw std::runtime_error("invalid constant stream accepted");++malformed;}
 if(*programs.vertex!=*vertex.shader(0,4)||*programs.pixel!=*pixel.shader(0,0)||selection.state().vertex!=4||selection.state().pixel!=0)throw std::runtime_error("command-to-cache selection");
 unsigned rejected=0;
 for(auto bad:std::vector<std::vector<uint32_t>>{{12,0,0},{11,1,0},{12,UINT32_MAX,0},{1,0,0},{12,5}}){bool failed=false;
  try{selection.execute(bad.data(),bad.size());}catch(const std::runtime_error&){failed=true;}
  if(!failed||selection.state().vertex!=4||selection.state().pixel!=0)throw std::runtime_error("failed request changed selection");++rejected;}
 uint32_t unchanged[]{0};auto retained=selection.execute(unchanged,1);
 if(retained.vertex.get()!=programs.vertex.get()||retained.pixel.get()!=programs.pixel.get())throw std::runtime_error("retained command identity");
 NativeShaderProgramSelectionCandidate swapped(pixel,0,pixel,0);bool stageFailed=false;
 try{swapped.execute(unchanged,1);}catch(const std::runtime_error&){stageFailed=true;}
 if(!stageFailed)throw std::runtime_error("wrong stage accepted");
 std::cout<<"command_to_installed_bytecode=passed retained_identity=passed failed_selection_atomicity="<<rejected<<" stage_rejection=passed constant_uploads_exact="<<constants.size()<<" malformed_constant_streams="<<malformed<<" retained_plan_repeat_uploads=0 invalid_plan_before_callback=passed invalidation_refresh=7\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

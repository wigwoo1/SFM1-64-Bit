from pathlib import Path
path=Path(__file__).resolve().parent/'probe_native_d3d9_shadow_device.cpp'
text=path.read_text()
def replace(old,new):
 global text
 assert text.count(old)==1,old
 text=text.replace(old,new)
replace('#include "native_shader_program_selection_candidate.h"','#include "native_shader_program_selection_candidate.h"\n#include "native_d3d9_shader_constants_candidate.h"')
replace('auto uploads=nativeUnlitDiagnosticConstantsCandidate(*vs);',r'''auto uploads=nativeUnlitDiagnosticConstantsCandidate(*vs);
std::vector<uint32_t> commandWords{12,4,11,0};
for(const auto& upload:uploads){commandWords.insert(commandWords.end(),{4,upload.index,static_cast<uint32_t>(upload.values.size()/4)});for(float value:upload.values){uint32_t bits;std::memcpy(&bits,&value,4);commandWords.push_back(bits);}}
commandWords.push_back(0);auto commandPlan=nativeShaderCommandPlanCandidate(commandWords,256,32);
auto plannedPrograms=selection.execute(commandPlan.selections.data(),commandPlan.selections.size());
if(plannedPrograms.vertex.get()!=vs.get()||plannedPrograms.pixel.get()!=ps.get()||commandPlan.uploads.size()!=uploads.size())throw std::runtime_error("mixed command preflight mismatch");''')
replace('for(const auto& upload:uploads)check(device->SetVertexShaderConstantF(upload.index,upload.values.data(),static_cast<UINT>(upload.values.size()/4)));',r'''nativeD3d9ShaderConstantsCandidate(device,commandPlan,256,32);
std::cout<<"shader_constant_command_upload_readback=passed\n";''')
path.write_text(text)

from pathlib import Path
root=Path(__file__).resolve().parent
path=root/'probe_native_d3d9_shadow_device.cpp'
text=path.read_text()
needle='#include "native_d3d9_shadow_commands_candidate.h"'
assert needle in text and 'native_d3d9_shader_pair_candidate.h' not in text
text=text.replace(needle,needle+'\n#include "native_d3d9_shader_pair_candidate.h"\n#include <iterator>')
needle='operation="state application";'
assert text.count(needle)==1
replacement=r'''operation="installed shader creation/binding";
auto loadCache=[](const char* path){std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("installed shader cache open");return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),{});};
RetainedVcs6ShaderCacheCandidate vertexCache(loadCache(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\unlitgeneric_vs20.vcs)"));
RetainedVcs6ShaderCacheCandidate pixelCache(loadCache(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\unlitgeneric_notexture_ps20.vcs)"));
{NativeD3d9ShaderPairCandidate pair(device,*vertexCache.shader(0,0),*pixelCache.shader(0,0));pair.bindAndVerify(device);
check(device->SetVertexShader(nullptr));check(device->SetPixelShader(nullptr));}
std::cout<<"installed_shader_creation_binding_readback=passed source_shader_draw=not_attempted\n";
operation="state application";'''
text=text.replace(needle,replacement)
path.write_text(text)

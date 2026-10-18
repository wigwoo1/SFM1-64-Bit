from pathlib import Path
p=Path('work/test_constant_storage_restore_native.cpp')
s=p.read_text()
assert 'manager controlled' in s
s=s.replace('#include "studio_constant_restore_defaults_candidate.h"', '#include "studio_constant_restore_defaults_candidate.h"\n#include "shader_binding_native_init.h"\n#include "manager_unbind_prefix_native_init.h"\n#include "shader_manager_restore_reset_native_init.h"\n#include "native_shader_binding_state_candidate.h"')
s=s.replace('static void manager(){events.push_back(6);}', '''struct Shader{};
static uint32_t shaderCounter,shaderCaptures;
static HRESULT shaderObserve(bool vertex,Shader* shader){if(shader)std::abort();events.insert(events.end(),{7,vertex?1u:2u});return E_FAIL;}
#ifdef _M_IX86
static uint32_t* __cdecl shaderGetter(){++shaderCaptures;return originalOwner+0x648/4;}
static HRESULT __stdcall shaderVertex(void*,Shader* shader){return shaderObserve(true,shader);}
static HRESULT __stdcall shaderPixel(void*,Shader* shader){return shaderObserve(false,shader);}
static std::array<unsigned char*,2> shaderCode{},shaderPrefixCode{};
static unsigned char* shaderManagerCode=nullptr;
#endif''')
src=Path('work/test_shader_binding_native.cpp').read_text()
start=src.index(' for(unsigned i=0;i<2;++i){const auto& body=shaderBindingBodies')
end=src.index('void* managerTable[12]{}',start)
preparation=src[start:end]
preparation=preparation.replace('code[i]','shaderCode[i]').replace('prefixCode[i]','shaderPrefixCode[i]')
preparation=preparation.replace(' std::array<unsigned char*,2> prefixCode{};','')
preparation=preparation.replace('&getter','&shaderGetter').replace('&forbiddenRefill','&refill').replace('&forbiddenExit','&shaderForbiddenExit')
preparation=preparation.replace('&counter','&shaderCounter')
preparation=preparation.replace('auto* managerCode=','shaderManagerCode=').replace('managerCode','shaderManagerCode')
# Return types are bool in this preparation helper; preserve failure checks.
import re
preparation=re.sub(r'return (?:[0-9]+);','return false;',preparation)
function='''static void __cdecl shaderForbiddenExit(){std::abort();}
static bool prepareShaderChain(std::ifstream& dll){
 static uint32_t initialized=1;static uint32_t* shaderCounterPointer=&shaderCounter;static uint32_t unused=0;
'''+preparation+'return true;\n}\n'
s=s.replace('static void __fastcall managerCall(void*,void*){manager();}',function)
s=s.replace('void restoreVertexDefaults(uint32_t bits){studioConstantRestoreDefaultsCandidate(bits,*this);}void unbindShaders(){manager();}', '''void restoreVertexDefaults(uint32_t bits){studioConstantRestoreDefaultsCandidate(bits,*this);}
 struct Device{HRESULT SetVertexShader(Shader* p){return shaderObserve(true,p);}HRESULT SetPixelShader(Shader* p){return shaderObserve(false,p);}};
 struct Lifecycle{
  Initialize& initialize;Device object;
  void recordBind(bool){++shaderCounter;}Device* commandState(){++shaderCaptures;return &object;}
  bool queueEnabled(Device*){return queued;}Device* device(Device* p){return p;}
  void queueShader(Device*,bool vertex,Shader* p){if(p||initialize.remaining<2)std::abort();initialize.remaining-=2;*initialize.cursor++=vertex?6:5;*initialize.cursor++=0;}
 };
 void unbindShaders(){NativeShaderBindingStateCandidateT<Shader> vertex{},pixel{};Lifecycle lifecycle{initialize};nativeShaderRestoreUnbindCandidate(vertex,pixel,lifecycle);if(!vertex.valid||!pixel.valid||vertex.shader||pixel.shader)std::abort();}''')
s=s.replace('void* managerTable[12]{};managerTable[11]=reinterpret_cast<void*>(&managerCall);void** managerObject=managerTable;void*** managerPointer=&managerObject;', 'if(!prepareShaderChain(dll))return 12;void* managerTable[12]{};managerTable[9]=shaderPrefixCode[0];managerTable[10]=shaderPrefixCode[1];managerTable[11]=shaderManagerCode;std::array<uint32_t,41> managerObject{};managerObject[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(managerTable));uint32_t* managerPointer=managerObject.data();')
s=s.replace('void* deviceTable[114]{};', 'void* deviceTable[114]{};deviceTable[92]=reinterpret_cast<void*>(&shaderVertex);deviceTable[107]=reinterpret_cast<void*>(&shaderPixel);')
s=s.replace('events.clear();allocations=frees=0;', 'events.clear();allocations=frees=0;shaderCounter=shaderCaptures=0;')
s=s.replace('native(original.data());syncStorage();', 'managerObject[0x9c/4]=managerObject[0xa0/4]=0;native(original.data());if(managerObject[0x9c/4]||managerObject[0xa0/4])return 13;syncStorage();')
s=s.replace('std::vector<uint32_t> record{allocations,frees,remaining,', 'if(shaderCounter!=2||shaderCaptures!=2)return 14;std::vector<uint32_t> record{shaderCounter,shaderCaptures,allocations,frees,remaining,')
s=s.replace('VirtualFree(defaultsCode,0,MEM_RELEASE);', 'VirtualFree(defaultsCode,0,MEM_RELEASE);for(auto* body:shaderCode)VirtualFree(body,0,MEM_RELEASE);for(auto* body:shaderPrefixCode)VirtualFree(body,0,MEM_RELEASE);VirtualFree(shaderManagerCode,0,MEM_RELEASE);')
s=s.replace('original scalars and defaults; manager controlled','original scalars, defaults and shader-unbind chain; external services controlled')
p.write_text(s)
print('integrated_original_shader_unbind_chain=1')

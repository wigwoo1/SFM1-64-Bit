from pathlib import Path
path=Path(__file__).resolve().parent/'probe_native_d3d9_shadow_device.cpp'
text=path.read_text()
def replace(before,after):
 global text
 assert text.count(before)==1,before
 text=text.replace(before,after)
replace('#include <iterator>','#include <iterator>\n#include "native_unlit_shader_draw_inputs_candidate.h"')
replace('struct Vertex {float x,y,z,rhw;DWORD color;};','struct Vertex {float x,y,z,u,v;};')
replace('IDirect3DDevice9*device=nullptr;','IDirect3DDevice9*device=nullptr;IDirect3DVertexDeclaration9*declaration=nullptr;')
replace('if(readback)readback->Release();','if(declaration)declaration->Release();if(readback)readback->Release();')
start=text.index('auto loadCache=')
end=text.index('{NativeD3d9ShaderPairCandidate',start)
cache=text[start:end]
text=text[:start]+text[end:]
replace('window=CreateWindowExW',cache+'auto vs=vertexCache.shader(0,4),ps=pixelCache.shader(0,0);auto uploads=nativeUnlitDiagnosticConstantsCandidate(*vs);std::cout<<"installed_shader_CPU_preflight=passed constant_uploads="<<uploads.size()<<"\\n";window=CreateWindowExW')
replace('''{NativeD3d9ShaderPairCandidate pair(device,*vertexCache.shader(0,4),*pixelCache.shader(0,0));pair.bindAndVerify(device);
check(device->SetVertexShader(nullptr));check(device->SetPixelShader(nullptr));}''','''auto shaderPair=std::make_unique<NativeD3d9ShaderPairCandidate>(device,*vs,*ps);shaderPair->bindAndVerify(device);
for(const auto& upload:uploads)check(device->SetVertexShaderConstantF(upload.index,upload.values.data(),static_cast<UINT>(upload.values.size()/4)));''')
replace('source_shader_draw=not_attempted','source_shader_draw=pending')
replace('check(device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1));check(device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE));check(device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));',r'''const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};check(device->CreateVertexDeclaration(elements,&declaration));check(device->SetVertexDeclaration(declaration));''')
replace('const Vertex triangle[]{{32,32,.5f,1,0xffff8040},{224,32,.5f,1,0xffff8040},{128,224,.5f,1,0xffff8040}};','const Vertex triangle[]{{-.75f,.75f,.5f,0,0},{.75f,.75f,.5f,1,0},{0,-.75f,.5f,.5f,1}};')
replace('source_shader_parity=unverified','installed_source_shader_draw=passed source_shader_parity=unverified')
replace('cleanup();return 0;','shaderPair.reset();cleanup();return 0;')
path.write_text(text)

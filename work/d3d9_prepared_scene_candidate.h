#pragma once
// Owned offscreen x64 correctness candidate. No original engine/shader dependency.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include "scene_gpu_upload_candidate.h"
#include "render_camera_candidate.h"
#include "normal_two_texture_material_candidate.h"
#include "scene_base_lightmap_candidate.h"
#include <atomic>
#include <functional>
#include <utility>

inline void d3d9SceneRequireCandidate(bool value,const char*message){if(!value)throw std::runtime_error(message);}
inline void d3d9SceneCheckCandidate(HRESULT result){if(FAILED(result))throw std::runtime_error("D3D9 scene HRESULT="+std::to_string(uint32_t(result)));}
inline std::atomic<size_t> d3d9SceneLiveResourcesCandidate{0};
enum class D3D9ShadingDiagnosticCandidate:uint32_t {Disabled,Normal,Tangent};
template<class T> class D3D9SceneComCandidate {
    T*pointer_=nullptr;
public:
    D3D9SceneComCandidate()=default;
    D3D9SceneComCandidate(const D3D9SceneComCandidate&)=delete;
    D3D9SceneComCandidate&operator=(const D3D9SceneComCandidate&)=delete;
    D3D9SceneComCandidate(D3D9SceneComCandidate&&other)noexcept:pointer_(std::exchange(other.pointer_,nullptr)){}
    D3D9SceneComCandidate&operator=(D3D9SceneComCandidate&&other)noexcept{if(this!=&other){reset();pointer_=std::exchange(other.pointer_,nullptr);}return *this;}
    ~D3D9SceneComCandidate(){reset();}
    void reset(T*next=nullptr){if(pointer_){pointer_->Release();--d3d9SceneLiveResourcesCandidate;}pointer_=next;if(next)++d3d9SceneLiveResourcesCandidate;}
    template<class Call>void create(Call call){d3d9SceneRequireCandidate(!pointer_,"D3D9 output already owned");T*value=nullptr;const auto hr=call(&value);reset(value);d3d9SceneCheckCandidate(hr);d3d9SceneRequireCandidate(pointer_!=nullptr,"D3D9 successful null output");}
    T*get()const noexcept{return pointer_;}T*operator->()const noexcept{return pointer_;}
};

class D3D9PreparedSceneCandidate {
    using Hook=std::function<void(const char*)>;
    struct Texture {D3D9SceneComCandidate<IDirect3DTexture9>gpu;std::shared_ptr<const ShaderTextureCpuPreparationCandidate>cpu;std::array<DWORD,13>states{};bool fixed=false;float level=0;};
    struct Material {Texture first,second;bool two=false,additive=false;float alpha=1,cutoff=0;MaterialUvTransformCandidate uv,uv2;};
    // The retained device is declared first and consequently released last.
    D3D9SceneComCandidate<IDirect3DDevice9>device_;
    D3D9SceneComCandidate<IDirect3DVertexShader9>vertex_;
    D3D9SceneComCandidate<IDirect3DPixelShader9>pixel_;
    D3D9SceneComCandidate<IDirect3DVertexDeclaration9>declaration_;
    D3D9SceneComCandidate<IDirect3DVertexBuffer9>vertices_;
    struct ShadingVertex {std::array<float,3>normal;std::array<float,4>tangent;};
    static_assert(sizeof(ShadingVertex)==28,"separate shading stream ABI");
    D3D9SceneComCandidate<IDirect3DVertexBuffer9>shading_;
    D3D9SceneComCandidate<IDirect3DVertexBuffer9>luxels_;
    D3D9SceneComCandidate<IDirect3DVertexShader9>lightmapVertex_;
    D3D9SceneComCandidate<IDirect3DPixelShader9>lightmapPixel_;
    D3D9SceneComCandidate<IDirect3DVertexDeclaration9>lightmapDeclaration_;
    struct Lightmap {Texture texture;std::shared_ptr<const SourceBspBaseLightmapCandidate>cpu;};
    std::vector<Lightmap>lightmaps_;
    std::array<float,4>lightmapModulation_{};
    bool baseLightmapEnabled_=false;
    D3D9ShadingDiagnosticCandidate diagnostic_=D3D9ShadingDiagnosticCandidate::Disabled;
    D3D9SceneComCandidate<IDirect3DIndexBuffer9>indices_;
    D3D9SceneComCandidate<IDirect3DSurface9>target_,depth_,readback_;
    std::vector<Material>materials_;
    std::vector<SceneBaseLightmapBatchCandidate>opaque_,translucent_;
    RenderCameraCandidate camera_{};UINT width_=0,height_=0,vertexCount_=0;
    size_t drawCalls_=0;
    D3D9PreparedSceneCandidate()=default;
    static constexpr const char*shader_=R"(
float4 rows[4]:register(c0);float4 screen:register(c4);
struct V {float3 p:POSITION;float2 uv:TEXCOORD0;};
struct P {float4 p:POSITION;float2 uv:TEXCOORD0;};
P vertexMain(V v){P o;float4 p=float4(v.p,1);o.p=float4(dot(rows[0],p),dot(rows[1],p),dot(rows[2],p),dot(rows[3],p));
o.p.xy=float2(o.p.x-o.p.w*screen.x,-o.p.y+o.p.w*screen.y);o.uv=v.uv;return o;}
sampler2D first:register(s0);sampler2D second:register(s1);
float4 params:register(c0);float4 uv0:register(c1);float4 uv1:register(c2);
float4 uv20:register(c3);float4 uv21:register(c4);float4 dx:register(c5);float4 dy:register(c6);
float4 fixedLevels:register(c7);
float4 pixelMain(P p):COLOR0{
float2 a=float2(dot(uv0.xyz,float3(p.uv,1)),dot(uv1.xyz,float3(p.uv,1)));
float4 color=fixedLevels.x>0.5?tex2Dlod(first,float4(a,0,fixedLevels.y)):params.w>0.5?tex2Dgrad(first,a,dx.xy,dy.xy):tex2D(first,a);
if(params.z>0.5){float2 b=float2(dot(uv20.xyz,float3(p.uv,1)),dot(uv21.xyz,float3(p.uv,1)));
float4 c=fixedLevels.z>0.5?tex2Dlod(second,float4(b,0,fixedLevels.w)):params.w>0.5?tex2Dgrad(second,b,dx.zw,dy.zw):tex2D(second,b);color=float4(color.rgb*c.rgb,1);}
color.a*=params.x;clip(color.a-params.y);return color;}
)";
    static constexpr const char*lightmapShader_=R"(
float4 rows[4]:register(c0);float4 screen:register(c4);
struct V {float3 p:POSITION;float2 uv:TEXCOORD0;float2 luxel:TEXCOORD1;};
struct P {float4 p:POSITION;float2 uv:TEXCOORD0;float2 luxel:TEXCOORD1;};
P vertexMain(V v){P o;float4 p=float4(v.p,1);o.p=float4(dot(rows[0],p),dot(rows[1],p),dot(rows[2],p),dot(rows[3],p));
o.p.xy=float2(o.p.x-o.p.w*screen.x,-o.p.y+o.p.w*screen.y);o.uv=v.uv;o.luxel=v.luxel;return o;}
sampler2D first:register(s0);sampler2D lightmap:register(s1);
float4 uv0:register(c1);float4 uv1:register(c2);float4 fixedLevels:register(c7);
float4 reciprocalSize:register(c8);float4 modulation:register(c9);
float3 lightAt(float2 p){return tex2Dlod(lightmap,float4((p+0.5)*reciprocalSize.xy,0,0)).rgb;}
float4 pixelMain(P p):COLOR0{
// The retained BSP preview stores (sourceU,1-sourceV). Restore Source UVs
// before the authored material transform; never flip raw lightmap luxels.
float2 sourceUv=float2(p.uv.x,1-p.uv.y);
float2 a=float2(dot(uv0.xyz,float3(sourceUv,1)),dot(uv1.xyz,float3(sourceUv,1)));
float3 albedo=fixedLevels.x>0.5?tex2Dlod(first,float4(a,0,fixedLevels.y)).rgb:tex2D(first,a).rgb;
float2 i=floor(p.luxel),f=frac(p.luxel);
float3 low=lerp(lightAt(i),lightAt(i+float2(1,0)),f.x);
float3 high=lerp(lightAt(i+float2(0,1)),lightAt(i+float2(1,1)),f.x);
return float4(albedo*lerp(low,high,f.y)*modulation.rgb,1);}
)";
    static constexpr const char*shadingShader_=R"(
float4 rows[4]:register(c0);float4 screen:register(c4);
struct V {float3 p:POSITION;float3 n:NORMAL0;float4 t:TANGENT0;};
struct P {float4 p:POSITION;float3 n:TEXCOORD0;float4 t:TEXCOORD1;};
P vertexMain(V v){P o;float4 p=float4(v.p,1);o.p=float4(dot(rows[0],p),dot(rows[1],p),dot(rows[2],p),dot(rows[3],p));
o.p.xy=float2(o.p.x-o.p.w*screen.x,-o.p.y+o.p.w*screen.y);o.n=v.n;o.t=v.t;return o;}
float4 diagnosticMode:register(c0);
float4 pixelMain(P p):COLOR0{return diagnosticMode.x<1.5?float4(p.n,1):p.t;}
)";
    static void stage(const Hook&hook,const char*name){if(hook)hook(name);}
    static void validateMaterial(const ModelRenderMaterialCandidate&m,float alpha){
        const auto shader=vmtAsciiLowerCandidate(m.source.syntax.key);
        if(shader!="unlitgeneric"&&shader!="unlittwotexture")throw std::runtime_error("D3D9 prepared unsupported shader: "+m.source.syntax.key);
        d3d9SceneRequireCandidate((shader=="unlittwotexture")==m.unlitTwoTexture,"D3D9 material shader/type mismatch");
        d3d9SceneRequireCandidate(std::isfinite(alpha)&&alpha>=0&&alpha<=1&&std::isfinite(m.alphaCutoff)&&m.alphaCutoff>=0&&m.alphaCutoff<=1,"D3D9 alpha/cutoff range");
        d3d9SceneRequireCandidate(!m.vertexLighting&&!m.vertexTransition&&!m.selfIllum.enabled&&!m.irisImage.width&&!m.source.baseTextureMissing,"D3D9 unsupported material feature");
        if(shader=="unlittwotexture"){validateNormalTwoTextureMaterialCandidate(m,alpha);}
        else d3d9SceneRequireCandidate(!m.unlitTwoTexture&&!m.additive,"D3D9 inconsistent material shader");
        // Unknown authored shader switches must not silently become an unlit pass.
        for(const auto&node:m.source.syntax.children){
            const auto key=vmtAsciiLowerCandidate(node.key);
            d3d9SceneRequireCandidate(!node.block,"D3D9 material parameter block unsupported");
            if(key=="$basetexture"||key=="$basetexturetransform"||key=="$texture2"||key=="$texture2transform"||key=="$alpha"||key=="$alphatest"||key=="$alphatestreference"||key=="$translucent"||key=="$additive"||key=="$gammacolorread"||key=="$linearwrite")continue;
            throw std::runtime_error("D3D9 prepared unsupported parameter: "+key);
        }
        for(const auto*uv:{&m.baseTextureTransform,&m.secondTextureTransform})for(float x:*uv)d3d9SceneRequireCandidate(std::isfinite(x),"D3D9 UV transform finite");
        auto texture=[](const ModelPreparedTextureCandidate&t){d3d9SceneRequireCandidate(bool(t.cpu),"D3D9 requires retained prepared texture");const auto&image=*t.cpu->image();const auto&p=image.plan();d3d9SceneRequireCandidate((image.format()==0||image.format()==13||image.format()==15)&&p[9]&&t.sourceMip<p[9]&&t.frame<p[10],"D3D9 prepared format/frame/mip");};
        texture(m.preparedBase);if(m.unlitTwoTexture)texture(m.preparedSecond);
    }
    Texture texture(const ModelPreparedTextureCandidate&input,bool srgb,const D3DCAPS9&caps,const Hook&hook){
        Texture t;t.cpu=input.cpu;const auto&image=*input.cpu->image();const auto&p=image.plan();const auto format=image.format();
        const auto flags=p[0];const bool point=(flags&1)!=0,noMip=(flags&0x100)!=0;
        const bool aniso=!point&&!noMip&&(flags&0x10)&&caps.MaxAnisotropy>=2&&(caps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC);
        const DWORD factor=aniso?std::min<DWORD>(caps.MaxAnisotropy,std::max<DWORD>(2,std::min<DWORD>(8,caps.MaxAnisotropy/4))):1;
        const DWORD u=(flags&0x20000000u)?D3DTADDRESS_BORDER:(flags&4)?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP;
        const DWORD v=(flags&0x20000000u)?D3DTADDRESS_BORDER:(flags&8)?D3DTADDRESS_CLAMP:D3DTADDRESS_WRAP;
        t.fixed=point||noMip||!(input.fullMipSampling||aniso);t.level=float(input.sourceMip);
        // NONE ignores MAXMIPLEVEL on the measured driver. An explicit fixed LOD
        // preserves the selected-view contract without making illegal 1x1 BC roots.
        t.states={u,v,u,0,aniso&&(caps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFANISOTROPIC)?DWORD(D3DTEXF_ANISOTROPIC):point?DWORD(D3DTEXF_POINT):DWORD(D3DTEXF_LINEAR),aniso?DWORD(D3DTEXF_ANISOTROPIC):point?DWORD(D3DTEXF_POINT):DWORD(D3DTEXF_LINEAR),t.fixed?DWORD(D3DTEXF_POINT):DWORD(D3DTEXF_LINEAR),0,input.sourceMip,factor,DWORD(srgb),0,0};
        t.gpu.create([&](auto**out){return device_->CreateTexture(p[6],p[7],p[9],0,format==13?D3DFMT_DXT1:format==15?D3DFMT_DXT5:D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,out,nullptr);});stage(hook,"texture-created");
        for(uint32_t level=0;level<p[9];++level){const auto lease=image.lease(input.frame,level);d3d9SceneRequireCandidate(bool(lease),"D3D9 mip lease");const UINT rows=format==0?lease->height():(lease->height()+3)/4;const UINT stride=format==0?lease->width()*4:((lease->width()+3)/4)*(format==13?8u:16u);
            d3d9SceneRequireCandidate(uint64_t(rows)*stride==lease->size(),"D3D9 complete prepared mip bytes");D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(t.gpu->LockRect(level,&lock,nullptr,0));
            try{d3d9SceneRequireCandidate(lock.Pitch>=int(stride),"D3D9 mip row pitch");for(UINT y=0;y<rows;++y){auto*dst=static_cast<uint8_t*>(lock.pBits)+size_t(y)*size_t(lock.Pitch);std::memcpy(dst,lease->data()+size_t(y)*stride,stride);if(format==0)for(UINT x=0;x<lease->width();++x)std::swap(dst[4*x],dst[4*x+2]);}stage(hook,"texture-locked-upload");}catch(...){t.gpu->UnlockRect(level);throw;}
            d3d9SceneCheckCandidate(t.gpu->UnlockRect(level));stage(hook,"texture-uploaded");
        }return t;
    }
public:
    D3D9PreparedSceneCandidate(const D3D9PreparedSceneCandidate&)=delete;
    using Options=std::array<float,8>; // explicit flag, dx.xy,dy.xy for base; second shares gradients in this proof
    template<class Prepared>static void validate(const Prepared&scene,const SceneBaseLightmapOptionsCandidate*lightmap=nullptr){
        static_assert(sizeof(void*)==8,"owned prepared renderer is native x64");
        const auto&u=scene.upload;
        validateSceneShadingCandidate(u);
        d3d9SceneRequireCandidate(scene.outputWidth&&scene.outputHeight&&scene.outputWidth<=4096&&scene.outputHeight<=4096,"D3D9 target dimensions");
        d3d9SceneRequireCandidate(!u.vertices.empty()&&!u.indices.empty()&&u.vertices.size()<=UINT_MAX/sizeof(SceneGpuVertexCandidate)&&u.indices.size()<=UINT_MAX/4,"D3D9 nonempty bounded geometry");
        for(float x:scene.camera.rows)d3d9SceneRequireCandidate(std::isfinite(x),"D3D9 finite camera rows");
        for(const auto&v:u.vertices)d3d9SceneRequireCandidate(sceneGpuVertexFiniteCandidate(v),"D3D9 finite vertex");
        for(auto index:u.indices)d3d9SceneRequireCandidate(index<u.vertices.size(),"D3D9 vertex index");
        d3d9SceneRequireCandidate(scene.materialAlpha.size()==u.materials.size(),"D3D9 material alpha coverage");
        if(lightmap)validateSceneBaseLightmapOptionsCandidate(*lightmap);
        for(size_t n=0;n<u.materials.size();++n){const auto&a=u.materials[n];const auto&m=scene.materialInputs.at(a.model).materials.at(a.material);if(lightmap&&sceneBaseLightmapMaterialCandidate(m))validateSceneBaseLightmapMaterialCandidate(m,scene.materialAlpha[n]);else validateMaterial(m,scene.materialAlpha[n]);}
        for(const auto*list:{&u.opaque,&u.translucent})for(const auto&draw:*list){
            d3d9SceneRequireCandidate(draw.indexCount&&draw.indexCount%3==0&&draw.firstIndex<=u.indices.size()&&draw.indexCount<=u.indices.size()-draw.firstIndex&&draw.materialSlot<u.materials.size(),"D3D9 indexed draw span");
            const auto&a=u.materials.at(draw.materialSlot);const auto&r=scene.materialInputs.at(draw.model).ranges.at(draw.range);
            d3d9SceneRequireCandidate(a.model==draw.model&&a.material==draw.materialIndex&&r.materialIndex==draw.materialIndex&&r.indexCount==draw.indexCount,"D3D9 material/draw identity");
        }
    }
    template<class Prepared>static std::unique_ptr<D3D9PreparedSceneCandidate>prepare(IDirect3DDevice9*device,const Prepared&scene,const Hook&hook={},D3D9ShadingDiagnosticCandidate diagnostic=D3D9ShadingDiagnosticCandidate::Disabled,std::optional<SceneBaseLightmapOptionsCandidate>lightmap=std::nullopt){
        validate(scene,lightmap?&*lightmap:nullptr);d3d9SceneRequireCandidate(uint32_t(diagnostic)<=2,"D3D9 diagnostic mode invalid");
        d3d9SceneRequireCandidate(!lightmap||diagnostic==D3D9ShadingDiagnosticCandidate::Disabled,"D3D9 lightmap and shading diagnostic cannot combine");
        std::optional<SceneBaseLightmapPlanCandidate>lightmapPlan;
        if(lightmap){lightmapPlan=prepareSceneBaseLightmapPlanCandidate(scene,*lightmap);stage(hook,"lightmap-cpu-plan");}
        const bool diagnosticEnabled=diagnostic!=D3D9ShadingDiagnosticCandidate::Disabled;
        if(diagnosticEnabled){
            validateSceneShadingCandidate(scene.upload,true);
            d3d9SceneRequireCandidate(!scene.upload.opaque.empty()&&scene.upload.translucent.empty(),"D3D9 raw diagnostic requires opaque draws");
            for(const auto&draw:scene.upload.opaque){requireSceneDrawShadingCandidate(scene.upload,draw,diagnostic==D3D9ShadingDiagnosticCandidate::Tangent);const auto&a=scene.upload.materials.at(draw.materialSlot);const auto&m=scene.materialInputs.at(a.model).materials.at(a.material);d3d9SceneRequireCandidate(!m.unlitTwoTexture&&!m.additive&&scene.materialAlpha.at(draw.materialSlot)==1&&m.alphaCutoff==0,"D3D9 raw diagnostic material controls unsupported");}
        }
        d3d9SceneRequireCandidate(device!=nullptr,"D3D9 device required");auto next=std::unique_ptr<D3D9PreparedSceneCandidate>(new D3D9PreparedSceneCandidate);next->diagnostic_=diagnostic;
        device->AddRef();next->device_.reset(device);next->width_=scene.outputWidth;next->height_=scene.outputHeight;next->vertexCount_=UINT(scene.upload.vertices.size());next->camera_=scene.camera;
        if(lightmap){next->baseLightmapEnabled_=true;std::copy(lightmap->modulation.begin(),lightmap->modulation.end(),next->lightmapModulation_.begin());next->opaque_=lightmapPlan->opaque;next->translucent_=lightmapPlan->translucent;}
        else{for(const auto&d:scene.upload.opaque)next->opaque_.push_back({d,SIZE_MAX});for(const auto&d:scene.upload.translucent)next->translucent_.push_back({d,SIZE_MAX});}
        D3DCAPS9 caps{};d3d9SceneCheckCandidate(device->GetDeviceCaps(&caps));d3d9SceneRequireCandidate(caps.MaxVertexIndex>=scene.upload.vertices.size()-1,"D3D9 vertex capability");
        if(diagnosticEnabled)d3d9SceneRequireCandidate(caps.MaxStreams>=2&&caps.MaxStreamStride>=sizeof(ShadingVertex),"D3D9 diagnostic stream capability");
        for(const auto*list:{&scene.upload.opaque,&scene.upload.translucent})for(const auto&draw:*list)d3d9SceneRequireCandidate(draw.indexCount/3<=caps.MaxPrimitiveCount,"D3D9 primitive capability");
        D3D9SceneComCandidate<ID3DBlob>vs,ps;
        const auto*shader=diagnosticEnabled?shadingShader_:shader_;
        auto compile=[&](const char*entry,const char*profile,auto&blob){D3D9SceneComCandidate<ID3DBlob>errors;blob.create([&](auto**out){ID3DBlob*error=nullptr;auto result=D3DCompile(shader,std::strlen(shader),"owned-prepared-scene",nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,0,out,&error);errors.reset(error);return result;});d3d9SceneRequireCandidate(!errors.get(),"D3D9 authored shader diagnostics");stage(hook,entry);};
        compile("vertexMain","vs_3_0",vs);compile("pixelMain","ps_3_0",ps);
        next->vertex_.create([&](auto**out){return device->CreateVertexShader(static_cast<const DWORD*>(vs->GetBufferPointer()),out);});stage(hook,"vertex-shader");
        next->pixel_.create([&](auto**out){return device->CreatePixelShader(static_cast<const DWORD*>(ps->GetBufferPointer()),out);});stage(hook,"pixel-shader");
        const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,24,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        const D3DVERTEXELEMENT9 shadingElements[]{{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{1,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0},{1,12,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TANGENT,0},D3DDECL_END()};
        next->declaration_.create([&](auto**out){return device->CreateVertexDeclaration(diagnosticEnabled?shadingElements:elements,out);});stage(hook,"declaration");
        next->vertices_.create([&](auto**out){return device->CreateVertexBuffer(UINT(scene.upload.vertices.size()*sizeof(SceneGpuVertexCandidate)),D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,out,nullptr);});stage(hook,"vertex-buffer");
        next->indices_.create([&](auto**out){return device->CreateIndexBuffer(UINT(scene.upload.indices.size()*4),D3DUSAGE_WRITEONLY,D3DFMT_INDEX32,D3DPOOL_MANAGED,out,nullptr);});stage(hook,"index-buffer");
        auto fill=[&](auto&buffer,const auto&values){void*dst=nullptr;const auto bytes=UINT(values.size()*sizeof(values[0]));d3d9SceneCheckCandidate(buffer->Lock(0,bytes,&dst,0));std::memcpy(dst,values.data(),bytes);d3d9SceneCheckCandidate(buffer->Unlock());};
        fill(next->vertices_,scene.upload.vertices);fill(next->indices_,scene.upload.indices);stage(hook,"geometry-uploaded");
        if(lightmapPlan&&!lightmapPlan->planes.empty()){
            d3d9SceneRequireCandidate(caps.MaxStreams>=2&&caps.MaxStreamStride>=8,"D3D9 lightmap stream capability");
            shader=lightmapShader_;vs.reset();ps.reset();compile("vertexMain","vs_3_0",vs);compile("pixelMain","ps_3_0",ps);
            next->lightmapVertex_.create([&](auto**out){return device->CreateVertexShader(static_cast<const DWORD*>(vs->GetBufferPointer()),out);});stage(hook,"lightmap-vertex-shader");
            next->lightmapPixel_.create([&](auto**out){return device->CreatePixelShader(static_cast<const DWORD*>(ps->GetBufferPointer()),out);});stage(hook,"lightmap-pixel-shader");
            const D3DVERTEXELEMENT9 lightElements[]{{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,24,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},{1,0,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,1},D3DDECL_END()};
            next->lightmapDeclaration_.create([&](auto**out){return device->CreateVertexDeclaration(lightElements,out);});stage(hook,"lightmap-declaration");
            next->luxels_.create([&](auto**out){return device->CreateVertexBuffer(UINT(lightmapPlan->luxels.size()*8),D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,out,nullptr);});stage(hook,"lightmap-luxel-buffer");
            void*luxels=nullptr;d3d9SceneCheckCandidate(next->luxels_->Lock(0,UINT(lightmapPlan->luxels.size()*8),&luxels,0));try{std::memcpy(luxels,lightmapPlan->luxels.data(),lightmapPlan->luxels.size()*8);stage(hook,"lightmap-luxels-locked");}catch(...){next->luxels_->Unlock();throw;}d3d9SceneCheckCandidate(next->luxels_->Unlock());stage(hook,"lightmap-luxels-uploaded");
            for(const auto&plane:lightmapPlan->planes){Lightmap lm;lm.cpu=plane;lm.texture.states={D3DTADDRESS_CLAMP,D3DTADDRESS_CLAMP,D3DTADDRESS_CLAMP,0,D3DTEXF_POINT,D3DTEXF_POINT,D3DTEXF_NONE,0,0,1,FALSE,0,0};
                lm.texture.gpu.create([&](auto**out){return device->CreateTexture(plane->width(),plane->height(),1,0,D3DFMT_A32B32G32R32F,D3DPOOL_MANAGED,out,nullptr);});stage(hook,"lightmap-texture");
                D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(lm.texture.gpu->LockRect(0,&lock,nullptr,0));try{d3d9SceneRequireCandidate(lock.Pitch>=int(plane->width()*16),"D3D9 lightmap row pitch");for(UINT y=0;y<plane->height();++y)std::memcpy(static_cast<uint8_t*>(lock.pBits)+size_t(y)*size_t(lock.Pitch),plane->pixels().data()+size_t(y)*plane->width(),size_t(plane->width())*16);stage(hook,"lightmap-texture-locked");}catch(...){lm.texture.gpu->UnlockRect(0);throw;}d3d9SceneCheckCandidate(lm.texture.gpu->UnlockRect(0));stage(hook,"lightmap-texture-uploaded");next->lightmaps_.push_back(std::move(lm));
            }
        }
        if(diagnosticEnabled){
            std::vector<ShadingVertex>values;values.reserve(scene.upload.vertexShading.size());for(const auto&v:scene.upload.vertexShading)values.push_back({v.hasNormal?v.normal:std::array<float,3>{},v.hasTangent?v.tangent:std::array<float,4>{}});
            next->shading_.create([&](auto**out){return device->CreateVertexBuffer(UINT(values.size()*sizeof(ShadingVertex)),D3DUSAGE_WRITEONLY,0,D3DPOOL_MANAGED,out,nullptr);});stage(hook,"shading-buffer");
            void*dst=nullptr;d3d9SceneCheckCandidate(next->shading_->Lock(0,UINT(values.size()*sizeof(ShadingVertex)),&dst,0));try{std::memcpy(dst,values.data(),values.size()*sizeof(ShadingVertex));stage(hook,"shading-locked-upload");}catch(...){next->shading_->Unlock();throw;}d3d9SceneCheckCandidate(next->shading_->Unlock());stage(hook,"shading-uploaded");
        }
        for(size_t n=0;n<scene.upload.materials.size();++n){const auto&a=scene.upload.materials[n];const auto&m=scene.materialInputs.at(a.model).materials.at(a.material);Material out;out.two=m.unlitTwoTexture;out.additive=m.additive;out.alpha=scene.materialAlpha[n];out.cutoff=m.alphaCutoff;out.uv=m.baseTextureTransform;out.uv2=m.secondTextureTransform;
            out.first=next->texture(m.preparedBase,m.colorSpace.baseSrgb,caps,hook);if(out.two)out.second=next->texture(m.preparedSecond,m.colorSpace.baseSrgb,caps,hook);next->materials_.push_back(std::move(out));}
        next->target_.create([&](auto**out){return device->CreateRenderTarget(next->width_,next->height_,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,out,nullptr);});stage(hook,"render-target");
        next->depth_.create([&](auto**out){return device->CreateDepthStencilSurface(next->width_,next->height_,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,out,nullptr);});stage(hook,"depth-target");
        next->readback_.create([&](auto**out){return device->CreateOffscreenPlainSurface(next->width_,next->height_,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,out,nullptr);});stage(hook,"readback");return next;
    }
    size_t drawCalls()const noexcept{return drawCalls_;}
    size_t lightmapCount()const noexcept{return lightmaps_.size();}
    const std::array<DWORD,13>&sampler(size_t material,size_t channel=0)const{return channel?materials_.at(material).second.states:materials_.at(material).first.states;}
    std::vector<float>render(const Options&options={},const Hook&hook={}){
        for(float x:options)d3d9SceneRequireCandidate(std::isfinite(x),"D3D9 render options finite");
        const bool diagnosticEnabled=diagnostic_!=D3D9ShadingDiagnosticCandidate::Disabled;
        if(diagnosticEnabled)for(float x:options)d3d9SceneRequireCandidate(x==0,"D3D9 raw diagnostic sampling options unsupported");
        if(baseLightmapEnabled_)for(float x:options)d3d9SceneRequireCandidate(x==0,"D3D9 normalized lightmap sampling overrides unsupported");
        auto*d=device_.get();D3D9SceneComCandidate<IDirect3DSurface9>oldTarget,oldDepth;
        oldTarget.create([&](auto**out){return d->GetRenderTarget(0,out);});IDirect3DSurface9*old=nullptr;auto hr=d->GetDepthStencilSurface(&old);if(hr!=D3DERR_NOTFOUND)d3d9SceneCheckCandidate(hr);oldDepth.reset(old);
        bool begun=false,locked=false;
        auto cleanup=[&]{if(locked)readback_->UnlockRect();if(begun)d->EndScene();for(DWORD n=0;n<2;++n)d->SetTexture(n,nullptr);d->SetStreamSource(0,nullptr,0,0);d->SetStreamSource(1,nullptr,0,0);d->SetIndices(nullptr);d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetVertexDeclaration(nullptr);d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,oldTarget.get());d->SetDepthStencilSurface(oldDepth.get());};
        try{
            d3d9SceneCheckCandidate(d->SetDepthStencilSurface(nullptr));d3d9SceneCheckCandidate(d->SetRenderTarget(0,target_.get()));d3d9SceneCheckCandidate(d->SetDepthStencilSurface(depth_.get()));D3DVIEWPORT9 viewport{0,0,width_,height_,0,1};d3d9SceneCheckCandidate(d->SetViewport(&viewport));
            const std::pair<D3DRENDERSTATETYPE,DWORD>states[]{{D3DRS_ZENABLE,TRUE},{D3DRS_ZFUNC,D3DCMP_LESS},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_FOGENABLE,FALSE},{D3DRS_LIGHTING,FALSE},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_DITHERENABLE,FALSE},{D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_COLORWRITEENABLE,15},{D3DRS_SEPARATEALPHABLENDENABLE,TRUE},{D3DRS_BLENDOP,D3DBLENDOP_ADD},{D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD}};
            for(const auto&s:states)d3d9SceneCheckCandidate(d->SetRenderState(s.first,s.second));
            d3d9SceneCheckCandidate(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0));
            d3d9SceneCheckCandidate(d->SetVertexDeclaration(declaration_.get()));d3d9SceneCheckCandidate(d->SetVertexShader(vertex_.get()));d3d9SceneCheckCandidate(d->SetPixelShader(pixel_.get()));d3d9SceneCheckCandidate(d->SetStreamSource(0,vertices_.get(),0,sizeof(SceneGpuVertexCandidate)));d3d9SceneCheckCandidate(d->SetIndices(indices_.get()));
            if(diagnosticEnabled){d3d9SceneCheckCandidate(d->SetStreamSource(1,shading_.get(),0,sizeof(ShadingVertex)));stage(hook,"diagnostic-stream-bound");}
            d3d9SceneCheckCandidate(d->SetVertexShaderConstantF(0,camera_.rows.data(),4));const float screen[4]{1.f/width_,1.f/height_,0,0};d3d9SceneCheckCandidate(d->SetVertexShaderConstantF(4,screen,1));d3d9SceneCheckCandidate(d->BeginScene());begun=true;drawCalls_=0;
            if(diagnosticEnabled)stage(hook,"diagnostic-scene-begun");
            if(baseLightmapEnabled_)stage(hook,"lightmap-scene-begun");
            for(const auto*list:{&opaque_,&translucent_})for(const auto&batch:*list){const auto&draw=batch.draw;const bool lit=batch.plane!=SIZE_MAX;const auto&m=materials_.at(draw.materialSlot);const bool blend=list==&translucent_||m.additive;
                if(baseLightmapEnabled_){d3d9SceneCheckCandidate(d->SetVertexDeclaration(lit?lightmapDeclaration_.get():declaration_.get()));d3d9SceneCheckCandidate(d->SetVertexShader(lit?lightmapVertex_.get():vertex_.get()));d3d9SceneCheckCandidate(d->SetPixelShader(lit?lightmapPixel_.get():pixel_.get()));d3d9SceneCheckCandidate(d->SetStreamSource(1,lit?luxels_.get():nullptr,0,lit?8:0));stage(hook,"lightmap-draw-state");}
                d3d9SceneCheckCandidate(d->SetRenderState(D3DRS_ZWRITEENABLE,!blend));d3d9SceneCheckCandidate(d->SetRenderState(D3DRS_ALPHABLENDENABLE,diagnosticEnabled?FALSE:blend));
                for(auto s:{D3DRS_SRCBLEND,D3DRS_SRCBLENDALPHA})d3d9SceneCheckCandidate(d->SetRenderState(s,s==D3DRS_SRCBLEND&&!m.additive?D3DBLEND_SRCALPHA:D3DBLEND_ONE));
                for(auto s:{D3DRS_DESTBLEND,D3DRS_DESTBLENDALPHA})d3d9SceneCheckCandidate(d->SetRenderState(s,m.additive?D3DBLEND_ONE:D3DBLEND_INVSRCALPHA));
                for(DWORD n=0;n<2;++n){const auto&t=n?(lit?lightmaps_.at(batch.plane).texture:m.second):m.first;d3d9SceneCheckCandidate(d->SetTexture(n,t.gpu.get()));if(t.gpu.get())for(DWORD s=1;s<=13;++s){d3d9SceneCheckCandidate(d->SetSamplerState(n,D3DSAMPLERSTATETYPE(s),t.states[s-1]));DWORD observed=0;d3d9SceneCheckCandidate(d->GetSamplerState(n,D3DSAMPLERSTATETYPE(s),&observed));d3d9SceneRequireCandidate(observed==t.states[s-1],"D3D9 actual sampler state differs");}}
                if(lit){const auto&lm=lightmaps_.at(batch.plane);const float dimensions[]{1.f/lm.cpu->width(),1.f/lm.cpu->height(),0,0};d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(8,dimensions,1));d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(9,lightmapModulation_.data(),1));stage(hook,"lightmap-texture-bound");}
                const float constants[7][4]{{m.alpha,m.cutoff,float(m.two),options[0]}, {m.uv[0],m.uv[1],m.uv[2],0},{m.uv[4],m.uv[5],m.uv[6],0},{m.uv2[0],m.uv2[1],m.uv2[2],0},{m.uv2[4],m.uv2[5],m.uv2[6],0},{options[1],options[2],options[1],options[2]},{options[3],options[4],options[3],options[4]}};
                d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(0,&constants[0][0],7));if(diagnosticEnabled){const float mode[4]{float(uint32_t(diagnostic_)),0,0,0};d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(0,mode,1));}const float fixed[4]{float(m.first.fixed),m.first.level,float(m.second.fixed),m.second.level};d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(7,fixed,1));d3d9SceneCheckCandidate(d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,vertexCount_,draw.firstIndex,draw.indexCount/3));++drawCalls_;if(diagnosticEnabled)stage(hook,"diagnostic-draw-completed");if(baseLightmapEnabled_)stage(hook,"lightmap-draw-completed");
            }
            d3d9SceneCheckCandidate(d->EndScene());begun=false;d3d9SceneCheckCandidate(d->GetRenderTargetData(target_.get(),readback_.get()));std::vector<float>pixels(size_t(width_)*height_*4);D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(readback_->LockRect(&lock,nullptr,D3DLOCK_READONLY));locked=true;if(baseLightmapEnabled_)stage(hook,"lightmap-readback-locked");for(UINT y=0;y<height_;++y)std::memcpy(pixels.data()+size_t(y)*width_*4,static_cast<const uint8_t*>(lock.pBits)+size_t(y)*size_t(lock.Pitch),size_t(width_)*16);d3d9SceneCheckCandidate(readback_->UnlockRect());locked=false;cleanup();return pixels;
        }catch(...){cleanup();throw;}
    }
};

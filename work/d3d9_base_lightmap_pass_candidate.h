#pragma once
// Offscreen, isolated base-diffuse operator. Not integrated renderer admission.
#include "d3d9_prepared_scene_candidate.h"
#include "source_bsp_base_lightmap_candidate.h"

struct D3D9BaseLightmapVertexCandidate {std::array<float,4> clip;std::array<float,2> luxel;};
class D3D9BaseLightmapPassCandidate {
    using Hook=std::function<void(const char*)>;
    D3D9SceneComCandidate<IDirect3DDevice9> device_;
    D3D9SceneComCandidate<IDirect3DVertexShader9> vertex_;
    D3D9SceneComCandidate<IDirect3DPixelShader9> pixel_;
    D3D9SceneComCandidate<IDirect3DVertexDeclaration9> declaration_;
    D3D9SceneComCandidate<IDirect3DTexture9> light_,base_;
    D3D9SceneComCandidate<IDirect3DSurface9> target_,readback_;
    std::shared_ptr<const SourceBspBaseLightmapCandidate> plane_;
    std::array<float,4> modulation_{};
    D3D9BaseLightmapPassCandidate()=default;
    static void stage(const Hook& h,const char* s){if(h)h(s);}
public:
    static std::unique_ptr<D3D9BaseLightmapPassCandidate> prepare(IDirect3DDevice9* d,
        std::shared_ptr<const SourceBspBaseLightmapCandidate> plane,
        const std::array<uint8_t,4>& albedo,const std::array<float,3>& modulation,const Hook& hook={}){
        d3d9SceneRequireCandidate(d&&plane,"lightmap operator device/plane required");
        for(float x:modulation)d3d9SceneRequireCandidate(std::isfinite(x)&&x>=0&&x<=16,"bounded explicit lightmap modulation");
        d3d9SceneRequireCandidate(albedo[3]==255,"lightmap operator requires opaque albedo");
        for(const auto& p:plane->pixels())for(size_t k=0;k<3;++k)
            d3d9SceneRequireCandidate(std::isfinite(p[k])&&p[k]>=0&&double(p[k])*std::max(1.,double(modulation[k]))<=double(std::numeric_limits<float>::max())/4.,"lightmap operator arithmetic headroom");
        auto next=std::unique_ptr<D3D9BaseLightmapPassCandidate>(new D3D9BaseLightmapPassCandidate);
        d->AddRef();next->device_.reset(d);next->plane_=std::move(plane);
        std::copy(modulation.begin(),modulation.end(),next->modulation_.begin());stage(hook,"device-retained");
        const char* shader=R"(
struct V {float4 position:POSITION;float2 luxel:TEXCOORD0;};
V vertexMain(V v){return v;}
sampler2D lightmap:register(s0);sampler2D albedo:register(s1);
float4 reciprocalSize:register(c0);float4 modulation:register(c1);
float3 lightAt(float2 p){return tex2Dlod(lightmap,float4((p+0.5)*reciprocalSize.xy,0,0)).rgb;}
float4 pixelMain(V v):COLOR0{
float2 i=floor(v.luxel),f=frac(v.luxel);
float3 a=lerp(lightAt(i),lightAt(i+float2(1,0)),f.x);
float3 b=lerp(lightAt(i+float2(0,1)),lightAt(i+float2(1,1)),f.x);
return float4(lerp(a,b,f.y)*tex2Dlod(albedo,float4(0.5,0.5,0,0)).rgb*modulation.rgb,1);
})";
        D3D9SceneComCandidate<ID3DBlob> vs,ps;
        auto compile=[&](const char* entry,const char* profile,auto& blob){blob.create([&](auto** out){return D3DCompile(shader,std::strlen(shader),"owned-base-lightmap",nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,0,out,nullptr);});};
        compile("vertexMain","vs_3_0",vs);stage(hook,"vertex-compiled");compile("pixelMain","ps_3_0",ps);stage(hook,"pixel-compiled");
        next->vertex_.create([&](auto** p){return d->CreateVertexShader(static_cast<const DWORD*>(vs->GetBufferPointer()),p);});stage(hook,"vertex-created");
        next->pixel_.create([&](auto** p){return d->CreatePixelShader(static_cast<const DWORD*>(ps->GetBufferPointer()),p);});stage(hook,"pixel-created");
        const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,16,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        next->declaration_.create([&](auto** p){return d->CreateVertexDeclaration(elements,p);});stage(hook,"declaration-created");
        auto upload=[&](auto& texture,UINT width,UINT height,D3DFORMAT format,const void* data,UINT stride){
            texture.create([&](auto** p){return d->CreateTexture(width,height,1,0,format,D3DPOOL_MANAGED,p,nullptr);});stage(hook,"texture-created");
            D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(texture->LockRect(0,&lock,nullptr,0));
            try{d3d9SceneRequireCandidate(lock.Pitch>=int(stride),"lightmap texture row pitch");
                for(UINT y=0;y<height;++y)std::memcpy(static_cast<uint8_t*>(lock.pBits)+size_t(y)*size_t(lock.Pitch),static_cast<const uint8_t*>(data)+size_t(y)*stride,stride);
                stage(hook,"texture-locked-upload");
            }catch(...){texture->UnlockRect(0);throw;}
            d3d9SceneCheckCandidate(texture->UnlockRect(0));stage(hook,"texture-uploaded");
        };
        upload(next->light_,next->plane_->width(),next->plane_->height(),D3DFMT_A32B32G32R32F,next->plane_->pixels().data(),next->plane_->width()*16);
        const std::array<uint8_t,4> bgra{albedo[2],albedo[1],albedo[0],albedo[3]};upload(next->base_,1,1,D3DFMT_A8R8G8B8,bgra.data(),4);
        next->target_.create([&](auto** p){return d->CreateRenderTarget(32,32,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,p,nullptr);});stage(hook,"target-created");
        next->readback_.create([&](auto** p){return d->CreateOffscreenPlainSurface(32,32,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,p,nullptr);});stage(hook,"readback-created");return next;
    }
    std::vector<float> render(const std::array<D3D9BaseLightmapVertexCandidate,3>& vertices,const Hook& hook={}){
        for(const auto& v:vertices){for(float x:v.clip)d3d9SceneRequireCandidate(std::isfinite(x),"finite lightmap clip");for(float x:v.luxel)d3d9SceneRequireCandidate(std::isfinite(x)&&std::abs(x)<=8192,"bounded lightmap luxel");d3d9SceneRequireCandidate(v.clip[3]>0&&std::abs(v.clip[0])<v.clip[3]&&std::abs(v.clip[1])<v.clip[3]&&v.clip[2]>0&&v.clip[2]<v.clip[3],"unclipped lightmap proof only");}
        auto*d=device_.get();D3D9SceneComCandidate<IDirect3DSurface9> oldTarget,oldDepth;
        oldTarget.create([&](auto** p){return d->GetRenderTarget(0,p);});IDirect3DSurface9* old=nullptr;const auto hr=d->GetDepthStencilSurface(&old);if(hr!=D3DERR_NOTFOUND)d3d9SceneCheckCandidate(hr);oldDepth.reset(old);
        bool begun=false,locked=false;
        auto cleanup=[&]{if(locked)readback_->UnlockRect();if(begun)d->EndScene();for(DWORD n=0;n<2;++n)d->SetTexture(n,nullptr);d->SetStreamSource(0,nullptr,0,0);d->SetStreamSource(1,nullptr,0,0);d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetVertexDeclaration(nullptr);d->SetRenderTarget(0,oldTarget.get());d->SetDepthStencilSurface(oldDepth.get());};
        try{
            d3d9SceneCheckCandidate(d->SetDepthStencilSurface(nullptr));d3d9SceneCheckCandidate(d->SetRenderTarget(0,target_.get()));D3DVIEWPORT9 viewport{0,0,32,32,0,1};d3d9SceneCheckCandidate(d->SetViewport(&viewport));stage(hook,"target-bound");
            const std::pair<D3DRENDERSTATETYPE,DWORD> states[]{{D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_ALPHABLENDENABLE,FALSE},{D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_COLORWRITEENABLE,15},{D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_MULTISAMPLEANTIALIAS,FALSE}};
            for(const auto& s:states)d3d9SceneCheckCandidate(d->SetRenderState(s.first,s.second));
            for(DWORD n=0;n<2;++n){d3d9SceneCheckCandidate(d->SetTexture(n,n?base_.get():light_.get()));
                const std::pair<D3DSAMPLERSTATETYPE,DWORD> states2[]{{D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP},{D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP},{D3DSAMP_MINFILTER,D3DTEXF_POINT},{D3DSAMP_MAGFILTER,D3DTEXF_POINT},{D3DSAMP_MIPFILTER,D3DTEXF_NONE},{D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MAXANISOTROPY,1},{D3DSAMP_SRGBTEXTURE,n?TRUE:FALSE}};
                for(const auto& s:states2){d3d9SceneCheckCandidate(d->SetSamplerState(n,s.first,s.second));DWORD observed=0;d3d9SceneCheckCandidate(d->GetSamplerState(n,s.first,&observed));d3d9SceneRequireCandidate(observed==s.second,"lightmap sampler state mismatch");}}
            stage(hook,"textures-bound");d3d9SceneCheckCandidate(d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0));
            d3d9SceneCheckCandidate(d->SetVertexDeclaration(declaration_.get()));d3d9SceneCheckCandidate(d->SetVertexShader(vertex_.get()));d3d9SceneCheckCandidate(d->SetPixelShader(pixel_.get()));
            const float dims[]{1.f/plane_->width(),1.f/plane_->height(),0,0};d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(0,dims,1));d3d9SceneCheckCandidate(d->SetPixelShaderConstantF(1,modulation_.data(),1));
            d3d9SceneCheckCandidate(d->BeginScene());begun=true;stage(hook,"scene-begun");
            d3d9SceneCheckCandidate(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,vertices.data(),sizeof(vertices[0])));stage(hook,"draw-completed");
            d3d9SceneCheckCandidate(d->EndScene());begun=false;
            d3d9SceneCheckCandidate(d->GetRenderTargetData(target_.get(),readback_.get()));stage(hook,"readback-copied");
            D3DLOCKED_RECT lock{};d3d9SceneCheckCandidate(readback_->LockRect(&lock,nullptr,D3DLOCK_READONLY));locked=true;stage(hook,"readback-locked");
            std::vector<float> pixels(4096);for(size_t y=0;y<32;++y)std::memcpy(pixels.data()+y*128,static_cast<uint8_t*>(lock.pBits)+y*size_t(lock.Pitch),512);
            d3d9SceneCheckCandidate(readback_->UnlockRect());locked=false;cleanup();return pixels;
        }catch(...){cleanup();throw;}
    }
};

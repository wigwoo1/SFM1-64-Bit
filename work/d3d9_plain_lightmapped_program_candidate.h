#pragma once
#include "d3d9_prepared_scene_candidate.h"
#include "source_plain_lightmapped_shader_candidate.h"

// Owned GPU program objects only. Uses existing COM ownership; does not bind the
// device, generate scene constants, load assets or select a material/render path.
// Prepare a replacement completely before assigning it to a committed owner.
class D3D9PlainLightmappedProgramCandidate {
public:
    using Hook=std::function<void(const char*)>;
private:
    // Declared first so the retained device is released after all its objects.
    D3D9SceneComCandidate<IDirect3DDevice9> device_;
    D3D9SceneComCandidate<IDirect3DVertexShader9> vertex_;
    D3D9SceneComCandidate<IDirect3DPixelShader9> pixel_;
    D3D9SceneComCandidate<IDirect3DVertexDeclaration9> declaration_;
    D3D9PlainLightmappedProgramCandidate()=default;
    static void stage(const Hook& hook,const char* name){if(hook)hook(name);}
    static void compile(D3D9SceneComCandidate<ID3DBlob>& blob,const char* source,
                        const char* name,const char* profile){
        ID3DBlob* compiled=nullptr;ID3DBlob* diagnostics=nullptr;
        const HRESULT result=D3DCompile(source,std::strlen(source),name,nullptr,
            nullptr,"main",profile,D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY|
            D3DCOMPILE_IEEE_STRICTNESS,0,&compiled,&diagnostics);
        blob.reset(compiled);D3D9SceneComCandidate<ID3DBlob> errors;errors.reset(diagnostics);
        if(FAILED(result)){
            if(errors.get())throw std::runtime_error(std::string("plain lightmapped shader compile: ")+
                std::string(static_cast<const char*>(errors->GetBufferPointer()),errors->GetBufferSize()));
            d3d9SceneCheckCandidate(result);
        }
        d3d9SceneRequireCandidate(blob.get()!=nullptr,"plain lightmapped compiler returned null output");
        d3d9SceneRequireCandidate(!errors.get()||errors->GetBufferSize()==0,"plain lightmapped compiler warning");
    }
public:
    D3D9PlainLightmappedProgramCandidate(const D3D9PlainLightmappedProgramCandidate&)=delete;
    D3D9PlainLightmappedProgramCandidate& operator=(const D3D9PlainLightmappedProgramCandidate&)=delete;
    static std::unique_ptr<D3D9PlainLightmappedProgramCandidate> prepare(
            IDirect3DDevice9* device,const Hook& hook={}){
        d3d9SceneRequireCandidate(device!=nullptr,"plain lightmapped program requires device");
        auto next=std::unique_ptr<D3D9PlainLightmappedProgramCandidate>(new D3D9PlainLightmappedProgramCandidate);
        device->AddRef();next->device_.reset(device);stage(hook,"device-retained");
        D3D9SceneComCandidate<ID3DBlob> vs,ps;
        compile(vs,sourcePlainLightmappedVertexShaderCandidate,"owned-vs.hlsl","vs_2_0");stage(hook,"vertex-compiled");
        compile(ps,sourcePlainLightmappedPixelShaderCandidate,"owned-ps.hlsl","ps_3_0");stage(hook,"pixel-compiled");
        next->vertex_.create([&](auto** p){return device->CreateVertexShader(static_cast<const DWORD*>(vs->GetBufferPointer()),p);});stage(hook,"vertex-created");
        next->pixel_.create([&](auto** p){return device->CreatePixelShader(static_cast<const DWORD*>(ps->GetBufferPointer()),p);});stage(hook,"pixel-created");
        const D3DVERTEXELEMENT9 elements[]{
            {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,12,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},
            {0,20,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,1},D3DDECL_END()};
        next->declaration_.create([&](auto** p){return device->CreateVertexDeclaration(elements,p);});stage(hook,"declaration-created");
        return next;
    }
    // Borrowed objects: remain valid only while this owner is retained.
    IDirect3DVertexShader9* vertexShader()const noexcept{return vertex_.get();}
    IDirect3DPixelShader9* pixelShader()const noexcept{return pixel_.get();}
    IDirect3DVertexDeclaration9* declaration()const noexcept{return declaration_.get();}
};

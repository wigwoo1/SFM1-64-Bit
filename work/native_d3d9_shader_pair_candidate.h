#pragma once
#include <d3d9.h>
#include "vcs6_shader_bytecode_candidate.h"

class NativeD3d9ShaderPairCandidate {
    std::shared_ptr<IDirect3DVertexShader9> vertex_;
    std::shared_ptr<IDirect3DPixelShader9> pixel_;
    static std::vector<DWORD> words(const std::vector<uint8_t>& bytes,uint32_t stage) {
        if(bytes.size()<8||bytes.size()%4)throw std::runtime_error("D3D9 shader byte count");
        std::vector<DWORD> result(bytes.size()/4);
        for(size_t i=0;i<result.size();++i)result[i]=uint32_t(bytes[i*4])|
            (uint32_t(bytes[i*4+1])<<8)|(uint32_t(bytes[i*4+2])<<16)|(uint32_t(bytes[i*4+3])<<24);
        if((result.front()>>16)!=stage||result.back()!=0x0000ffff)
            throw std::runtime_error("D3D9 shader stage/envelope");
        return result;
    }
public:
    NativeD3d9ShaderPairCandidate(IDirect3DDevice9* device,
        const std::vector<uint8_t>& vertex,const std::vector<uint8_t>& pixel) {
        auto vs=words(vertex,0xfffe),ps=words(pixel,0xffff);
        if(!device)throw std::runtime_error("D3D9 shader device missing");
        IDirect3DVertexShader9* vertexObject=nullptr;
        auto result=device->CreateVertexShader(vs.data(),&vertexObject);
        if(FAILED(result)||!vertexObject){if(vertexObject)vertexObject->Release();throw std::runtime_error("D3D9 CreateVertexShader failed");}
        vertex_.reset(vertexObject,[](IDirect3DVertexShader9* object){object->Release();});
        IDirect3DPixelShader9* pixelObject=nullptr;
        result=device->CreatePixelShader(ps.data(),&pixelObject);
        if(FAILED(result)||!pixelObject){if(pixelObject)pixelObject->Release();throw std::runtime_error("D3D9 CreatePixelShader failed");}
        pixel_.reset(pixelObject,[](IDirect3DPixelShader9* object){object->Release();});
    }
    void bindAndVerify(IDirect3DDevice9* device) const {
        if(!device)throw std::runtime_error("D3D9 shader bind device missing");
        if(FAILED(device->SetVertexShader(vertex_.get()))||FAILED(device->SetPixelShader(pixel_.get())))
            throw std::runtime_error("D3D9 shader binding failed");
        IDirect3DVertexShader9* vs=nullptr;IDirect3DPixelShader9* ps=nullptr;
        auto vertexResult=device->GetVertexShader(&vs),pixelResult=device->GetPixelShader(&ps);
        bool equal=SUCCEEDED(vertexResult)&&SUCCEEDED(pixelResult)&&vs==vertex_.get()&&ps==pixel_.get();
        if(vs)vs->Release();if(ps)ps->Release();
        if(!equal)throw std::runtime_error("D3D9 shader binding readback mismatch");
    }
};

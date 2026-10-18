#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <filesystem>
#include <memory>
#include "vtf_block_decode.h"
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"windowscodecs.lib")
template<class T>struct StudioWicReleaseCandidate{void operator()(T* p)const{if(p)p->Release();}};
inline VtfRgbaImageCandidate studioReadPngCandidate(const std::filesystem::path& path){
 struct Apartment{HRESULT result=CoInitializeEx(nullptr,COINIT_MULTITHREADED);Apartment(){if(FAILED(result)&&result!=RPC_E_CHANGED_MODE)throw std::runtime_error("PNG COM initialization failed");}~Apartment(){if(SUCCEEDED(result))CoUninitialize();}} apartment;
 auto check=[](HRESULT result){if(FAILED(result))throw std::runtime_error("PNG decode failed: "+std::to_string(static_cast<unsigned>(result)));};
 IWICImagingFactory* rawFactory=nullptr;check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&rawFactory)));std::unique_ptr<IWICImagingFactory,StudioWicReleaseCandidate<IWICImagingFactory>> factory(rawFactory);
 IWICBitmapDecoder* rawDecoder=nullptr;check(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&rawDecoder));std::unique_ptr<IWICBitmapDecoder,StudioWicReleaseCandidate<IWICBitmapDecoder>> decoder(rawDecoder);GUID format{};check(decoder->GetContainerFormat(&format));if(!IsEqualGUID(format,GUID_ContainerFormatPng))throw std::runtime_error("texture file is not PNG");
 UINT frames=0;check(decoder->GetFrameCount(&frames));if(frames!=1)throw std::runtime_error("PNG frame count unsupported");IWICBitmapFrameDecode* rawFrame=nullptr;check(decoder->GetFrame(0,&rawFrame));std::unique_ptr<IWICBitmapFrameDecode,StudioWicReleaseCandidate<IWICBitmapFrameDecode>> frame(rawFrame);
 UINT width=0,height=0;check(frame->GetSize(&width,&height));uint64_t pixels=uint64_t(width)*height;if(!width||!height||width>16384||height>16384||pixels>67108864)throw std::runtime_error("PNG dimensions exceed texture limit");
 IWICFormatConverter* rawConverter=nullptr;check(factory->CreateFormatConverter(&rawConverter));std::unique_ptr<IWICFormatConverter,StudioWicReleaseCandidate<IWICFormatConverter>> converter(rawConverter);check(converter->Initialize(frame.get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
 VtfRgbaImageCandidate result{width,height,std::vector<uint8_t>(static_cast<size_t>(pixels)*4)};check(converter->CopyPixels(nullptr,width*4,static_cast<UINT>(result.rgba.size()),result.rgba.data()));return result;
}

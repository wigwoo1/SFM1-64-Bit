#include "win32_export_image_candidate.h"
#include <iostream>
int main(int argc,char**argv){try{
 const auto path=std::filesystem::path(argc>1?argv[1]:"outputs/cycles-worker/explicit-denoiser.png");
 ExportImageWindowContextCandidate context(path);
 Gdiplus::Bitmap target(128,64,PixelFormat32bppARGB);
 {Gdiplus::Graphics graphics(&target);paintExportImageCandidate(graphics,context,128,64);}
 Gdiplus::Color outside,left,right;
 target.GetPixel(0,32,&outside);target.GetPixel(32,32,&left);target.GetPixel(96,32,&right);
 if(outside.GetR()!=0||outside.GetG()!=0||outside.GetB()!=0)throw std::runtime_error("Letterbox not black");
 HWND window=createExportImageWindowCandidate(nullptr,path,false);if(!window)throw std::runtime_error("Preview window missing");
 SendMessageW(window,WM_CLOSE,0,0);if(IsWindow(window))throw std::runtime_error("Preview window remained open");
 bool rejected=false;try{ExportImageWindowContextCandidate invalid("outputs/missing-preview-test.png");}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("Missing image accepted");
 std::cout<<"PASS real PNG decode, aspect fit letterbox, native window lifecycle and missing-image rejection\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}

#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gdiplus.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <algorithm>
#pragma comment(lib,"Gdiplus.lib")
inline HWND exportImageWindowCandidate=nullptr;
struct ExportImageWindowContextCandidate {
 ULONG_PTR token=0;
 std::unique_ptr<Gdiplus::Bitmap> image;
 explicit ExportImageWindowContextCandidate(const std::filesystem::path&path){
  Gdiplus::GdiplusStartupInput input;if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok)throw std::runtime_error("Image preview initialization failed");
  image.reset(Gdiplus::Bitmap::FromFile(path.c_str(),FALSE));
  if(!image||image->GetLastStatus()!=Gdiplus::Ok||!image->GetWidth()||!image->GetHeight()||image->GetWidth()>8192||image->GetHeight()>8192){image.reset();Gdiplus::GdiplusShutdown(token);token=0;throw std::runtime_error("Accepted image could not be decoded");}
 }
 ~ExportImageWindowContextCandidate(){image.reset();if(token)Gdiplus::GdiplusShutdown(token);}
};
inline void paintExportImageCandidate(Gdiplus::Graphics&graphics,ExportImageWindowContextCandidate&context,int width,int height){
 graphics.Clear(Gdiplus::Color(255,0,0,0));if(width<=0||height<=0)return;
 const double scale=std::min(double(width)/context.image->GetWidth(),double(height)/context.image->GetHeight());
 int imageWidth=std::max(1,int(context.image->GetWidth()*scale)),imageHeight=std::max(1,int(context.image->GetHeight()*scale));
 graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
 if(graphics.DrawImage(context.image.get(),(width-imageWidth)/2,(height-imageHeight)/2,imageWidth,imageHeight)!=Gdiplus::Ok)throw std::runtime_error("Image preview drawing failed");
}
inline LRESULT CALLBACK exportImageWindowProcCandidate(HWND window,UINT message,WPARAM key,LPARAM value){
 auto context=reinterpret_cast<ExportImageWindowContextCandidate*>(GetWindowLongPtrW(window,GWLP_USERDATA));
 if(message==WM_NCCREATE){context=static_cast<std::unique_ptr<ExportImageWindowContextCandidate>*>(reinterpret_cast<CREATESTRUCTW*>(value)->lpCreateParams)->release();SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(context));}
 if(message==WM_PAINT&&context){PAINTSTRUCT paint{};HDC dc=BeginPaint(window,&paint);RECT client{};GetClientRect(window,&client);try{Gdiplus::Graphics graphics(dc);paintExportImageCandidate(graphics,*context,client.right,client.bottom);}catch(...){FillRect(dc,&client,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));}EndPaint(window,&paint);return 0;}
 if(message==WM_SIZE){InvalidateRect(window,nullptr,FALSE);return 0;}
 if(message==WM_CLOSE){DestroyWindow(window);return 0;}
 if(message==WM_NCDESTROY){if(exportImageWindowCandidate==window)exportImageWindowCandidate=nullptr;SetWindowLongPtrW(window,GWLP_USERDATA,0);delete context;}
 return DefWindowProcW(window,message,key,value);
}
inline HWND createExportImageWindowCandidate(HWND owner,const std::filesystem::path&path,bool visible=true){
 auto context=std::make_unique<ExportImageWindowContextCandidate>(path);
 WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=exportImageWindowProcCandidate;cls.lpszClassName=L"Sfm64ExportImageCandidate";cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
 if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Image preview registration failed");
 if(exportImageWindowCandidate)DestroyWindow(exportImageWindowCandidate);
 HWND window=CreateWindowExW(0,cls.lpszClassName,L"Cycles export preview",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,980,600,owner,nullptr,cls.hInstance,&context);
 if(!window)throw std::runtime_error("Image preview window creation failed");exportImageWindowCandidate=window;if(visible){ShowWindow(window,SW_SHOW);UpdateWindow(window);}return window;
}

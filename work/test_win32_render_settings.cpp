#define NOMINMAX
#include "win32_render_settings_candidate.h"
#include <cassert>
#include <iostream>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
static HWND modalOwner;
static UINT modalCommand;
static bool modalMutate,modalVisited;
static UINT modalExpectedUiScale=100;
static PlaybackRenderSettingsCandidate modalBefore;
static bool captureRenderSettingsScreenshotCandidate(HWND window,const wchar_t*path){
 RECT bounds{};if(!GetWindowRect(window,&bounds))return false;
 const int width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;if(width<=0||height<=0)return false;
 HDC screen=GetDC(nullptr),memory=screen?CreateCompatibleDC(screen):nullptr;
 HBITMAP bitmap=screen?CreateCompatibleBitmap(screen,width,height):nullptr;
 HGDIOBJ prior=memory&&bitmap?SelectObject(memory,bitmap):nullptr;
 const bool drawn=memory&&bitmap&&prior&&PrintWindow(window,memory,PW_RENDERFULLCONTENT);
 Gdiplus::Bitmap image(bitmap,nullptr);
 const CLSID pngEncoder={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
 const bool saved=drawn&&image.Save(path,&pngEncoder,nullptr)==Gdiplus::Ok;
 if(prior)SelectObject(memory,prior);if(bitmap)DeleteObject(bitmap);if(memory)DeleteDC(memory);if(screen)ReleaseDC(nullptr,screen);
 return saved;
}
static void CALLBACK exerciseModal(HWND window,UINT,UINT_PTR timer,DWORD){
 KillTimer(window,timer);
 assert(!IsWindowEnabled(modalOwner));
 assert(sameRenderSettingsCandidate(playbackRenderSettingsCandidate,modalBefore));
 auto context=reinterpret_cast<RenderSettingsDialogCandidate*>(GetWindowLongPtrW(window,GWLP_USERDATA));assert(context&&context->uiScalePercent==modalExpectedUiScale);
 RECT scaled{};GetWindowRect(context->renderer,&scaled);assert(scaled.right-scaled.left==renderSettingsScaleMetricCandidate(250,context->dpi,modalExpectedUiScale));
 if(modalMutate){
  SendMessageW(GetDlgItem(window,1),CB_SETCURSEL,2,0);
  SendMessageW(window,WM_COMMAND,MAKEWPARAM(1,CBN_SELCHANGE),LPARAM(GetDlgItem(window,1)));
  SendMessageW(GetDlgItem(window,2),CB_SETCURSEL,2,0);
  SetWindowTextW(GetDlgItem(window,3),L"512");
  SetWindowTextW(GetDlgItem(window,4),L"1e-8");
  SendMessageW(GetDlgItem(window,5),BM_SETCHECK,BST_UNCHECKED,0);
 }
 if(modalExpectedUiScale==150){wchar_t path[32768]{};DWORD length=GetEnvironmentVariableW(L"SFM64_RENDER_SETTINGS_CAPTURE",path,32768);if(length&&length<32768)assert(captureRenderSettingsScreenshotCandidate(window,path));}
 ShowWindow(window,SW_HIDE);
 modalVisited=true;
 SendMessageW(window,WM_COMMAND,modalCommand,0);
}
static LRESULT CALLBACK modalHook(int code,WPARAM key,LPARAM value){
 if(code==HCBT_ACTIVATE){
  auto window=reinterpret_cast<HWND>(key);wchar_t name[128]{};
  GetClassNameW(window,name,128);
  if(std::wstring(name)==L"Sfm64RenderSettingsCandidate")assert(SetTimer(window,91,10,exerciseModal));
 }
 return CallNextHookEx(nullptr,code,key,value);
}
int main(){
 Gdiplus::GdiplusStartupInput gdiplusInput;ULONG_PTR gdiplusToken=0;const bool gdiplusReady=Gdiplus::GdiplusStartup(&gdiplusToken,&gdiplusInput,nullptr)==Gdiplus::Ok;assert(gdiplusReady);
 assert(renderSettingsScaleMetricCandidate(16,96)==16&&renderSettingsScaleMetricCandidate(16,96,150)==24&&renderSettingsScaleMetricCandidate(250,144)==375&&renderSettingsScaleMetricCandidate(250,144,150)==563&&renderSettingsScaleMetricCandidate(250,192)==500&&renderSettingsScaleMetricCandidate(250,192,250)==1250);
 WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=renderSettingsWindowProcCandidate;cls.lpszClassName=L"Sfm64RenderSettingsTest";
 assert(RegisterClassW(&cls));
 int preserved=0;
 for(double threshold:{0.0,1.0,.01,.025,1e-8,std::nextafter(.025,1.0),std::numeric_limits<double>::min(),std::numeric_limits<double>::denorm_min()}){
  RenderSettingsDialogCandidate context;context.staged.noiseThreshold=threshold;
  auto before=context.staged;
  HWND window=CreateWindowExW(0,cls.lpszClassName,L"test",WS_OVERLAPPED,0,0,414,340,nullptr,nullptr,cls.hInstance,&context);assert(window);
  SendMessageW(window,WM_COMMAND,IDOK,0);
  assert(context.closed&&context.accepted&&sameRenderSettingsCandidate(before,context.staged));
  ++preserved;
 }
 for(int resolution=0;resolution<3;++resolution){
  RenderSettingsDialogCandidate context;
  HWND window=CreateWindowExW(0,cls.lpszClassName,L"test",WS_OVERLAPPED,0,0,414,340,nullptr,nullptr,cls.hInstance,&context);assert(window);
  assert(!IsWindowEnabled(context.samples)&&!IsWindowEnabled(context.threshold)&&IsWindowEnabled(context.resolution));
  SendMessageW(context.renderer,CB_SETCURSEL,2,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1,CBN_SELCHANGE),LPARAM(context.renderer));
  assert(IsWindowEnabled(context.samples)&&IsWindowEnabled(context.threshold)&&IsWindowEnabled(context.adaptive)&&IsWindowEnabled(context.denoising));
  SendMessageW(context.resolution,CB_SETCURSEL,resolution,0);
  SetWindowTextW(context.samples,L"256");SetWindowTextW(context.threshold,L"0.025");
  SendMessageW(context.adaptive,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(context.denoising,BM_SETCHECK,BST_CHECKED,0);
  SendMessageW(context.renderer,CB_SETCURSEL,0,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1,CBN_SELCHANGE),LPARAM(context.renderer));
  assert(!IsWindowEnabled(context.samples)&&!IsWindowEnabled(context.threshold)&&!IsWindowEnabled(context.adaptive)&&!IsWindowEnabled(context.denoising));
  SendMessageW(context.renderer,CB_SETCURSEL,2,0);SendMessageW(window,WM_COMMAND,MAKEWPARAM(1,CBN_SELCHANGE),LPARAM(context.renderer));
  assert(IsWindowEnabled(context.samples)&&IsWindowEnabled(context.threshold)&&SendMessageW(context.samples,WM_GETTEXTLENGTH,0,0)==3);
  SendMessageW(window,WM_COMMAND,IDOK,0);
  assert(context.closed&&context.accepted&&context.staged.renderer==2&&context.staged.resolution==resolution);
  assert(context.staged.samples==256&&context.staged.noiseThreshold==.025&&!context.staged.adaptive&&context.staged.denoising);
 }
 RenderSettingsDialogCandidate context;auto before=context.staged;
 HWND window=CreateWindowExW(0,cls.lpszClassName,L"test",WS_OVERLAPPED,0,0,414,340,nullptr,nullptr,cls.hInstance,&context);assert(window);
 SetWindowTextW(context.samples,L"999");SendMessageW(window,WM_COMMAND,IDCANCEL,0);
 assert(context.closed&&!context.accepted&&context.staged.samples==before.samples);
 RenderSettingsDialogCandidate dpiContext;
 HWND dpiWindow=CreateWindowExW(0,cls.lpszClassName,L"dpi test",WS_OVERLAPPED,0,0,414,340,nullptr,nullptr,cls.hInstance,&dpiContext);assert(dpiWindow);
 RECT suggested{40,50,868,730};
 SendMessageW(dpiWindow,WM_DPICHANGED,MAKELONG(192,192),LPARAM(&suggested));
 assert(dpiContext.dpi==192);
 RECT scaledControl{};GetWindowRect(dpiContext.renderer,&scaledControl);
 assert(scaledControl.right-scaledControl.left==renderSettingsScaleMetricCandidate(250,192));
 SendMessageW(dpiWindow,WM_CLOSE,0,0);assert(dpiContext.closed);
 modalOwner=CreateWindowExW(0,L"STATIC",L"modal test owner",WS_OVERLAPPED,0,0,80,80,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);assert(modalOwner);
 auto hook=SetWindowsHookExW(WH_CBT,modalHook,nullptr,GetCurrentThreadId());assert(hook);
 for(int mode=0;mode<3;++mode){
  playbackRenderSettingsCandidate={1,0,128,1e-8,true,true};
  modalBefore=playbackRenderSettingsCandidate;modalVisited=false;
  modalCommand=mode==1?IDCANCEL:IDOK;modalMutate=mode!=0;modalExpectedUiScale=mode==2?150:100;
  bool accepted=showRenderSettingsCandidate(modalOwner,modalExpectedUiScale);
  assert(modalVisited&&IsWindowEnabled(modalOwner)&&accepted==(mode!=1));
  if(mode!=2)assert(sameRenderSettingsCandidate(playbackRenderSettingsCandidate,modalBefore));
  else assert(sameRenderSettingsCandidate(playbackRenderSettingsCandidate,{2,2,512,1e-8,false,true}));
 }
 assert(UnhookWindowsHookEx(hook));DestroyWindow(modalOwner);
 Gdiplus::GdiplusShutdown(gdiplusToken);
 std::cout<<"PASS native Win32 controls: "<<preserved<<" exact threshold reopen/apply cases; Cycles selections at all three resolutions, quality values, flags, 96/144/192 DPI metrics, 192-DPI layout transition and cancel isolation\n";
 std::cout<<"PASS actual modal loop: unchanged Apply, changed Cancel, changed Apply, staged isolation and owner restoration\n";
}

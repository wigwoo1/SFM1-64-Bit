#pragma once
#include <windows.h>
#include <algorithm>
#include <string>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <initializer_list>
#include <limits>

#include "render_settings_candidate.h"
inline PlaybackRenderSettingsCandidate playbackRenderSettingsCandidate;
inline PlaybackRenderSettingsCandidate playbackSavedRenderSettingsCandidate;
inline std::wstring renderThresholdTextCandidate(double value){
 std::wostringstream output;output.imbue(std::locale::classic());
 output<<std::setprecision(std::numeric_limits<double>::max_digits10)<<value;
 return output.str();
}
struct RenderSettingsDialogCandidate {
 PlaybackRenderSettingsCandidate staged;
 bool accepted=false,closed=false;
 HWND renderer{},resolution{},samples{},threshold{},adaptive{},denoising{};
 HWND rendererLabel{},resolutionLabel{},samplesLabel{},thresholdLabel{},information{},apply{},cancel{};
 HFONT uiFont{};
 UINT dpi=96;
 UINT uiScalePercent=100;
};
inline int renderSettingsScaleMetricCandidate(int value,UINT dpi,UINT uiScalePercent=100){
 const UINT boundedDpi=std::clamp(dpi,48u,768u),boundedScale=std::clamp(uiScalePercent,75u,250u);
 return MulDiv(value,int(boundedDpi*boundedScale),9600);
}
inline void updateRenderSettingsEngineControlsCandidate(RenderSettingsDialogCandidate& context){
 const bool cycles=SendMessageW(context.renderer,CB_GETCURSEL,0,0)==2;
 for(HWND control:{context.samplesLabel,context.samples,context.thresholdLabel,context.threshold,context.adaptive,context.denoising})
  if(control)EnableWindow(control,cycles);
}
inline void layoutRenderSettingsControlsCandidate(HWND window,RenderSettingsDialogCandidate& context){
 auto metric=[&](int value){return renderSettingsScaleMetricCandidate(value,context.dpi,context.uiScalePercent);};
 auto place=[&](HWND child,int x,int y,int width,int height){if(child)SetWindowPos(child,nullptr,metric(x),metric(y),metric(width),metric(height),SWP_NOZORDER|SWP_NOACTIVATE);};
 place(context.rendererLabel,16,16,110,22);place(context.renderer,130,12,250,150);
 place(context.resolutionLabel,16,52,110,22);place(context.resolution,130,48,250,150);
 place(context.samplesLabel,16,90,110,22);place(context.samples,130,86,250,24);
 place(context.thresholdLabel,16,126,110,22);place(context.threshold,130,122,250,24);
 place(context.adaptive,16,164,170,24);place(context.denoising,200,164,170,24);
 place(context.information,16,204,364,48);place(context.apply,196,266,88,28);place(context.cancel,292,266,88,28);
 if(context.uiFont)for(HWND child=GetWindow(window,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(context.uiFont),TRUE);
}
inline LRESULT CALLBACK renderSettingsWindowProcCandidate(HWND window,UINT message,WPARAM key,LPARAM value){
 auto context=reinterpret_cast<RenderSettingsDialogCandidate*>(GetWindowLongPtrW(window,GWLP_USERDATA));
 if(message==WM_NCCREATE){context=static_cast<RenderSettingsDialogCandidate*>(reinterpret_cast<CREATESTRUCTW*>(value)->lpCreateParams);SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(context));}
 if(!context)return DefWindowProcW(window,message,key,value);
 if(message==WM_CREATE){
  context->dpi=GetDpiForWindow(window);if(!context->dpi)context->dpi=96;
  context->uiFont=CreateFontW(-MulDiv(9,int(context->dpi*context->uiScalePercent),7200),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
  auto control=[&](const wchar_t*kind,const wchar_t*text,DWORD style,int x,int y,int width,int height,int id){
   HWND child=CreateWindowExW(0,kind,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,window,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
   SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(context->uiFont?context->uiFont:GetStockObject(DEFAULT_GUI_FONT)),TRUE);return child;
  };
  context->rendererLabel=control(L"STATIC",L"Renderer",0,16,16,110,22,0);
  context->renderer=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,130,12,250,150,1);
  for(auto text:{L"Source native",L"Vulkan",L"Blender Cycles"})SendMessageW(context->renderer,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
  SendMessageW(context->renderer,CB_SETCURSEL,context->staged.renderer,0);
  context->resolutionLabel=control(L"STATIC",L"Resolution",0,16,52,110,22,0);
  context->resolution=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,130,48,250,150,2);
  for(auto text:{L"1920 x 1080",L"2560 x 1440",L"3840 x 2160"})SendMessageW(context->resolution,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
  SendMessageW(context->resolution,CB_SETCURSEL,context->staged.resolution,0);
  context->samplesLabel=control(L"STATIC",L"Cycles samples",0,16,90,110,22,0);
  context->samples=control(L"EDIT",std::to_wstring(context->staged.samples).c_str(),WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,130,86,250,24,3);
  context->thresholdLabel=control(L"STATIC",L"Noise threshold",0,16,126,110,22,0);
  context->threshold=control(L"EDIT",renderThresholdTextCandidate(context->staged.noiseThreshold).c_str(),WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,130,122,250,24,4);
  context->adaptive=control(L"BUTTON",L"Adaptive sampling",BS_AUTOCHECKBOX|WS_TABSTOP,16,164,170,24,5);
  context->denoising=control(L"BUTTON",L"Denoising",BS_AUTOCHECKBOX|WS_TABSTOP,200,164,170,24,6);
  SendMessageW(context->adaptive,BM_SETCHECK,context->staged.adaptive?BST_CHECKED:BST_UNCHECKED,0);
  SendMessageW(context->denoising,BM_SETCHECK,context->staged.denoising?BST_CHECKED:BST_UNCHECKED,0);
  context->information=control(L"STATIC",L"Export supports the selected shot's first frame with Cycles. Source and Vulkan export and viewport renderer switching are pending.",0,16,204,364,48,0);
  context->apply=control(L"BUTTON",L"Apply",BS_DEFPUSHBUTTON|WS_TABSTOP,196,266,88,28,IDOK);
  context->cancel=control(L"BUTTON",L"Cancel",WS_TABSTOP,292,266,88,28,IDCANCEL);
  layoutRenderSettingsControlsCandidate(window,*context);
  updateRenderSettingsEngineControlsCandidate(*context);
  return 0;
 }
 if(message==WM_DPICHANGED){
  const UINT nextDpi=HIWORD(key);if(nextDpi>=48&&nextDpi<=768)context->dpi=nextDpi;
  const auto suggested=reinterpret_cast<const RECT*>(value);
  if(suggested)SetWindowPos(window,nullptr,suggested->left,suggested->top,suggested->right-suggested->left,suggested->bottom-suggested->top,SWP_NOZORDER|SWP_NOACTIVATE);
  HFONT nextFont=CreateFontW(-MulDiv(9,int(context->dpi*context->uiScalePercent),7200),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
  if(nextFont){HFONT previous=context->uiFont;context->uiFont=nextFont;layoutRenderSettingsControlsCandidate(window,*context);if(previous)DeleteObject(previous);}else layoutRenderSettingsControlsCandidate(window,*context);
  return 0;
 }
 if(message==WM_COMMAND&&LOWORD(key)==1&&HIWORD(key)==CBN_SELCHANGE&&HWND(value)==context->renderer){updateRenderSettingsEngineControlsCandidate(*context);return 0;}
 if(message==WM_COMMAND&&LOWORD(key)==IDOK){
  char samples[128]{},threshold[128]{};
  GetWindowTextA(context->samples,samples,128);GetWindowTextA(context->threshold,threshold,128);
  int count=0;double noise=0;
  auto sampleEnd=samples+std::char_traits<char>::length(samples),noiseEnd=threshold+std::char_traits<char>::length(threshold);
  auto a=std::from_chars(samples,sampleEnd,count);auto b=std::from_chars(threshold,noiseEnd,noise);
  int renderer=int(SendMessageW(context->renderer,CB_GETCURSEL,0,0)),resolution=int(SendMessageW(context->resolution,CB_GETCURSEL,0,0));
  if(a.ec!=std::errc{}||a.ptr!=sampleEnd||b.ec!=std::errc{}||b.ptr!=noiseEnd||count<1||!std::isfinite(noise)||noise<0||noise>1||renderer<0||renderer>2||resolution<0||resolution>2){
   MessageBoxW(window,L"Enter a positive sample count and a noise threshold between 0 and 1.",L"Invalid render settings",MB_OK|MB_ICONERROR);return 0;
  }
  context->staged={renderer,resolution,count,noise,SendMessageW(context->adaptive,BM_GETCHECK,0,0)==BST_CHECKED,SendMessageW(context->denoising,BM_GETCHECK,0,0)==BST_CHECKED};
  context->accepted=true;DestroyWindow(window);return 0;
 }
 if(message==WM_CLOSE||(message==WM_COMMAND&&LOWORD(key)==IDCANCEL)){DestroyWindow(window);return 0;}
 if(message==WM_DESTROY){context->closed=true;return 0;}
 if(message==WM_NCDESTROY&&context->uiFont){DeleteObject(context->uiFont);context->uiFont=nullptr;}
 return DefWindowProcW(window,message,key,value);
}
inline bool showRenderSettingsCandidate(HWND owner,UINT uiScalePercent=100){
 if(uiScalePercent<75||uiScalePercent>250)throw std::runtime_error("Render settings UI scale outside 75-250 percent");
 WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=renderSettingsWindowProcCandidate;cls.lpszClassName=L"Sfm64RenderSettingsCandidate";cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));cls.hbrBackground=GetSysColorBrush(COLOR_BTNFACE);
 if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Render settings window registration failed");
 RenderSettingsDialogCandidate context;context.staged=playbackRenderSettingsCandidate;context.uiScalePercent=uiScalePercent;
 const UINT ownerDpi=owner?GetDpiForWindow(owner):96;
 HWND window=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,L"Render settings",WS_CAPTION|WS_SYSMENU, CW_USEDEFAULT,CW_USEDEFAULT,renderSettingsScaleMetricCandidate(414,ownerDpi,uiScalePercent),renderSettingsScaleMetricCandidate(340,ownerDpi,uiScalePercent),owner,nullptr,cls.hInstance,&context);
 if(!window)throw std::runtime_error("Render settings window creation failed");
 EnableWindow(owner,FALSE);ShowWindow(window,SW_SHOW);MSG message{};bool quit=false;
 while(!context.closed){auto status=GetMessageW(&message,nullptr,0,0);if(status<=0){quit=status==0;if(quit)PostQuitMessage(int(message.wParam));break;}if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
 if(IsWindow(window))DestroyWindow(window);EnableWindow(owner,TRUE);SetActiveWindow(owner);
 if(context.accepted)playbackRenderSettingsCandidate=context.staged;
 return context.accepted;
}


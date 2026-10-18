#define NOMINMAX
#include "win32_vulkan_playback_candidate.h"
#include <iostream>
static int settingsMode;
static bool settingsVisited;
static void CALLBACK exerciseEditorSettings(HWND window,UINT,UINT_PTR timer,DWORD){
 KillTimer(window,timer);ShowWindow(window,SW_HIDE);settingsVisited=true;
 if(settingsMode){
  SendMessageW(GetDlgItem(window,1),CB_SETCURSEL,settingsMode==3?1:2,0);
  SetWindowTextW(GetDlgItem(window,4),settingsMode==3?L"0.01":L"1e-8");
 }
 SendMessageW(window,WM_COMMAND,settingsMode==1?IDCANCEL:IDOK,0);
}
static LRESULT CALLBACK editorSettingsHook(int code,WPARAM key,LPARAM value){
 if(code==HCBT_ACTIVATE){wchar_t name[128]{};auto window=reinterpret_cast<HWND>(key);GetClassNameW(window,name,128);
  if(std::wstring(name)==L"Sfm64RenderSettingsCandidate"&&!SetTimer(window,92,10,exerciseEditorSettings))throw std::runtime_error("Settings test timer failed");
 }
 return CallNextHookEx(nullptr,code,key,value);
}
int main(){try{
 playbackDocumentRevisionCandidate=0;uint64_t captured=999;
 playbackEditorHandler=[&](PlaybackEditorActionCandidate action,size_t){if(action==PlaybackEditorActionCandidate::ExportShotFirstFrame){captured=playbackDocumentRevisionCandidate;return false;}return action==PlaybackEditorActionCandidate::TrimStart||action==PlaybackEditorActionCandidate::Undo;};
 playbackScalarEditorHandler=[](const PlaybackScalarEditCandidate&edit,size_t){return edit.value!=0;};
 playbackFloatKeyEditorHandler=[](const PlaybackFloatKeyEditCandidate&,size_t){return true;};
 queuePlaybackEditorActionCandidate(PlaybackEditorActionCandidate::TrimStart);
 queuePlaybackEditorActionCandidate(PlaybackEditorActionCandidate::ExportShotFirstFrame);
 queuePlaybackScalarEditCandidate({1,"test",0});
 if(!drainPlaybackEditorActionsCandidate(0)||captured!=1||playbackDocumentRevisionCandidate!=1)throw std::runtime_error("Capture/no-op ordering failed");
 queuePlaybackFloatKeyEditCandidate({1,0,2});queuePlaybackEditorActionCandidate(PlaybackEditorActionCandidate::Undo);
 drainPlaybackEditorActionsCandidate(0);if(playbackDocumentRevisionCandidate!=3||captured==playbackDocumentRevisionCandidate)throw std::runtime_error("Later edit/undo did not stale capture");
 queuePlaybackEditorActionCandidate(PlaybackEditorActionCandidate::SaveAs);if(drainPlaybackEditorActionsCandidate(0)||playbackDocumentRevisionCandidate!=3)throw std::runtime_error("Nonmutation advanced revision");
 HWND preview=createExportImageWindowCandidate(nullptr,"outputs/cycles-worker/explicit-denoiser.png",false);
 queuePlaybackScalarEditCandidate({1,"test",1});drainPlaybackEditorActionsCandidate(0);
 if(exportImageWindowCandidate||IsWindow(preview)||playbackDocumentRevisionCandidate!=4)throw std::runtime_error("Mutation left stale preview open");
 HWND owner=CreateWindowExA(0,"STATIC","editor settings test",WS_OVERLAPPED,0,0,80,80,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 if(!owner)throw std::runtime_error("Editor test owner failed");
 SetWindowLongPtrA(owner,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(vulkanPlaybackWindowProc));
 playbackDocumentDirty=false;playbackRenderSettingsCandidate={};playbackSavedRenderSettingsCandidate={};
 preview=createExportImageWindowCandidate(nullptr,"outputs/cycles-worker/explicit-denoiser.png",false);
 auto hook=SetWindowsHookExW(WH_CBT,editorSettingsHook,nullptr,GetCurrentThreadId());if(!hook)throw std::runtime_error("Editor modal hook failed");
 for(settingsMode=0;settingsMode<4;++settingsMode){
  settingsVisited=false;SendMessageW(owner,WM_COMMAND,41016,0);
  if(!settingsVisited||!IsWindowEnabled(owner))throw std::runtime_error("Editor modal did not restore owner");
  if(settingsMode<2){if(playbackDocumentRevisionCandidate!=4||playbackHasUnsavedCandidate()||exportImageWindowCandidate!=preview||!IsWindow(preview))throw std::runtime_error("No-op/cancel invalidated editor");}
  else{
   if(playbackDocumentRevisionCandidate!=uint64_t(settingsMode+3)||exportImageWindowCandidate||IsWindow(preview))throw std::runtime_error("Settings change left preview/revision stale");
   char title[256]{};GetWindowTextA(owner,title,256);
   bool expectedDirty=settingsMode==2;
   if(playbackHasUnsavedCandidate()!=expectedDirty||(std::string(title).find("prototype *")!=std::string::npos)!=expectedDirty){std::cerr<<"mode="<<settingsMode<<" dirty="<<playbackHasUnsavedCandidate()<<" title="<<title<<" settings="<<renderSettingsJsonCandidate(playbackRenderSettingsCandidate)<<'\n';throw std::runtime_error("Settings dirty/title mismatch");}
  }
 }
 UnhookWindowsHookEx(hook);DestroyWindow(owner);
 std::cout<<"PASS editor settings menu: Apply/cancel/no-op revision, preview lifecycle, dirty title and saved-baseline restoration\n";
 std::cout<<"PASS queue revision: earlier edit visible at capture, no-op/save ignored, later key edit and undo invalidate revision\n";return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}

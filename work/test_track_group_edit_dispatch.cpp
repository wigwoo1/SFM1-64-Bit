#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "win32_vulkan_playback_candidate.h"
#include <iostream>

int main(){try{
 PlaybackEditorBindingScopeCandidate scope;
 WNDCLASSA cls{};cls.hInstance=GetModuleHandleA(nullptr);cls.lpfnWndProc=vulkanPlaybackWindowProc;cls.lpszClassName="Sfm64TrackGroupDispatchTest";
 if(!RegisterClassA(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("register dispatch window");
 HWND window=CreateWindowExA(0,cls.lpszClassName,"dispatch test",WS_OVERLAPPED,0,0,640,480,nullptr,nullptr,cls.hInstance,nullptr);
 if(!window)throw std::runtime_error("create dispatch window");vulkanPlaybackState.window=window;
 createPlaybackEditorMenuCandidate(window);enablePlaybackEditorMenuCandidate(true);
 if(GetMenuState(playbackEditorMenu,SFM_EDITOR_TRACK_GROUP_VISIBILITY,MF_BYCOMMAND)==UINT(-1))throw std::runtime_error("track-group menu item missing");
 size_t actionCalls=0;playbackEditorHandler=[&](PlaybackEditorActionCandidate action,size_t){if(action!=PlaybackEditorActionCandidate::TrackGroupVisibility)throw std::runtime_error("wrong track-group command action");++actionCalls;return false;};
 SendMessageA(window,WM_COMMAND,MAKEWPARAM(41026,0),0);
 if(!playbackEditorQueue.empty())throw std::runtime_error("Graph Editor Delete key command collided with track-group action");
 SendMessageA(window,WM_COMMAND,MAKEWPARAM(SFM_EDITOR_TRACK_GROUP_VISIBILITY,0),0);
 if(playbackEditorQueue.size()!=1||!std::holds_alternative<PlaybackEditorActionCandidate>(playbackEditorQueue.front()))throw std::runtime_error("track-group menu command was not queued");
 drainPlaybackEditorActionsCandidate(7);if(actionCalls!=1)throw std::runtime_error("track-group action dispatch failed");
 size_t editCalls=0;playbackTrackGroupEditHandler=[&](const PlaybackTrackGroupEditCandidate&edit,size_t frame){if(edit.element!=19||edit.visible||edit.name!="Audio edit"||frame!=7)throw std::runtime_error("track-group edit payload changed");++editCalls;return true;};
 PlaybackTrackGroupEditCandidate edit{19,false,"Audio edit"};if(!queuePlaybackTrackGroupEditCandidate(edit))throw std::runtime_error("track-group edit rejected");edit.name="mutated after queue";
 if(!drainPlaybackEditorActionsCandidate(7)||editCalls!=1)throw std::runtime_error("queued track-group edit dispatch failed");
 DestroyWindow(window);std::cout<<"track_group_dispatch=menu_queue_unique_command_id_payload_copy_passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

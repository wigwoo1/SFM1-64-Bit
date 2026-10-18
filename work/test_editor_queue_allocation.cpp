#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <new>
#include "win32_vulkan_playback_candidate.h"
static bool failAllocation=false;
void*operator new(size_t bytes){if(failAllocation)throw std::bad_alloc();if(auto*p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete(void*p,size_t)noexcept{std::free(p);}
int main(){playbackEditorHandler=[](auto,size_t){return true;};playbackScalarEditorHandler=[](const auto&,size_t){return true;};PlaybackScalarEditCandidate edit{123,std::string(200,'x'),1.f};failAllocation=true;bool accepted=queuePlaybackEditorActionCandidate(PlaybackEditorActionCandidate::Undo);failAllocation=false;if(accepted||!playbackEditorQueue.empty())return 1;if(!queuePlaybackEditorActionCandidate(PlaybackEditorActionCandidate::Undo))return 2;failAllocation=true;accepted=queuePlaybackScalarEditCandidate(edit);failAllocation=false;if(accepted||playbackEditorQueue.size()!=1||!std::holds_alternative<PlaybackEditorActionCandidate>(playbackEditorQueue[0]))return 3;if(!queuePlaybackScalarEditCandidate(edit)||playbackEditorQueue.size()!=2)return 4;std::cout<<"editor_queue_allocation_failures=2 history_preserved_retry=passed\n";}

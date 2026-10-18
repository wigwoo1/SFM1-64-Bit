#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_graph_state_lookup_candidate.h"
#include <cassert>
int main(int argc,char** argv){try{static_assert(sizeof(void*)==8);if(argc<2)return 2;for(int i=1;i<argc;++i){auto g=loadGraph(argv[i]);SessionRuntimeHandles handles(g);SessionGraphStateLookupCandidate adapter(g,handles,handles.graphHandles.at(0));auto* state=adapter.lookup();assert(state && g.strings.at(state->type)=="DmeGraphEditorState");bool grid=false;for(auto& a:state->attributes)if(g.strings.at(a.name)=="displayGrid"){assert(a.type==4);grid=true;}assert(grid);auto index=static_cast<unsigned>(state-g.elements.data());assert(handles.resolve(g,handles.graphHandles.at(index))==state);handles.retire(handles.graphHandles.at(index));assert(!adapter.lookup());std::cout<<argv[i]<<" graph state="<<index<<" native handle lookup and retirement passed\n";}return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}

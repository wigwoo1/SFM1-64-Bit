#pragma once
template<class Object,class Adapter>void graphReleaseNestedCandidate(Object** storage,Adapter& a){if(auto* object=*storage)a.invokeSlot0(object,1);*storage=nullptr;}
template<class Adapter>void graphUpdateChildViewsCandidate(Adapter& a){const auto count=a.controllerViewCount();for(int i=0;i<count;++i)if(auto* view=a.liveControllerView(i))a.invokeViewSlot3(view);}

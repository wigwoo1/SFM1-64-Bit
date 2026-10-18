#pragma once
#include <cstdint>
template<class Adapter>std::int32_t graphBookmarkCountCandidate(int component,Adapter& a){if(component>=a.componentCountSlot62())return -1;return a.storedCount90(component);}
template<class Adapter>std::uint32_t graphBookmarkTimeCandidate(int index,int component,Adapter& a){if(component>=a.componentCountSlot62() || index>=a.storedCount90(component))return 0;return a.storedTime84(component,index);}
template<class Vector,class Adapter>void graphReplaceBookmarksCandidate(int component,const Vector& source,Adapter& a){if(component>=a.componentCountSlot62())return;a.assignVector1038cd00(component,a.sourceBase(source),a.sourceCount(source));}

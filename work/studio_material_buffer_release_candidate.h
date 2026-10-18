#pragma once
#include "studio_material_buffer_resize_candidate.h"
// Return codes ignored as in 10073ad0. Valid live extent must be representable.
template<class Free>void studioMaterialBufferReleaseCandidate(StudioMaterialBufferCandidate& buffer,Free free){
 if(!buffer.base)return;
 if(buffer.committedEnd>buffer.base){buffer.dirty=false;free(buffer.base,buffer.committedEnd-buffer.base,0x4000);buffer.committedEnd=buffer.base;buffer.cursor=buffer.base;}
 if(buffer.base){free(buffer.base,0,0x8000);buffer.base=0;}
}

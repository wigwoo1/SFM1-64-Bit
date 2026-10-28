#pragma once
#include <cstdint>
// Routing protocol of stdshader1005A820. Ops owns valid live parameter/info
// access, skin predicate, hardware, shared shader services and complete draw
// implementations. Raw info offsets14C/150 intentionally have no inferred
// material semantics. Other-family declared fields are not automatically legal
// for the installed VertexLitGeneric schema. This adds no renderer admission,
// resource ownership, exception rollback or synchronization.
template<class Ops>inline void genericMaterialDrawDispatchCandidate(
 uintptr_t shader,uintptr_t parameters,uintptr_t api,uintptr_t shadow,
 uint32_t vertexLit,uintptr_t info,uint32_t compression,uintptr_t context,Ops&ops){
 if(ops.wantsSkin(parameters,info)&&ops.hardwareLevel()>=92){ops.skin(shader,parameters,api,shadow,info,compression,context);return;}
 bool flashlight=false;
 if(uint8_t(vertexLit)||(ops.infoIndex(info,0x14c)!=0xffffffffu&&ops.integer(parameters,ops.infoIndex(info,0x14c))!=0)){
  if(uint8_t(ops.flashlight(parameters))){
   flashlight=true;
   const auto alternate=ops.infoIndex(info,0x150);
   if(alternate!=0xffffffffu&&ops.integer(parameters,alternate)!=0){
    if(shadow){ops.generic(shader,parameters,api,shadow,vertexLit,true,0,info,compression,context);ops.generic(shader,parameters,api,shadow,vertexLit,true,1,info,compression,context);return;}
    if(uint8_t(ops.query59(api))){ops.skip(shader,0);ops.generic(shader,parameters,api,0,vertexLit,true,1,info,compression,context);}
    else{ops.generic(shader,parameters,api,0,vertexLit,true,0,info,compression,context);ops.skip(shader,0);}
    return;
   }
  }
 }
 ops.generic(shader,parameters,api,shadow,vertexLit,flashlight,0,info,compression,context);
}

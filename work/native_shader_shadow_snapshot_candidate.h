#pragma once
#include "native_alpha_shadow_state_candidate.h"
#include <array>
#include <cstring>
// Owned reproduction of the audited 256-byte fixture, not a native x64 ABI.
// Only 0..16 stages are currently validated against installed instructions.
using NativeShaderShadowSnapshotCandidate=std::array<uint8_t,256>;
inline void aggregateNativeShaderShadowSnapshotCandidate(NativeShaderShadowSnapshotCandidate&storage,size_t stageCount){
 if(stageCount>16)throw std::runtime_error("unaudited shader texture stage count");
 auto next=storage;
 aggregateNativeTextureStageFlagsCandidate(next.data()+0x58,16,next.data()+0x88,16,stageCount);
 auto read=[&](size_t offset){uint32_t value;std::memcpy(&value,next.data()+offset,4);return value;};
 NativeAlphaShadowStateCandidate alpha;alpha.controls=next[0x6c];alpha.comparison=read(0x20);uint32_t reference=read(0x24);std::memcpy(&alpha.reference,&reference,4);
 auto aggregate=aggregateNativeBlendShadowCandidate(alpha,next[0x6d],read(8),read(12),read(16),read(20),read(24),read(28));
 std::array<uint32_t,8>fields{aggregate.source,aggregate.destination,aggregate.operation,aggregate.separateSource,aggregate.separateDestination,aggregate.separateOperation,aggregate.alpha.comparison,static_cast<uint32_t>(aggregate.alpha.reference)};
 std::memcpy(next.data()+0x38,fields.data(),sizeof(fields));next[0x6d]=aggregate.secondaryControls;storage=next;
}

inline void setNativeShaderTextureEnabledCandidate(NativeShaderShadowSnapshotCandidate&storage,size_t stage,size_t hardwareStageCount,uint8_t value){
 if(hardwareStageCount>16||stage>=hardwareStageCount)throw std::runtime_error("shader texture stage bounds");
 storage[0x88+stage]=static_cast<uint8_t>((storage[0x88+stage]&0xfeu)|(value&1u));
}

inline void setNativeShaderBlendFactorsCandidate(NativeShaderShadowSnapshotCandidate&storage,uint32_t source,uint32_t destination,bool separate){
 constexpr uint32_t mapping[]{1,2,9,10,5,6,7,8,11,3,4};
 if(source>=11||destination>=11)throw std::runtime_error("unsupported native blend factor enum");
 const size_t offset=separate?0x14:8;const uint32_t fields[]{mapping[source],mapping[destination]};std::memcpy(storage.data()+offset,fields,sizeof(fields));
}

inline void setNativeShaderBlendOperationCandidate(NativeShaderShadowSnapshotCandidate&storage,uint32_t operation,bool separate){
 if(operation>4)throw std::runtime_error("unsupported native blend operation enum");
 const uint32_t mapped=operation+1;std::memcpy(storage.data()+(separate?0x1c:0x10),&mapped,4);
}

inline void setNativeShaderBlendEnabledCandidate(NativeShaderShadowSnapshotCandidate&storage,uint8_t value,bool separate){
 auto&flags=storage[separate?0x6d:0x6c];const uint8_t mask=separate?1:0x20;flags=static_cast<uint8_t>((flags&~mask)|((value&1u)*mask));
}
inline void setNativeShaderAlphaCoverageCandidate(NativeShaderShadowSnapshotCandidate&storage,uint8_t value){storage[0x6d]=static_cast<uint8_t>((storage[0x6d]&~8u)|((value&1u)<<3));}
inline void setNativeShaderAlphaEnabledCandidate(NativeShaderShadowSnapshotCandidate&storage,uint8_t value){storage[0x6c]=static_cast<uint8_t>((storage[0x6c]&~0x40u)|((value&1u)<<6));}
inline void setNativeShaderAlphaFunctionCandidate(NativeShaderShadowSnapshotCandidate&storage,uint32_t comparison,float reference){
 NativeAlphaShadowStateCandidate state;if(!setNativeAlphaShadowFunctionCandidate(state,comparison,reference))throw std::runtime_error("unsupported native alpha enum");std::memcpy(storage.data()+0x20,&state.comparison,4);std::memcpy(storage.data()+0x24,&state.reference,4);
}

// Recovered installed slots: 2 depth enable, 3 depth write, 4 packed raster
// control, 6 RGB write, 7 alpha write. Byte arguments have distinct rules.
inline void setNativeShaderWriteControlCandidate(NativeShaderShadowSnapshotCandidate&storage,unsigned slot,uint8_t value){
 if(slot==2){storage[0x6c]=static_cast<uint8_t>((storage[0x6c]&~2u)|((value&1u)<<1));return;}
 if(slot==4){storage[0x6c]=static_cast<uint8_t>((storage[0x6c]&~12u)|((value<<2)&12u));return;}
 if(slot==3){const uint32_t normalized=value!=0;std::memcpy(storage.data()+0x2c,&normalized,4);return;}
 if(slot==6||slot==7){uint32_t flags;std::memcpy(&flags,storage.data()+0x30,4);const uint32_t mask=slot==6?7u:8u;flags=value?flags|mask:flags&~mask;std::memcpy(storage.data()+0x30,&flags,4);return;}
 throw std::runtime_error("unsupported recovered write-control slot");
}
inline void setNativeShaderDepthComparisonCandidate(NativeShaderShadowSnapshotCandidate&storage,uint32_t comparison,bool reverseDepth){
 if(comparison>7)throw std::runtime_error("unsupported native depth comparison enum");
 constexpr uint32_t reversed[]{1,5,3,7,2,6,4,8};const uint32_t mapped=reverseDepth?reversed[comparison]:comparison+1;std::memcpy(storage.data()+0x28,&mapped,4);
}
inline void setNativeShaderRasterMiscCandidate(NativeShaderShadowSnapshotCandidate&storage,unsigned slot,uint32_t first,uint32_t second=0){
 if(slot==14){if(first==1)return;if(second>2)throw std::runtime_error("unsupported native raster mode");const uint32_t mapped=second+1;std::memcpy(storage.data()+0x34,&mapped,4);return;}
 if(slot==15){storage[0x6c]=static_cast<uint8_t>((storage[0x6c]&~16u)|((first&1u)<<4));return;}
 if(slot==22){std::memcpy(storage.data()+0x68,&first,4);storage[0x6c]=static_cast<uint8_t>((storage[0x6c]&~1u)|(second&1u));return;}
 if(slot==23){storage[0x6d]=static_cast<uint8_t>((storage[0x6d]&~4u)|((first&1u)<<2));return;}
 throw std::runtime_error("unsupported recovered raster/misc slot");
}
inline void setNativeShaderHardwareControlCandidate(NativeShaderShadowSnapshotCandidate&storage,unsigned slot,uint32_t stage,uint8_t value,bool supported,size_t stageCount){
 if(slot==19){storage[0x6c]=static_cast<uint8_t>((storage[0x6c]&0x7fu)|(supported?((value&1u)<<7):0u));return;}
 if(slot!=20||stage>=16||(supported&&(stageCount>16||stage>=stageCount)))throw std::runtime_error("hardware control stage bounds");
 storage[0x58+stage]=static_cast<uint8_t>((storage[0x58+stage]&~2u)|(supported?((value&1u)<<1):0u));
}
// Audited state effects of default dispatch except shader selection.
// Shader-manager callbacks and renderer integration remain separate.
inline void initializeNativeShaderRecoveredDefaultsCandidate(NativeShaderShadowSnapshotCandidate&storage,size_t stageCount,bool reverseDepth=false){
 if(stageCount>16)throw std::runtime_error("unaudited default texture stage count");
 auto next=storage;setNativeShaderHardwareControlCandidate(next,19,0,0,false,stageCount);for(size_t stage=0;stage<stageCount;++stage)setNativeShaderHardwareControlCandidate(next,20,static_cast<uint32_t>(stage),0,false,stageCount);setNativeShaderRasterMiscCandidate(next,14,2,2);setNativeShaderRasterMiscCandidate(next,15,1);setNativeShaderRasterMiscCandidate(next,22,0,0);setNativeShaderRasterMiscCandidate(next,23,0);setNativeShaderDepthComparisonCandidate(next,3,reverseDepth);setNativeShaderWriteControlCandidate(next,2,1);setNativeShaderWriteControlCandidate(next,3,1);setNativeShaderWriteControlCandidate(next,4,0);setNativeShaderWriteControlCandidate(next,6,1);setNativeShaderWriteControlCandidate(next,7,0);setNativeShaderAlphaEnabledCandidate(next,0);setNativeShaderBlendEnabledCandidate(next,0,false);setNativeShaderBlendFactorsCandidate(next,0,0,false);setNativeShaderBlendOperationCandidate(next,0,false);setNativeShaderBlendEnabledCandidate(next,0,true);setNativeShaderBlendFactorsCandidate(next,0,0,true);setNativeShaderBlendOperationCandidate(next,0,true);setNativeShaderAlphaFunctionCandidate(next,6,.7f);setNativeShaderAlphaCoverageCandidate(next,0);std::memset(next.data()+0x80,0,8);for(size_t stage=0;stage<stageCount;++stage)setNativeShaderTextureEnabledCandidate(next,stage,stageCount,0);storage=next;
}

// Shader-manager handle assignment; callback owns shader lookup/compilation.
// Diagnostic formatter and manager behavior are not asserted by this helper.
template<class Manager> inline void selectNativeShaderCandidate(NativeShaderShadowSnapshotCandidate&storage,bool pixel,const char*name,uint32_t combo,Manager&&manager){
 const uint32_t handle=manager(pixel,name,combo);
 std::memcpy(storage.data()+(pixel?0x74:0x70),&handle,4);std::memcpy(storage.data()+(pixel?0x7c:0x78),&combo,4);
}

// Complete audited default field reset with externally supplied shader handles.
// This does not initialize a renderer or claim native shader-manager parity.
template<class Manager> inline void initializeNativeShaderDefaultsCandidate(NativeShaderShadowSnapshotCandidate&storage,size_t stageCount,bool reverseDepth,Manager&&manager){
 auto next=storage;initializeNativeShaderRecoveredDefaultsCandidate(next,stageCount,reverseDepth);selectNativeShaderCandidate(next,false,nullptr,0,manager);selectNativeShaderCandidate(next,true,nullptr,0,manager);storage=next;
}

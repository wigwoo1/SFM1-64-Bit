#pragma once
#include "buffer_owned_constructor_candidate.h"
#include "buffer_write_check_candidate.h"
#include "buffer_default_overflow_candidate.h"
#include "buffer_external_conversion_candidate.h"
#include "buffer_storage_growth_candidate.h"
#include "buffer_string_extract_candidate.h"
#include <cstdlib>
#include <new>
#include <cassert>
// Native storage adapter for recovered routines; not a complete CUtlBuffer API.
class NativeBufferCompositionCandidate{
 std::uint8_t* storage_=nullptr;
 std::uint32_t cap_=0,grow_=0,get_=0,put_=0,field_=0,offset_=0,max_=0,state_=0;
 std::uint8_t error_=0,flags_=0;bool read_=false,write_=false;
public:
 NativeBufferCompositionCandidate(std::uint32_t grow,std::uint32_t cap,std::uint32_t flags){constructOwnedBufferCandidate(grow,cap,flags,*this);}
 ~NativeBufferCompositionCandidate(){if(bufferSigned32Candidate(grow_)>=0)std::free(storage_);}
 NativeBufferCompositionCandidate(const NativeBufferCompositionCandidate&)=delete;
 NativeBufferCompositionCandidate& operator=(const NativeBufferCompositionCandidate&)=delete;
 auto sourceData(){return storage_;}void setStorage(std::uint8_t* p){storage_=p;}
 auto capacity(){return cap_;}void setCapacity(std::uint32_t v){cap_=v;}
 auto growSize(){return grow_;}void setGrowSize(std::uint32_t v){grow_=v;}
 auto getPosition(){return get_;}void setGetPosition(std::uint32_t v){get_=v;}
 auto putPosition(){return put_;}void setPutPosition(std::uint32_t v){put_=v;}
 auto bufferOffset(){return offset_;}void setBufferOffset(std::uint32_t v){offset_=v;}
 auto maxGet(){return max_;}void setMaxGet(std::uint32_t v){max_=v;}
 auto errorByte(){return error_;}void setErrorByte(std::uint8_t v){error_=v;}
 auto flagsByte(){return flags_;}void setFlagsByte(std::uint8_t v){flags_=v;}
 auto state2c(){return state_;}void setState2c(std::uint32_t v){state_=v;}
 void setField18(std::uint32_t v){field_=v;}
 std::uint8_t* allocate(std::uint32_t n){auto* p=static_cast<std::uint8_t*>(std::malloc(n));if(!p && n)throw std::bad_alloc();return p;}
 std::uint8_t* reallocate(std::uint8_t* old,std::uint32_t n){auto* p=static_cast<std::uint8_t*>(std::realloc(old,n));if(!p && n)throw std::bad_alloc();return p;}
 auto sourceByte(std::uint32_t i){assert(i<cap_);return storage_[i];}
 void writeByte(std::uint32_t i,std::uint8_t v){assert(i<cap_);storage_[i]=v;}
 void installReadCallback(){read_=true;}void installWriteCallback(){write_=true;}
 bool readOverflow(std::uint32_t n){assert(read_);return defaultBufferReadOverflowCandidate(n);}
 bool writeOverflow(std::uint32_t n){assert(write_);return defaultBufferWriteOverflowCandidate(n,*this);}
 void terminateBuffer(){terminateBufferCandidate(*this);}
 void convertExternalStorage(std::uint32_t n){convertExternalBufferCandidate(n,*this);}
 void growStorage(std::uint32_t n){growBufferStorageCandidate(n,*this);}
};

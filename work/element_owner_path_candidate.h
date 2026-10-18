#pragma once
#include <array>
#include <cstdint>
#include <cstring>
inline bool elementOwnerPathSeparatorCandidate(char byte) {
    return byte=='/' || byte=='\\';
}
//109760d0. Capacity-failure callback must not return in production when the
// physical buffer is too small. Returning is allowed only with sufficient
// physical backing, e.g. an instrumentation fixture declaring smaller capacity.
template<class CapacityFailure>
void appendElementOwnerSlashCandidate(char* path,std::int32_t capacity,
    CapacityFailure capacityFailure) {
    const auto length=std::strlen(path);
    if (!length || elementOwnerPathSeparatorCandidate(path[length-1])) return;
    if (static_cast<std::int32_t>(length+1)>=capacity) capacityFailure(path);
    path[length]='\\';path[length+1]=0;
}
//10975f80 primary cleanup. Nonnull terminated writable buffer; failures retain
// earlier edits and skip the final separator conversion, as in the original.
inline bool cleanupElementOwnerPathCandidate(char* path,char separator) {
    std::size_t read=0,write=0;
    bool previousSeparator=false;
    while (path[read]) {
        const auto byte=path[read++];
        const auto currentSeparator=elementOwnerPathSeparatorCandidate(byte);
        if (!currentSeparator || !previousSeparator) path[write++]=byte;
        previousSeparator=currentSeparator;
    }
    path[write]=0;
    read=write=0;
    while (path[read]) {
        if (path[read]=='.' && elementOwnerPathSeparatorCandidate(path[read+1]) &&
            (read==0 || path[read-1]!='.')) {
            read+=2;
        } else {
            path[write++]=path[read++];
        }
    }
    path[write]=0;
    const auto length=std::strlen(path);
    if (length>2 && path[length-1]=='.' &&
        elementOwnerPathSeparatorCandidate(path[length-2])) path[length-2]=0;
    read=0;
    while (path[read]) {
        if (path[read]=='.' && path[read+1]=='.' &&
            (read==0 || elementOwnerPathSeparatorCandidate(path[read-1])) &&
            (!path[read+2] || elementOwnerPathSeparatorCandidate(path[read+2]))) {
            auto back=static_cast<std::ptrdiff_t>(read)-2;
            while (back>=0 && !elementOwnerPathSeparatorCandidate(path[back])) --back;
            if (back<0) return false;
            const auto suffix=read+2;
            std::memmove(path+back,path+suffix,std::strlen(path+suffix)+1);
            read=0;
        } else {
            ++read;
        }
    }
    for (auto* p=path;*p;++p)
        if (elementOwnerPathSeparatorCandidate(*p)) *p=separator;
    return true;
}
//10975d20 secondary cleanup; preserves leading pair and original index advance.
inline void cleanupElementOwnerDuplicateSeparatorsCandidate(char* path) {
    auto length=std::strlen(path);
    for (std::size_t i=1;i+1<length;++i) {
        if (elementOwnerPathSeparatorCandidate(path[i]) &&
            elementOwnerPathSeparatorCandidate(path[i+1])) {
            std::memmove(path+i,path+i+1,length-i);
            --length;
        }
    }
}
//10a64a90 only when locale-mode global10fdb1c0 is zero; nonnull precondition.
inline char* lowercaseElementOwnerPathAsciiCandidate(char* path) {
    for (auto* p=path;*p;++p)
        if (*p>='A' && *p<='Z') *p=static_cast<char>(*p+0x20);
    return path;
}
//10976270 positive-capacity path, with caller-selected ASCII locale mode.
// Nonnull source/output and nonoverlapping storage are copy preconditions.
inline void normalizeElementOwnerLocalAsciiCandidate(char* output,
    std::int32_t capacity,const char* source) {
    std::size_t i=0;
    const auto size=static_cast<std::size_t>(capacity);
    for (;i<size && source[i];++i) output[i]=source[i];
    for (;i<size;++i) output[i]=0;
    output[size-1]=0;
    for (auto* p=output;*p;++p)
        if (elementOwnerPathSeparatorCandidate(*p)) *p='\\';
    (void)cleanupElementOwnerPathCandidate(output,'\\');
    cleanupElementOwnerDuplicateSeparatorsCandidate(output);
    lowercaseElementOwnerPathAsciiCandidate(output);
}
//10976910 final path, unlike local normalization, reports cleanup failure.
// Callback supplies original diagnostic policy; it may throw/terminate.
template<class RootFailure>
void finishElementOwnerAbsolutePathCandidate(char* output,RootFailure rootFailure) {
    if (!cleanupElementOwnerPathCandidate(output,'\\')) rootFailure();
    for (auto* p=output;*p;++p)
        if (elementOwnerPathSeparatorCandidate(*p)) *p='\\';
}
//10976240; nonnull terminated source is original precondition.
inline bool elementOwnerPathAbsoluteCandidate(const char* path) {
    const auto first=path[0];
    return (first && path[1]==':') || first=='/' || first=='\\';
}
inline void copyElementOwnerPathCandidate(char* output,std::int32_t capacity,
    const char* source) {
    const auto size=static_cast<std::size_t>(capacity);
    std::size_t i=0;
    for (;i<size && source[i];++i) output[i]=source[i];
    for (;i<size;++i) output[i]=0;
    output[size-1]=0;
}
inline void appendElementOwnerPathBoundedCandidate(char* output,
    std::int32_t capacity,const char* source) {
    const auto used=std::strlen(output),length=std::strlen(source);
    const auto space=static_cast<std::ptrdiff_t>(capacity)-
        static_cast<std::ptrdiff_t>(used)-1;
    if (space<=0) return;
    const auto count=length<static_cast<std::size_t>(space)?length:
        static_cast<std::size_t>(space);
    if (!count) return;
    std::memcpy(output+used,source,count);output[used+count]=0;
}
//10976910 positive capacity/nonoverlap and signed-representable lengths only.
// Adapter.currentDirectory implements10a1a0c7; diagnostic implements imported
// policy and must not return after actual buffer exhaustion/CWD failure.
template<class Adapter>
void makeElementOwnerAbsolutePathCandidate(char* output,std::int32_t capacity,
    const char* source,const char* base,Adapter& adapter) {
    const auto failure=[&](const char*) {adapter.diagnostic("capacity");};
    if (elementOwnerPathAbsoluteCandidate(source)) {
        copyElementOwnerPathCandidate(output,capacity,source);
    } else {
        if (base && elementOwnerPathAbsoluteCandidate(base)) {
            copyElementOwnerPathCandidate(output,capacity,base);
        } else {
            if (!adapter.currentDirectory(output,capacity)) adapter.diagnostic("cwd");
            if (base) {
                appendElementOwnerSlashCandidate(output,capacity,failure);
                appendElementOwnerPathBoundedCandidate(output,capacity,base);
            }
        }
        appendElementOwnerSlashCandidate(output,capacity,failure);
        appendElementOwnerPathBoundedCandidate(output,capacity,source);
    }
    finishElementOwnerAbsolutePathCandidate(output,[&] {adapter.diagnostic("root");});
}
//10353980 orchestration; adapter binds bundled helpers/service, not OS defaults.
template<class Adapter>
void normalizeElementOwnerPathCandidate(char* output,std::int32_t capacity,
    const char* source,Adapter& adapter) {
    std::array<char,0x104> normalized;
    adapter.normalizeLocal(normalized.data(),0x104,source);
    if (elementOwnerPathAbsoluteCandidate(normalized.data())) {
        adapter.copy(output,normalized.data(),capacity);
    } else {
        std::array<char,0x104> serviceBuffer;
        adapter.servicePath(serviceBuffer.data(),0x104);
        adapter.relative(output,capacity,normalized.data(),serviceBuffer.data());
    }
}

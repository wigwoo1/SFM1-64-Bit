#pragma once
#include "callback_inline_detach_candidate.h"
#include "element_callback_insert_candidate.h"
#include "owner_symbol_pointer_growth_candidate.h"
#include <array>
// Composes recovered normal paths. Allocator must return valid allocations,
// preserve reallocated contents, and outlive this storage. No thread safety.
template<class Pointer,class Allocator>
class ElementCallbackStorageCandidate {
    std::array<Pointer,16> inlineStorage{};
    Pointer* storage=inlineStorage.data();
    Pointer* cached=inlineStorage.data();
    std::int32_t allocated=16,used=0,growSize=-1;
    Allocator& allocator;
public:
    explicit ElementCallbackStorageCandidate(Allocator& a):allocator(a) {}
    ElementCallbackStorageCandidate(const ElementCallbackStorageCandidate&)=delete;
    ElementCallbackStorageCandidate& operator=(const ElementCallbackStorageCandidate&)=delete;
    ~ElementCallbackStorageCandidate() {if(growSize>=0 && storage)allocator.free(storage);}
    auto count() const {return used;}
    auto capacity() const {return allocated;}
    bool external() const {return growSize<0;}
    int inlineCapacity() const {return 16;}
    void detach(int size) {detachCallbackInlineStorageCandidate(storage,allocated,growSize,size,allocator);}
    void grow(int delta) {growOwnerSymbolPointersCandidate(storage,allocated,growSize,delta,allocator);}
    void setCount(int n) {used=n;}
    Pointer* data() {return storage;}
    Pointer* cachedData() {return cached;}
    void setCached(Pointer* p) {cached=p;}
    int insert(int index,Pointer value) {return insertElementCallbackCandidate(index,&value,*this);}
    int append(Pointer value) {return insert(used,value);}
};

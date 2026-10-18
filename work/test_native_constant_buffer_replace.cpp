#include <initializer_list>
#include <cstddef>
#include "native_constant_buffer_replace_candidate.h"
#include <cassert>
#include <cstdio>
struct Allocator{unsigned frees=0,allocs=0;size_t bytes=0;void* result=nullptr;Allocator** active=nullptr;Allocator* replacement=nullptr;uint32_t* count=nullptr;void Free(void* value){assert(value==reinterpret_cast<void*>(uintptr_t{0x100001234ull}));++frees;if(replacement){*active=replacement;*count=7;}}void* Alloc(size_t value){++allocs;bytes=value;return result;}};
struct Lifecycle{Allocator*& active;uint32_t value;unsigned reads=0;Allocator* allocator(){++reads;return active;}uint32_t count(unsigned id){assert(id==2);return value;}};
int main(){for(bool swap:{false,true})for(bool fail:{false,true}){Allocator first,second;Allocator* active=&first;Lifecycle lifecycle{active,3};first.active=&active;first.count=&lifecycle.value;first.replacement=swap?&second:nullptr;first.result=second.result=fail?nullptr:reinterpret_cast<void*>(uintptr_t{0x200005678ull});void* slot=reinterpret_cast<void*>(uintptr_t{0x100001234ull});NativeConstantBufferServicesCandidateT<Lifecycle> services{lifecycle};studioConstantBufferReplaceCandidate(slot,2,16,services);assert(first.frees==1&&lifecycle.reads==2&&first.allocs==(swap?0u:1u)&&second.allocs==(swap?1u:0u));assert((swap?second.bytes:first.bytes)==(swap?112u:48u));assert(slot==(fail?nullptr:reinterpret_cast<void*>(uintptr_t{0x200005678ull})));}std::puts("constant_allocator_bridge_cases=4 passed");}


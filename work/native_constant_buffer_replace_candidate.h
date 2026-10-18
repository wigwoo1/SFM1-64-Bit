#pragma once
#include <cstddef>
#include "studio_constant_buffer_replace_candidate.h"
// Typed allocator contract, not a fabricated Source vtable layout.
template<class Lifecycle>struct NativeConstantBufferServicesCandidateT{
 Lifecycle& lifecycle;
 template<class Pointer>void free(Pointer value){lifecycle.allocator()->Free(value);}
 uint32_t count(unsigned id){return lifecycle.count(id);}
 void* allocate(uint32_t bytes){return lifecycle.allocator()->Alloc(static_cast<std::size_t>(bytes));}
};


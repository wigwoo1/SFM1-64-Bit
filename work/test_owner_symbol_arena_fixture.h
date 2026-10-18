#pragma once
#include "owner_symbol_arena_fit_candidate.h"
#include "owner_symbol_arena_record_candidate.h"
#include "owner_symbol_pointer_growth_candidate.h"
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <cassert>
// Single-thread fixture: real aligned records and native-width pointer growth;
// host allocations remain substitutes for the unrecovered production allocator.
struct OwnerSymbolArenaFixture {
    using Block=OwnerSymbolArenaBlockCandidate;
    Block** table=nullptr;
    std::int32_t capacity=0,count=0;
    std::vector<std::unique_ptr<Block>> metadata;
    std::vector<std::unique_ptr<char[]>> payloads;
    struct Allocator {
        Block** allocate(std::size_t bytes) {
            auto p=static_cast<Block**>(std::malloc(bytes));assert(p);return p;
        }
        Block** reallocate(Block** p,std::size_t bytes) {
            auto q=static_cast<Block**>(std::realloc(p,bytes));assert(q);return q;
        }
    } allocator;
    OwnerSymbolArenaFixture()=default;
    OwnerSymbolArenaFixture(const OwnerSymbolArenaFixture&)=delete;
    OwnerSymbolArenaFixture& operator=(const OwnerSymbolArenaFixture&)=delete;
    ~OwnerSymbolArenaFixture() {std::free(table);}
    const char* store(std::uint32_t hash,const char* text) {
        const auto length=std::strlen(text);
        if(length>0x7ffffff0u)throw std::overflow_error("fixture string too large");
        const auto bytes=ownerSymbolRecordBytesCandidate(static_cast<std::uint32_t>(length));
        auto index=findOwnerSymbolArenaFitCandidate(table,count,static_cast<std::int32_t>(bytes));
        if(index<0) {
            const auto total=ownerSymbolNewBlockBytesCandidate(bytes);
            auto block=std::make_unique<Block>();block->capacity=total-12;
            auto payload=std::make_unique<char[]>(total-8);
            if(count+1>capacity)growOwnerSymbolPointersCandidate(table,capacity,0,count+1-capacity,allocator);
            index=count++;table[index]=block.get();
            metadata.push_back(std::move(block));payloads.push_back(std::move(payload));
        }
        auto& block=*table[index];
        return writeOwnerSymbolArenaRecordCandidate(payloads[index].get(),block.used,
            bytes,hash,text,static_cast<std::uint32_t>(length+1))+4;
    }
};

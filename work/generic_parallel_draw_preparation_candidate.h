#pragma once
#include "generic_single_draw_preparation_candidate.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <exception>
struct GenericDrawPreparationRequestCandidate {
    GenericAmbientStateCandidate ambient;
    GenericLightArrayCandidate lights;
    std::array<uint32_t,18> baseline{};
    GenericLightingConstantsRecordCandidate previous;
    uint32_t compression=0;
    int32_t bones=0,query=0,configuredCount=0;
    uint8_t staticControlFlow=0;
    bool waterWrite=false,skipUnusedFill=false;
    std::vector<uint32_t> unusedWords;
};
// Immutable request snapshots and independent selector state. Cache decode is
// internally synchronized; GPU calls belong to the render owner, not workers.
inline std::vector<GenericPreparedSingleDrawCandidate> genericParallelDrawPreparationCandidate(
    RetainedVcs6ShaderCacheCandidate& vertex,uint32_t vertexStatic,
    RetainedVcs6ShaderCacheCandidate& pixel,uint32_t pixelStatic,
    const std::vector<GenericDrawPreparationRequestCandidate>& requests,unsigned workers){
    if(workers==0||workers>64)throw std::invalid_argument("draw preparation worker count");
    std::vector<GenericPreparedSingleDrawCandidate> results(requests.size());
    std::atomic<size_t> next{0};std::atomic<bool> failed{false};
    std::exception_ptr failure;std::mutex failureMutex;std::vector<std::thread> threads;
    auto run=[&](){try{
        while(!failed.load()){
            const size_t index=next.fetch_add(1);if(index>=requests.size())break;
            const auto& r=requests[index];NativeShaderProgramSelectionCandidate selection(vertex,vertexStatic,pixel,pixelStatic);
            results[index]=genericSingleDrawPreparationCandidate(selection,r.ambient,r.lights,r.baseline,r.compression,r.bones,r.staticControlFlow,r.query,r.waterWrite,r.configuredCount,r.skipUnusedFill,r.unusedWords,r.previous);
        }
    }catch(...){std::lock_guard<std::mutex> lock(failureMutex);if(!failure)failure=std::current_exception();failed.store(true);}};
    try{for(unsigned n=0;n<(std::min)(size_t(workers),requests.size());++n)threads.emplace_back(run);}
    catch(...){failed.store(true);for(auto& thread:threads)thread.join();throw;}
    for(auto& thread:threads)thread.join();if(failure)std::rethrow_exception(failure);
    return results;
}

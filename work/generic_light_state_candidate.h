#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <utility>
// Owned values, not a native x86/x64 record ABI. Baseline is explicit because
// the original runtime may populate its static comparison block.
struct GenericLightRecordCandidate {
    bool present=false;
    std::array<uint32_t,18> ambientWords{};
    int32_t lightCount=0;
    uint32_t secondFlagWord=0;
};
struct GenericLightStateCandidate {
    int32_t lightCount=0;
    bool firstFlag=false,secondFlag=false;
};
inline GenericLightStateCandidate genericAggregateLightStateCandidate(
    const std::vector<GenericLightRecordCandidate>& records,
    const std::array<uint32_t,18>& baseline) {
    GenericLightStateCandidate result;
    for(const auto& record:records)if(record.present){
        result.firstFlag=result.firstFlag || record.ambientWords!=baseline;
        result.secondFlag=result.secondFlag || record.secondFlagWord!=0;
        result.lightCount=(std::max)(result.lightCount,record.lightCount);
    }
    return result;
}
// Single draw owner. Replace inputs at draw boundaries; an explicit invalidation
// also supports refresh without changing stored values.
class RetainedGenericLightStateCandidate {
    std::vector<GenericLightRecordCandidate> records_;
    std::array<uint32_t,18> baseline_{};
    GenericLightStateCandidate state_;
    bool valid_=false;
    size_t aggregations_=0;
public:
    void replace(std::vector<GenericLightRecordCandidate> records,
                 std::array<uint32_t,18> baseline) {
        records_=std::move(records);baseline_=baseline;valid_=false;
    }
    void invalidate() noexcept {valid_=false;}
    GenericLightStateCandidate query() {
        if(!valid_){state_=genericAggregateLightStateCandidate(records_,baseline_);
            valid_=true;++aggregations_;}
        return state_;
    }
    size_t aggregations() const noexcept {return aggregations_;}
};

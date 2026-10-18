#pragma once
#include <array>
#include "reference_record_transfer.h"
struct RecoveredPendingReferenceEntryCandidate{
 std::array<uint32_t,4> identity{};
 RecoveredTransferReferenceRecordCandidate record{};
};
//10353550: field-wise construction, preserving destination reserved bits.
inline void constructRecoveredPendingReferenceEntryCandidate(RecoveredPendingReferenceEntryCandidate&destination,const RecoveredPendingReferenceEntryCandidate&source){
 transferRecoveredReferenceRecordCandidate(destination.record,source.record);
 destination.identity=source.identity;
}

//103534E0 constructs from separate identity and live record (removal path).
inline void constructRecoveredPendingRemovalEntryCandidate(RecoveredPendingReferenceEntryCandidate& destination,const std::array<uint32_t,4>& identity,const RecoveredTransferReferenceRecordCandidate& record){
 transferRecoveredReferenceRecordCandidate(destination.record,record);destination.identity=identity;
}

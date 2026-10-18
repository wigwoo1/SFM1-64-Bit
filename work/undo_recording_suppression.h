#pragma once
#include <cstdint>
// Undo subsystem +74 is datamodel +C18C (embedded at +C118).
// Native reverse replay 10390D50 saves bit 3, clears it, restores only that bit.
struct RuntimeUndoRecordingSuppression{
 uint8_t&flags;uint8_t saved;
 explicit RuntimeUndoRecordingSuppression(uint8_t&value):flags(value),saved(static_cast<uint8_t>(value&8)){flags=static_cast<uint8_t>(flags&0xf7);}
 ~RuntimeUndoRecordingSuppression(){flags=static_cast<uint8_t>((flags&0xf7)|saved);}
 RuntimeUndoRecordingSuppression(const RuntimeUndoRecordingSuppression&)=delete;
 RuntimeUndoRecordingSuppression&operator=(const RuntimeUndoRecordingSuppression&)=delete;
};

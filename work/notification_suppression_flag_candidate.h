#pragma once
#include <cstdint>
// Concrete CDataModel slot119 reaches nested field bit4; keep storage native.
inline bool notificationSuppressionFlagCandidate(std::uint8_t field) {
    return (field&0x10u)!=0;
}
inline void setNotificationSuppressionFlagCandidate(std::uint8_t& field,std::uint8_t incoming) {
    field=static_cast<std::uint8_t>((field&0xefu)|((incoming&1u)<<4));
}

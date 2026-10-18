#pragma once
#include <cstdint>
// Names verified through installed GetMode/Play/Record bindings.
// This chooses an operation; it does not implement playback or recording.
enum class ChannelOperation {None,Copy,Record,Play};
inline ChannelOperation channelOperation(int32_t mode){
 switch(mode){case 1:return ChannelOperation::Copy;case 2:return ChannelOperation::Record;case 3:return ChannelOperation::Play;default:return ChannelOperation::None;}
}

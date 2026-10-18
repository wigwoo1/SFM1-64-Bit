#pragma once
enum class ChannelPlayPath {NoWrite,CopySource,SampleLog};
// Recovered 0x104CE1A0 after destination/log resolution.
inline ChannelPlayPath channelPlayPath(bool destination,bool log,bool force,bool empty,bool useDefault,bool source){
 if(!destination||!log)return ChannelPlayPath::NoWrite;
 if(!force&&empty&&!useDefault&&source)return ChannelPlayPath::CopySource;
 return ChannelPlayPath::SampleLog;
}

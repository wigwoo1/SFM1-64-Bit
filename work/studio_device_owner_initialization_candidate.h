#pragma once
#include <cstdint>
#include <initializer_list>
// Identifiers name recovered fields; their values are NOT physical offsets.
// A real owner binding must classify/widen pointer fields independently.
enum class StudioDeviceOwnerFieldCandidate{Slot2c4,Slot2c8,Slot2c,Slotb4,Slotb8,EventQuerySupport,Slot2a0,Slotc4,Slota8,Slotb0,Slot178,Slot17c,Slot180,Slot184,Slot188,Slot2a8,Slot2cc};
template<class Backend>void studioDeviceOwnerInitializationCandidate(Backend& backend){
 using Field=StudioDeviceOwnerFieldCandidate;
 backend.base();backend.installDerivedIdentity();
 for(unsigned i=1;i<=6;++i)backend.descriptorWord(i,0);
 backend.descriptorWord(1,1);backend.clearDescriptor();backend.descriptorWord(0,1);backend.descriptorWord(1,1);
 for(auto field:{Field::Slot2c4,Field::Slot2c8,Field::Slot2c})backend.set(field,0);
 backend.set(Field::Slotb4,0);backend.clearPairByte(0);backend.set(Field::Slotb8,0);backend.clearPairByte(1);
 backend.keepUpperFlags();backend.set(Field::EventQuerySupport,-1);backend.set(Field::Slot2a0,-1);
 for(auto field:{Field::Slotc4,Field::Slota8,Field::Slotb0,Field::Slot178,Field::Slot17c,Field::Slot180,Field::Slot184,Field::Slot188,Field::Slot2a8,Field::Slot2cc})backend.set(field,0);
 backend.clearFinalByte();
}

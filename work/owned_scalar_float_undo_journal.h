#pragma once
#include "runtime_undo_journal.h"
#include "undo_recording_gate.h"
#include "scalar_float_undo_replay.h"
using OwnedScalarFloatUndoJournalCandidate=RuntimeUndoJournal<RecoveredScalarFloatUndoPayloadCandidate>;
// Explicit native file ID adapter; never infer file ownership from serialized DMX.
template<class Projection>bool editOwnedScalarFloatWithUndoCandidate(Projection&projection,OwnedScalarFloatUndoJournalCandidate&journal,uint32_t element,const std::string&name,float value,int phase,uint32_t fileId){
 const bool ownerPresent=element<projection.elementHandles.size()&&projection.scene.resolve(projection.elementHandles[element]);
 const bool recording=recoveredUndoRecordingGate(journal.flags,ownerPresent,fileId);
 if(recording){if(journal.undo.size()==journal.undo.max_size())throw std::runtime_error("scalar undo capacity overflow");if(journal.undo.size()==journal.undo.capacity()){const auto capacity=journal.undo.capacity();const auto grown=capacity>journal.undo.max_size()/2?journal.undo.max_size():capacity*2;journal.undo.reserve(grown>journal.undo.size()?grown:journal.undo.size()+1);}}
 RecoveredScalarFloatUndoPayloadCandidate record;
 if(!projection.setScalarFloat(element,name,value,phase,recording?&record:nullptr))return false;
 if(recording)journal.append(record);return true;
}
template<class Projection>size_t replayOwnedScalarFloatJournalCandidate(Projection&projection,OwnedScalarFloatUndoJournalCandidate&journal,bool reverse,int phase){
 auto replay=[&](const auto&record,bool backwards){projection.replayScalarFloatUndo(record,backwards,phase);};
 return reverse?journal.reverse(replay):journal.forward(replay);
}

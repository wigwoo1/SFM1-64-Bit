#pragma once
template<class Adapter>
void modelUndoActionCandidate(Adapter& a) {
    if(!a.undoAvailable())return;
    a.performUndo();a.invokeLiveModelSlot126();
}
template<class Adapter>
void modelRedoActionCandidate(Adapter& a) {
    if(!a.redoAvailable())return;
    a.performRedo();a.invokeLiveModelSlot126();
}

#pragma once
#include "retained_obj_scene_candidate.h"
inline std::shared_ptr<const RetainedObjSceneCandidate> objLaunchSceneCandidate;
inline std::shared_ptr<const RetainedObjSceneCandidate> objSavedSessionOwnerCandidate;

// Immutable import snapshots; history edits run at the completed-frame boundary.
inline std::vector<std::shared_ptr<const RetainedObjSceneCandidate>> objUndoHistoryCandidate,objRedoHistoryCandidate;
inline void commitObjEditorCandidate(std::shared_ptr<const RetainedObjSceneCandidate> next){auto current=std::atomic_load(&objLaunchSceneCandidate);objUndoHistoryCandidate.push_back(current);objRedoHistoryCandidate.clear();std::atomic_store(&objLaunchSceneCandidate,std::move(next));}
inline bool undoObjEditorCandidate(){if(objUndoHistoryCandidate.empty())return false;auto current=std::atomic_load(&objLaunchSceneCandidate);objRedoHistoryCandidate.push_back(current);auto next=objUndoHistoryCandidate.back();objUndoHistoryCandidate.pop_back();std::atomic_store(&objLaunchSceneCandidate,std::move(next));return true;}
inline bool redoObjEditorCandidate(){if(objRedoHistoryCandidate.empty())return false;auto current=std::atomic_load(&objLaunchSceneCandidate);objUndoHistoryCandidate.push_back(current);auto next=objRedoHistoryCandidate.back();objRedoHistoryCandidate.pop_back();std::atomic_store(&objLaunchSceneCandidate,std::move(next));return true;}

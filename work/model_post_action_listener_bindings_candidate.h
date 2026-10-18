#pragma once
#include "undo_listener_registration_candidate.h"
#include "notification_listener_removal_candidate.h"
#include "undo_post_action_listeners_candidate.h"
// Native member access replaces original this+c118 adjustment.
template<class Listener,class Model>
bool modelRegisterPostActionListenerCandidate(Listener* listener,Model& model){return registerUndoListenerCandidate(listener,model.undoStack());}
template<class Listener,class Model>
bool modelRemovePostActionListenerCandidate(Listener* listener,Model& model){return removeNotificationListenerCandidate(listener,model.undoStack());}
template<class Model>
void modelDispatchPostActionListenersCandidate(Model& model){notifyUndoPostActionListenersCandidate(model.undoStack());}

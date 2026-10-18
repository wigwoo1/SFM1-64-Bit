from pathlib import Path
p=Path('work/test_post_action_listener_lifecycle.cpp').read_text()
p='#include "model_post_action_listener_bindings_candidate.h"\n'+p
p=p.replace('int main(){Adapter a;Listener','struct Model { Adapter stack; Adapter& undoStack(){return stack;} };\nint main(){Model model;Adapter& a=model.stack;Listener')
p=p.replace('registerUndoListenerCandidate(&x,a)','modelRegisterPostActionListenerCandidate(&x,model)').replace('registerUndoListenerCandidate(&y,a)','modelRegisterPostActionListenerCandidate(&y,model)').replace('registerUndoListenerCandidate(&z,a)','modelRegisterPostActionListenerCandidate(&z,model)').replace('removeNotificationListenerCandidate(&y,a)','modelRemovePostActionListenerCandidate(&y,model)').replace('notifyUndoPostActionListenersCandidate(a)','modelDispatchPostActionListenersCandidate(model)')
Path('work/test_model_post_action_listener_bindings.cpp').write_text(p)

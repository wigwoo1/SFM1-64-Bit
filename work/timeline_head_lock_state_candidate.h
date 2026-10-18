#pragma once
// Recovered state transitions; settings and UI adapters are supplied by the application.
struct TimelineHeadLockStateCandidate {
 bool locked,overwrite;
 explicit TimelineHeadLockStateCandidate(bool savedLock,bool initialOverwrite):locked(savedLock),overwrite(initialOverwrite){}
 static constexpr bool missingSavedLockDefault=true;
 bool buttonChecked()const{return locked;}
 bool buttonEnabled()const{return !overwrite;}
 template<class Persist,class Notify>void setLocked(bool value,Persist persist,Notify notify){if(locked==value)return;locked=value;persist(value);notify();}
 template<class Notify>void setOverwrite(bool value,Notify notify){if(overwrite==value)return;overwrite=value;notify();}
};

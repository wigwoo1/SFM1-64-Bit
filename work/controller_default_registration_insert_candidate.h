#pragma once
template<class Record,class Adapter>auto defaultControllerRegistrationInsertCandidate(Record* record,Adapter& a){
 if(!a.registrationService()){
  a.guardService(9,1,215);
  if(!a.registrationService())a.installDefaultRegistrationService();
  a.guardService(10,1,218);
 }
 auto* container=a.getServiceContainer(a.registrationService(),1);
 if(!container)return a.nullRecord();
 a.guardService(9,1,315);
 auto* result=a.insertContainer(container,record);
 a.guardService(10,1,317);
 return result;
}

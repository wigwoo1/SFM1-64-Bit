#include "reference_owned_scene.h"
#include <iostream>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");RecoveredOwnedSceneCandidate scene;RecoveredPendingIdentityCandidate identity{};identity[0]=1;auto&owner=scene.create(identity);std::vector<unsigned>events;uint32_t expectedToken=0;
 OwnedAttributeAttachmentListenerCandidate first,second,added;
 added=[&](auto&a){if(scene.attributes.resolve(a.token)!=&a)throw std::runtime_error("callback attribute unresolved");events.push_back(3);};
 first=[&](auto&a){if(expectedToken&&expectedToken!=a.token)throw std::runtime_error("snapshot attribute changed");expectedToken=a.token;events.push_back(1);scene.removeAttachmentListener(owner,second);scene.addAttachmentListener(owner,added);};second=[&](auto&a){if(a.token!=expectedToken)throw std::runtime_error("second callback wrong attribute");events.push_back(2);};
 scene.addAttachmentListener(owner,second);scene.addAttachmentListener(owner,first);auto&a=scene.attach(owner,1,{0xffffffffu});if(events!=std::vector<unsigned>{1,2}||expectedToken!=a.token)throw std::runtime_error("owned attachment snapshot mismatch");
 events.clear();expectedToken=0;scene.attach(owner,1,{0xffffffffu});if(events!=std::vector<unsigned>{3,1})throw std::runtime_error("next attachment listener order");
 events.clear();scene.suppressAttachmentListeners=true;scene.attach(owner,1,{0xffffffffu});if(!events.empty())throw std::runtime_error("owned attachment suppression");
 RecoveredOwnedSceneElementCandidate foreign;bool rejected=false;try{scene.addAttachmentListener(foreign,added);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("foreign listener owner accepted");std::cout<<"owned_attachment_listener_checks=4\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

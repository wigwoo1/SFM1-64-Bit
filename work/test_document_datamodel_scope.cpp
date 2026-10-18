#include "document_datamodel_scope_candidate.h"
#include <iostream>
#include <stdexcept>
#include <vector>
static void require(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 unsigned checks=0;
 for(unsigned initial=0;initial<256;++initial)for(unsigned mutation=0;mutation<256;++mutation){
  uint8_t flags=uint8_t(initial);std::vector<unsigned>events;
  documentDataModelScopeCandidate(flags,[&]{
   require(!documentDataModelFlagCandidate(flags),"outer flag not cleared");events.push_back(1);
   flags=uint8_t(mutation);
   documentDataModelScopeCandidate(flags,[&]{
    require(!documentDataModelFlagCandidate(flags),"nested flag not cleared");events.push_back(2);
    flags=uint8_t((flags&8u)|((mutation^0xa5u)&0xf7u));
   });
   require(documentDataModelFlagCandidate(flags)==((mutation&8)!=0),"nested restoration");events.push_back(3);
  });
  require(documentDataModelFlagCandidate(flags)==((initial&8)!=0),"outer restoration");
  require((flags&0xf7u)==((mutation^0xa5u)&0xf7u),"unrelated flag mutations lost");
  require(events==std::vector<unsigned>({1,2,3}),"callback order");++checks;
 }
 unsigned mixedChecks=0;
 for(unsigned initial=0;initial<256;++initial)for(unsigned argument=0;argument<256;++argument){
  uint8_t flags=uint8_t(initial);
  documentDataModelScopeCandidate(flags,[&]{
   require(documentChangePropagationSuppressedCandidate(flags)==((initial&16u)!=0),"scope changed propagation gate");
   setDocumentChangePropagationSuppressedCandidate(flags,0xabcdef00u|argument);
   require(!documentDataModelFlagCandidate(flags),"propagation setter changed scope bit");
   require(documentChangePropagationSuppressedCandidate(flags)==((argument&1u)!=0),"propagation argument normalization");
  });
  require(documentDataModelFlagCandidate(flags)==((initial&8u)!=0),"mixed scope restoration");
  require(documentChangePropagationSuppressedCandidate(flags)==((argument&1u)!=0),"scope lost propagation mutation");
  require((flags&0xe7u)==(initial&0xe7u),"mixed scope changed other flags");++mixedChecks;
 }
 std::cout<<"nested_document_scope_sequences="<<checks<<" mixed_scope_propagation_sequences="<<mixedChecks<<" unrelated_flag_mutations=preserved callback_order=passed normal_return_only=true\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

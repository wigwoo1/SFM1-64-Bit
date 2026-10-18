#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "projected_light_saved_fields_candidate.h"
#include "quaternion_position_matrix_candidate.h"
#include "projected_light_local_matrix_candidate.h"
#include "split_frame_matrix_candidate.h"
#include "angle_basis_products_candidate.h"
#include "projected_light_jitter_displacement_candidate.h"
int main(){try{
 std::ofstream matrixOutput("outputs/sfm64-session-light-local-matrices.txt");if(!matrixOutput)throw std::runtime_error("matrix artifact open");
 const auto basis=angleBasisProductsCandidate({{0,0x3f800000u,0,0x3f800000u,0,0x3f800000u}});
 if(basis[0]!=0x3f800000u||basis[2]!=0x80000000u||basis[4]!=0xbf800000u||basis[5]!=0x80000000u||basis[8]!=0x3f800000u)throw std::logic_error("basis signs mismatch");
 std::cout<<"owned_basis_known_cases=1\n";
 const std::array<uint32_t,3> jitterOrigin{{0x40800000u,0x40a00000u,0x40c00000u}};
 const std::array<uint32_t,6> rightUp{{basis[3],basis[4],basis[5],basis[6],basis[7],basis[8]}};
 const auto displaced=projectedLightJitterDisplacementCandidate(jitterOrigin,rightUp,{{0x3f800000u,0x40000000u}},0x40400000u);
 if(displaced!=std::array<uint32_t,3>{{0x40800000u,0x40000000u,0x41400000u}})throw std::logic_error("jitter displacement mismatch");
 if(projectedLightJitterDisplacementCandidate(jitterOrigin,rightUp,{{0,0x80000000u}},0x7f801234u)!=jitterOrigin)throw std::logic_error("zero jitter must skip radius");
 std::cout<<"owned_jitter_displacement_known_cases=2\n";
 const std::array<uint32_t,4> identityQuaternion{{0,0,0,0x3f800000u}};
 const std::array<uint32_t,3> position{{0x80000000u,0x7f801234u,0x40400000u}};
 const auto matrix=quaternionPositionMatrixCandidate(identityQuaternion,position);
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(matrix[i*4+j]!=(i==j?0x3f800000u:0))throw std::logic_error("identity matrix mismatch");
 if(matrix[3]!=0x80000000u||matrix[7]!=0x7fc01234u||matrix[11]!=0x40400000u)throw std::logic_error("translation bits mismatch");
 const std::array<uint32_t,4> halfTurn{{0,0,0x3f800000u,0}};
 ProjectedLightSavedFieldsCandidate local{};local.localOrientationWords=identityQuaternion;local.localPositionWords=position;
 for(uint32_t word:{0x3f800000u,0x40000000u,0xbf800000u,0x00000000u,0x80000000u,0x7fc01234u,0x7f801234u}){
  local.hasLocalScale=true;local.localScaleWord=word;const auto scaled=projectedLightLocalMatrixCandidate(local);
  const bool nan=(word&0x7f800000u)==0x7f800000u&&(word&0x007fffffu)!=0;
  for(unsigned row=0;row<3;++row){if(scaled[row*4+3]!=matrix[row*4+3])throw std::logic_error("local scale translation mismatch");if(scaled[row*4+row]!=(nan?(word|0x00400000u):word))throw std::logic_error("local scale diagonal mismatch");}
 }
 std::cout<<"owned_local_scale_cases=7\n";
 const auto rotated=quaternionPositionMatrixCandidate(halfTurn,{{0,0,0}});
 if(rotated[0]!=0xbf800000u||rotated[5]!=0xbf800000u||rotated[10]!=0x3f800000u)throw std::logic_error("half-turn matrix mismatch");
 const auto frame=quaternionPositionMatrixCandidate(identityQuaternion,{{0x3f800000u,0x40000000u,0x40400000u}});
 const auto localTest=quaternionPositionMatrixCandidate(identityQuaternion,{{0x40800000u,0x40a00000u,0x40c00000u}});
 const auto same=splitFrameMatrixCandidate(localTest,frame,frame,true);
 const auto distinct=splitFrameMatrixCandidate(localTest,frame,rotated,false);
 for(unsigned row=0;row<3;++row)if(same[row*4+3]!=distinct[row*4+3])throw std::logic_error("split frame translation mismatch");
 if(distinct[0]!=0xbf800000u||distinct[5]!=0xbf800000u)throw std::logic_error("split frame rotation mismatch");
 std::cout<<"owned_matrix_known_cases=2 original_execution_parity=unverified\n";

 for(const char* path:{"work/roundtrip-11.dmx","work/roundtrip-139.dmx"}){
  auto graph=loadGraph(path);size_t lights=0;
  for(size_t index=0;index<graph.elements.size();++index){const auto& element=graph.elements[index];if(graph.strings.at(element.type)=="DmeProjectedLight"){
   unsigned childrenOwners=0;
   for(size_t owner=0;owner<graph.elements.size();++owner)for(const auto& a:graph.elements[owner].attributes)if(graph.strings.at(a.name)=="children"&&a.type==15){
    if(std::any_of(a.values.begin(),a.values.end(),[&](const auto& v){return v.reference==index;})){
     ++childrenOwners;std::cout<<"light_parent_candidate light="<<index<<" owner="<<owner<<" owner_type="<<graph.strings.at(graph.elements[owner].type)<<'\n';
    }
   }
   std::cout<<"light_children_reference_owners="<<childrenOwners<<'\n';
   const auto snapshot=projectedLightSavedFieldsCandidate(graph,index);
   const auto localMatrix=projectedLightLocalMatrixCandidate(snapshot);
   matrixOutput<<path<<" element="<<index<<" name="<<graph.strings.at(element.name)<<" matrix_words=";
   for(auto word:localMatrix)matrixOutput<<std::hex<<word<<',';matrixOutput<<std::dec<<'\n';
   const auto baseMatrix=quaternionPositionMatrixCandidate(snapshot.localOrientationWords,snapshot.localPositionWords);
   for(unsigned row=0;row<3;++row)if(localMatrix[row*4+3]!=baseMatrix[row*4+3])throw std::logic_error("scale changed translation");
   if(!snapshot.hasLocalScale&&localMatrix!=baseMatrix)throw std::logic_error("absent scale changed matrix");
   const auto& names=projectedLightSavedFloatNamesCandidate;
   for(unsigned field=0;field<names.size();++field)for(const auto& attribute:element.attributes)if(graph.strings.at(attribute.name)==names[field]&&snapshot.floatWords[field]!=wireU32(attribute.values[0]))throw std::logic_error("projected field bits changed");
   const char* typedNames[]{"color","animationTime","numPlanes","positionJitter","castsShadows","drawShadowFrustum","uberlight","volumetric"};
   for(unsigned typed=0;typed<8;++typed){
    const auto attribute=std::find_if(element.attributes.begin(),element.attributes.end(),[&](const auto& a){return graph.strings.at(a.name)==typedNames[typed];});
    if(attribute==element.attributes.end())throw std::logic_error("typed fixture field absent");
    const auto& value=attribute->values.at(0);
    if(typed<3){const uint32_t actual[]{snapshot.colorWord,snapshot.animationTimeWord,snapshot.numPlanesWord};if(actual[typed]!=wireU32(value))throw std::logic_error("typed word changed");}
    else if(typed==3){if(std::memcmp(snapshot.positionJitterWords.data(),value.wire.data(),8))throw std::logic_error("jitter changed");}
    else if(snapshot.booleanBytes[typed-4]!=static_cast<uint8_t>(value.wire[0]))throw std::logic_error("boolean changed");
    if(lights==0)for(unsigned variant=0;variant<3;++variant){auto malformed=graph;auto& attributes=malformed.elements[index].attributes;
     auto field=std::find_if(attributes.begin(),attributes.end(),[&](const auto& a){return malformed.strings.at(a.name)==typedNames[typed];});
     if(variant==0){auto copy=*field;attributes.push_back(copy);}else if(variant==1)field->type=3;else field->values[0].wire.pop_back();
     bool rejected=false;try{projectedLightSavedFieldsCandidate(malformed,index);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::logic_error("malformed typed field accepted");
    }
   }
   const auto& snapshotTransform=graph.elements.at(snapshot.transformReference);
   if(snapshot.transformId!=snapshotTransform.id)throw std::logic_error("transform identity changed");
   for(const auto& a:snapshotTransform.attributes){const auto& name=graph.strings.at(a.name);
    if(name=="position"&&std::memcmp(snapshot.localPositionWords.data(),a.values.at(0).wire.data(),12))throw std::logic_error("position changed");
    if(name=="orientation"&&std::memcmp(snapshot.localOrientationWords.data(),a.values.at(0).wire.data(),16))throw std::logic_error("orientation changed");
   }
   for(const auto& a:element.attributes)if(graph.strings.at(a.name)=="texture"&&snapshot.textureName!=a.values.at(0).text)throw std::logic_error("texture changed");
   if(lights==0)for(unsigned variant=0;variant<6;++variant){auto malformed=graph;
    auto& attributes=malformed.elements[index].attributes;
    auto ref=std::find_if(attributes.begin(),attributes.end(),[&](const auto& a){return malformed.strings.at(a.name)=="transform";});
    if(variant==0)ref->values[0].reference=0xffffffff;
    else if(variant==1)malformed.elements[snapshot.transformReference].type=element.type;
    else if(variant==2||variant==3){auto& fields=malformed.elements[snapshot.transformReference].attributes;auto field=std::find_if(fields.begin(),fields.end(),[&](const auto& a){return malformed.strings.at(a.name)=="orientation";});if(variant==2)field->values[0].wire.pop_back();else{auto copy=*field;fields.push_back(copy);}}
    else{auto texture=std::find_if(attributes.begin(),attributes.end(),[&](const auto& a){return malformed.strings.at(a.name)=="texture";});if(variant==4)texture->values[0].stringIndex=0xffffffff;else texture->values[0].text+="invalid";}
    bool rejected=false;try{projectedLightSavedFieldsCandidate(malformed,index);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::logic_error("invalid texture or transform accepted");
   }
   if(lights==0){
    for(uint32_t word:{0x00000000u,0x80000000u,0x3f800000u,0x40000000u,0xbf800000u,0x7f800000u,0x7fc01234u,0x7f801234u}){
     auto scaled=graph;auto& fields=scaled.elements[snapshot.transformReference].attributes;
     fields.erase(std::remove_if(fields.begin(),fields.end(),[&](const auto& a){return scaled.strings.at(a.name)=="scale";}),fields.end());
     Attribute scale{};scale.name=internString(scaled,"scale");scale.type=3;Value value{};value.wire.resize(4);std::memcpy(value.wire.data(),&word,4);scale.values.push_back(value);fields.push_back(scale);
     const auto result=projectedLightSavedFieldsCandidate(scaled,index);if(!result.hasLocalScale||result.localScaleWord!=word)throw std::logic_error("scale bits changed");
     for(unsigned variant=0;variant<3;++variant){auto malformed=scaled;auto& attrs=malformed.elements[snapshot.transformReference].attributes;auto& field=attrs.back();if(variant==0){auto copy=field;attrs.push_back(copy);}else if(variant==1)field.type=2;else field.values[0].wire.pop_back();
      bool rejected=false;try{projectedLightSavedFieldsCandidate(malformed,index);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::logic_error("malformed scale accepted");
     }
    }
    auto absent=graph;auto& fields=absent.elements[snapshot.transformReference].attributes;fields.erase(std::remove_if(fields.begin(),fields.end(),[&](const auto& a){return absent.strings.at(a.name)=="scale";}),fields.end());const auto result=projectedLightSavedFieldsCandidate(absent,index);if(result.hasLocalScale||result.localScaleWord!=0x3f800000u)throw std::logic_error("absent scale policy");
   }
   if(lights==0){
    for(unsigned variant=0;variant<3;++variant){auto malformed=graph;auto& attributes=malformed.elements[index].attributes;
     auto field=std::find_if(attributes.begin(),attributes.end(),[&](const auto& a){return malformed.strings.at(a.name)=="horizontalFOV";});
     if(field==attributes.end())throw std::logic_error("fixture FOV absent");
     if(variant==0){auto copy=*field;attributes.push_back(copy);}else if(variant==1)field->type=2;else field->values[0].wire.pop_back();
     try{projectedLightSavedFieldsCandidate(malformed,index);throw std::logic_error("invalid projected field accepted");}catch(const std::invalid_argument&){}
    }
   }
   if(lights==0)for(const auto& attribute:element.attributes)if(graph.strings.at(attribute.name)=="transform"){
    const auto ref=attribute.values.at(0).reference;const auto& transform=graph.elements.at(ref);
    std::cout<<"transform_type="<<graph.strings.at(transform.type)<<'\n';
    for(const auto& a:transform.attributes)std::cout<<"transform_field="<<graph.strings.at(a.name)<<" type="<<unsigned(a.type)<<" bytes="<<(a.values.empty()?0:a.values[0].wire.size())<<'\n';
   }
   if(lights++==0){std::cout<<"fixture="<<path<<" light="<<graph.strings.at(element.name)<<'\n';
    for(const auto& attribute:element.attributes){std::cout<<"field="<<graph.strings.at(attribute.name)<<" type="<<unsigned(attribute.type)<<" values="<<attribute.values.size();
     if(!attribute.values.empty()){const auto& value=attribute.values.front();std::cout<<" wire_bytes="<<value.wire.size()<<" ref="<<value.reference;
      if(value.wire.size()==4)std::cout<<" word="<<wireU32(value);
     }std::cout<<'\n';}
   }
  }}
  std::cout<<"projected_lights="<<lights<<'\n';
  std::cout<<"projected_light_saved_snapshots="<<lights<<" float_fields_per_snapshot=31 typed_fields_per_snapshot=8 local_matrix_snapshots=1 texture_transform_snapshots=1 malformed_field_cases_rejected=57 scale_bit_cases=8 absent_scale_cases=1\n";
 }
 matrixOutput.flush();if(!matrixOutput)throw std::runtime_error("matrix artifact write");
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

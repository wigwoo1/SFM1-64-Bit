#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <set>
#include "projected_light_saved_fields_candidate.h"
#include "saved_dag_world_matrix_candidate.h"
#include "matrix_orientation_candidate.h"
#include "projected_light_jitter_pose_candidate.h"
#include "projected_light_prepared_fields_candidate.h"
int main(){try{
 std::ofstream preparedOutput("outputs/sfm64-session-light-prepared-fields.txt");if(!preparedOutput)throw std::runtime_error("prepared light artifact open");
 std::ofstream jitterPoses("outputs/sfm64-session-light-jitter-poses.txt");if(!jitterPoses)throw std::runtime_error("jitter pose artifact open");
 ProjectedLightSavedFieldsCandidate synthetic{};synthetic.positionJitterWords={{0x3f800000u,0x40000000u}};synthetic.floatWords[4]=0x40400000u;const auto testPose=projectedLightJitterPoseCandidate(synthetic,{{0,0,0,0x3f800000u,0x40800000u,0x40a00000u,0x40c00000u}});if(testPose[4]!=0x40800000u||testPose[5]!=0x40000000u||testPose[6]!=0x41400000u)throw std::logic_error("complete jitter known case");
 ProjectedLightSavedFieldsCandidate controls{};controls.colorWord=0xffffffffu;controls.floatWords[26]=0x3f800000u;controls.floatWords[1]=0x42340000u;controls.floatWords[2]=0x42700000u;controls.floatWords[0]=0x7f801234u;
 auto preparedTest=projectedLightPreparedFieldsCandidate(controls,{{0,0,0,0x3f800000u,0,0,0}});
 if(preparedTest.rgbWords!=std::array<uint32_t,3>{{0x3f800000u,0x3f800000u,0x3f800000u}}||preparedTest.controlWords[1]!=0x42700000u||preparedTest.controlWords[2]!=0x42700000u||preparedTest.controlWords[0]!=0x7fc01234u)throw std::logic_error("prepared color/FOV/scalar mismatch");
 controls.floatWords[1]=0x7f801234u;controls.floatWords[2]=0x42340000u;preparedTest=projectedLightPreparedFieldsCandidate(controls,{});if(preparedTest.controlWords[1]!=0x42340000u)throw std::logic_error("unordered horizontal FOV mismatch");
 controls.floatWords[1]=0x42340000u;controls.floatWords[2]=0x7f801234u;preparedTest=projectedLightPreparedFieldsCandidate(controls,{});if(preparedTest.controlWords[1]!=0x7f801234u)throw std::logic_error("unordered vertical FOV mismatch");
 controls.animationTimeWord=10000u;controls.floatWords[10]=0x41c00000u;preparedTest=projectedLightPreparedFieldsCandidate(controls,{});
 if(projectedLightPreparedTextureFrameCandidate(preparedTest,true,7)!=3||projectedLightPreparedTextureFrameCandidate(preparedTest,false,0)!=0)throw std::logic_error("prepared texture frame mismatch");
 controls.animationTimeWord=uint32_t(-10000);preparedTest=projectedLightPreparedFieldsCandidate(controls,{});if(projectedLightPreparedTextureFrameCandidate(preparedTest,true,7)!=-3)throw std::logic_error("negative texture frame mismatch");
 std::cout<<"prepared_light_known_cases=3\n";
 std::ofstream poses("outputs/sfm64-session-light-world-poses.txt");if(!poses)throw std::runtime_error("pose artifact open");
 std::ofstream matrices("outputs/sfm64-session-light-world-matrices.txt");if(!matrices)throw std::runtime_error("world matrix artifact open");
 for(const char* path:{"work/roundtrip-11.dmx","work/roundtrip-139.dmx"}){
  const auto graph=loadGraph(path);
  std::vector<std::vector<size_t>> owners(graph.elements.size());
  for(size_t owner=0;owner<graph.elements.size();++owner)for(const auto& a:graph.elements[owner].attributes)if(graph.strings.at(a.name)=="children"&&a.type==15){
   for(const auto& value:a.values)if(value.reference<owners.size()){
    auto& list=owners[value.reference];if(std::find(list.begin(),list.end(),owner)==list.end())list.push_back(owner);
   }
  }
  const SavedDagWorldMatrixCandidate evaluator(graph);
  unsigned lights=0,ambiguous=0,cycles=0,overrides=0;size_t maximumDepth=0;
  std::cout<<"fixture="<<path<<'\n';
  for(size_t index=0;index<graph.elements.size();++index)if(graph.strings.at(graph.elements[index].type)=="DmeProjectedLight"){
   const auto world=evaluator.evaluate(index);
   const auto pose=matrixOrientationCandidate(world);
   const auto saved=projectedLightSavedFieldsCandidate(graph,index);
   const auto jittered=projectedLightJitterPoseCandidate(saved,pose);
   const auto prepared=projectedLightPreparedFieldsCandidate(saved,jittered);
   if(prepared.quaternionPositionWords!=jittered||prepared.textureName!=saved.textureName||prepared.booleanBytes!=saved.booleanBytes)throw std::logic_error("prepared light data changed");
   preparedOutput<<path<<" light="<<index<<" fov="<<std::hex<<prepared.controlWords[1]<<" rgb=";for(auto word:prepared.rgbWords)preparedOutput<<word<<',';preparedOutput<<" controls=";for(auto word:prepared.controlWords)preparedOutput<<word<<',';preparedOutput<<std::dec<<'\n';
   jitterPoses<<path<<" light="<<index<<" quaternion_position_words=";for(auto word:jittered)jitterPoses<<std::hex<<word<<',';jitterPoses<<std::dec<<'\n';
   poses<<path<<" light="<<index<<" quaternion_position_words=";for(auto word:pose)poses<<std::hex<<word<<',';poses<<std::dec<<'\n';
   matrices<<path<<" light="<<index<<" words=";for(auto word:world)matrices<<std::hex<<word<<',';matrices<<std::dec<<'\n';
   if(lights==0){auto moved=graph;const auto parent=owners[index].at(0);uint32_t transformRef=0xffffffff;
    for(const auto& a:moved.elements[parent].attributes)if(moved.strings.at(a.name)=="transform")transformRef=a.values.at(0).reference;
    auto& transform=moved.elements.at(transformRef);
    for(auto& a:transform.attributes)if(moved.strings.at(a.name)=="position"){const auto value=wireFloat(a.values.at(0));setFloat(a,0,value+10.0f);}
    const SavedDagWorldMatrixCandidate translated(moved);const auto result=translated.evaluate(index);
    float before,after;std::memcpy(&before,&world[3],4);std::memcpy(&after,&result[3],4);
    if(after!=before+10.0f)throw std::logic_error("nonidentity parent translation mismatch");
    std::cout<<"nonidentity_parent_translation_cases=1\n";
   }
   if(lights==0)for(unsigned variant=0;variant<5;++variant){auto malformed=graph;
    if(variant==0){for(auto& e:malformed.elements)for(auto& a:e.attributes)if(malformed.strings.at(a.name)=="children"&&a.type==15)a.values.erase(std::remove_if(a.values.begin(),a.values.end(),[&](const auto& v){return v.reference==index;}),a.values.end());
     Attribute children{};children.name=internString(malformed,"children");children.type=15;Value value{};value.reference=static_cast<uint32_t>(index);children.values.push_back(value);auto& attrs=malformed.elements[index].attributes;attrs.erase(std::remove_if(attrs.begin(),attrs.end(),[&](const auto& a){return malformed.strings.at(a.name)=="children";}),attrs.end());attrs.push_back(children);
    }else if(variant==1){Attribute children{};children.name=internString(malformed,"children");children.type=15;Value value{};value.reference=static_cast<uint32_t>(index);children.values.push_back(value);malformed.elements[index].attributes.push_back(children);
    }else if(variant==2){Attribute a{};a.name=internString(malformed,"overridePos");a.type=4;Value value{};value.wire.push_back(1);a.values.push_back(value);malformed.elements[index].attributes.push_back(a);
    }else if(variant==3){malformed.elements[index].type=malformed.elements[projectedLightSavedFieldsCandidate(graph,index).transformReference].type;
    }else{auto& transform=malformed.elements[projectedLightSavedFieldsCandidate(graph,index).transformReference];for(auto& a:transform.attributes)if(malformed.strings.at(a.name)=="orientation")a.values[0].wire.pop_back();}
    bool rejected=false;try{SavedDagWorldMatrixCandidate candidate(malformed);candidate.evaluate(index);}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::logic_error("invalid world hierarchy accepted");
   }
   ++lights;std::set<size_t> seen;size_t cursor=index,depth=0;
   std::cout<<"light="<<index;
   for(;;){
    if(!seen.insert(cursor).second){++cycles;std::cout<<" cycle="<<cursor;break;}
    const auto& node=graph.elements[cursor];std::cout<<" node="<<cursor<<":"<<graph.strings.at(node.type);
    for(const auto& a:node.attributes){const auto& name=graph.strings.at(a.name);
     if(name=="overridePos"||name=="overrideRot"||name=="overrideParent"){
      ++overrides;std::cout<<" override="<<name<<":type"<<unsigned(a.type);
      if(a.values.size()==1){if(a.type==1)std::cout<<":ref"<<a.values[0].reference;else if(a.values[0].wire.size()==1)std::cout<<":byte"<<unsigned(static_cast<unsigned char>(a.values[0].wire[0]));}
     }
     if(name=="transform"&&a.type==1&&a.values.size()==1&&a.values[0].reference<graph.elements.size()){
      const auto& transform=graph.elements[a.values[0].reference];std::cout<<" transform="<<a.values[0].reference<<":"<<graph.strings.at(transform.type);
      for(const auto& field:transform.attributes)std::cout<<" transform_field="<<graph.strings.at(field.name)<<":type"<<unsigned(field.type);
     }
    }
    if(owners[cursor].empty())break;
    if(owners[cursor].size()!=1){++ambiguous;std::cout<<" ambiguous_owners="<<owners[cursor].size();break;}
    cursor=owners[cursor][0];++depth;
   }
   maximumDepth=std::max(maximumDepth,depth);std::cout<<" depth="<<depth<<'\n';
  }
  std::cout<<"lights="<<lights<<" ambiguous_chains="<<ambiguous<<" cycles="<<cycles<<" override_attributes="<<overrides<<" world_matrices="<<lights<<" rejected_hierarchy_cases=5 maximum_depth="<<maximumDepth<<'\n';
 }
 preparedOutput.flush();if(!preparedOutput)throw std::runtime_error("prepared light artifact write");
 jitterPoses.flush();if(!jitterPoses)throw std::runtime_error("jitter pose artifact write");
 poses.flush();if(!poses)throw std::runtime_error("pose artifact write");
 matrices.flush();if(!matrices)throw std::runtime_error("world matrix artifact write");
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

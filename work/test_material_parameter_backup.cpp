#include "material_parameter_backup_candidate.h"
#include "material_object_store_candidate.h"
#include <iostream>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static bool same(const std::vector<MaterialSessionOverrideInputCandidate>&a,const std::vector<MaterialSessionOverrideInputCandidate>&b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].name!=b[i].name||a[i].nameAvailable!=b[i].nameAvailable||a[i].typeFlags!=b[i].typeFlags||a[i].words!=b[i].words||a[i].textureName!=b[i].textureName)return false;return true;}
static MaterialTypedDispatchCandidate value(MaterialValueConstructionKindCandidate kind,uint32_t bits=0){MaterialTypedDispatchCandidate v;v.name="$value";v.created=materialValueConstructCandidate(v.value,kind,{bits,0,0,0});return v;}
static MaterialSessionOverrideInputCandidate row(const char*name="$value",uint32_t type=5){MaterialSessionOverrideInputCandidate r;r.name=name;r.typeFlags=type;r.words.fill(0x12345678);r.textureName="old";return r;}
int main(){try{
 static_assert(sizeof(void*)==8,"owned x64");unsigned checks=0;auto check=[&](bool b,const char*what){need(b,what);++checks;};
 auto scalar=value(MaterialValueConstructionKindCandidate::Float,0x80000000);std::string out="keep";
 check(materialParameterStringSnapshotCandidate(scalar,out)&&out=="-0.000000","retained signed zero");
 auto notCreated=scalar;notCreated.created=false;out="keep";check(!materialParameterStringSnapshotCandidate(notCreated,out)&&out=="keep","uncreated output retained");
 auto unknown=scalar;unknown.value.numeric.typeAndMetadata=5;check(!materialParameterStringSnapshotCandidate(unknown,out)&&out=="keep","unknown storage refused");
 auto vector=scalar;vector.value.numeric.typeAndMetadata=0x52;check(!materialParameterStringSnapshotCandidate(vector,out)&&out=="keep","bad component extent");
 auto matrix=scalar;matrix.value.numeric.typeAndMetadata=0x47;check(!materialParameterStringSnapshotCandidate(matrix,out)&&out=="keep","matrix storage required");
 auto resource=scalar;resource.name="$resource";resource.value.numeric.typeAndMetadata=0x43;
 check(!materialParameterStringSnapshotCandidate(resource,out)&&out=="keep","missing resource name refused");
 MaterialParameterResourceTextCandidate projection;projection.name="";check(materialParameterStringSnapshotCandidate(resource,out,projection)&&out.empty(),"known empty resource name");
 projection.name=std::string(512,'x');out="keep";check(!materialParameterStringSnapshotCandidate(resource,out,projection)&&out=="keep","resource extent refused");
 projection.name.reset();projection.environmentTexture=true;check(materialParameterStringSnapshotCandidate(resource,out,projection)&&out=="env_cubemap","explicit texture sentinel");
 resource.value.numeric.typeAndMetadata=0x48;out="keep";check(!materialParameterStringSnapshotCandidate(resource,out,projection)&&out=="keep","material is not texture sentinel");
 resource.value.numeric.typeAndMetadata=0x43;
 MaterialParameterCollectionCandidate collection;collection.values={scalar,resource};
 std::vector<MaterialSessionOverrideInputCandidate>rows{row(),row("$resource")};auto before=rows;
 check(!captureMaterialParameterBackupCandidate(rows,collection,0)&&same(rows,before),"late unavailable name rolls back earlier scalar text");
 check(!captureMaterialParameterBackupCandidate(rows,collection,0,[](size_t,const auto&)->MaterialParameterResourceTextCandidate{throw std::runtime_error("service failure");})&&same(rows,before),"throwing name service rolls back");
 check(captureMaterialParameterBackupCandidate(rows,collection,0,[](size_t,const auto&){return MaterialParameterResourceTextCandidate{std::string("texture/name"),false};})&&rows[0].textureName=="-0.000000"&&rows[1].textureName=="texture/name"&&rows[0].words==before[0].words,"ordered snapshot publication");
 auto snapshot=rows;collection.values[0].value.numeric.vector.fill(0x3f800000);check(rows[0].textureName=="-0.000000"&&same(rows,snapshot),"captured text does not alias source");
 for(int phase:{-1,0,1,2,3,4,5})for(uint32_t flags:{0u,0x20u,0x400u}){
  rows={row()};rows[0].typeFlags|=flags;before=rows;auto ordinary=collection;ordinary.values.resize(1);
  check(captureMaterialParameterBackupCandidate(rows,ordinary,phase)&&((phase==0||phase==1||phase==2||(phase==4&&!(flags&0x400)))&&!(flags&0x20)?rows[0].textureName=="1.000000"&&rows[0].typeFlags==(5|flags|0x4080):same(rows,before)),"phase/read-only mutation gate");
 }
 rows={row()};rows[0].textureName="1.000000";before=rows;check(captureMaterialParameterBackupCandidate(rows,collection,0)&&same(rows,before),"equal text keeps flags/bytes");
 auto duplicate=collection.values[0];duplicate.value.numeric.vector.fill(0x40000000);duplicate.name="$VALUE";collection.values.push_back(duplicate);
 rows={row()};check(captureMaterialParameterBackupCandidate(rows,collection,0)&&rows[0].textureName=="2.000000","native reverse case-insensitive FindVar order");
 MaterialParameterBackupServicesCandidate retained(collection);collection.values.clear();auto token=retained.find("$value");check(token.found&&retained.stringValue(token.variable)=="2.000000","source retirement preserves captured parameter snapshot");
 collection.values={value(MaterialValueConstructionKindCandidate::Integer,42)};rows={row("$missing")};before=rows;check(captureMaterialParameterBackupCandidate(rows,collection,0)&&same(rows,before),"missing variable skipped");
 rows={row("$FLAGS"),row("$flags2"),row("plain")};before=rows;check(captureMaterialParameterBackupCandidate(rows,collection,0)&&same(rows,before),"reserved/non-parameter names skipped");
 rows={row("$value",2),row("$value",3),row("$value",14),row()};check(captureMaterialParameterBackupCandidate(rows,collection,0)&&rows[0].words[0]==42&&rows[1].words[0]==0x42280000&&rows[2].words[0]==0x3f800000&&rows[2].words[1]==0&&rows[3].textureName=="42","typed templates retain separate cached getters");
 rows={row()};rows[0].name.assign(4096,'x');before=rows;check(!captureMaterialParameterBackupCandidate(rows,collection,0)&&same(rows,before),"template extent retains output");
 auto huge=collection;huge.values.resize(257);rows={row()};before=rows;check(!captureMaterialParameterBackupCandidate(rows,huge,0)&&same(rows,before),"parameter extent retains output");
 // Exercise the actual owned store path, not an invented parallel string store.
 MaterialObjectStoreCandidate store;auto source=std::make_unique<SourceMaterialPatchNodeCandidate>();source->key="VertexLitGeneric";source->children.push_back({"$alpha",1,".25"});source->children.push_back({"$color",1,"[1 0.5 0]"});
 uintptr_t id=0;check(store.createMaterial("fixture/capture",source,id)&&store.initializeParameters(id),"store constructed and initialized");
 const auto*state=store.get(id)->parameterState.get();need(state,"retained parameter state");const auto oldCount=store.get(id)->parameterCount;const auto oldFlags=store.get(id)->flags;
 // The VMT's alpha is retained as a string variable, with a parsed float cache.
 // GetString must preserve its original .25 spelling, not format that cache.
 rows={row("$alpha"),row("$color")};check(captureMaterialParameterBackupCandidate(rows,state->material.parameters,0)&&rows[0].textureName==".25"&&rows[1].textureName=="[ 1.000000 0.500000 0.000000 ]","real initialized store -> text backup");
 check(store.get(id)->parameterState.get()==state&&store.get(id)->parameterCount==oldCount&&store.get(id)->flags==oldFlags,"store unchanged by backup");
 MaterialParameterBackupServicesCandidate independent(state->material.parameters);check(store.uncacheParameters(id)&&!store.get(id)->parameterState,"store retirement");auto alpha=independent.find("$alpha");check(alpha.found&&independent.stringValue(alpha.variable)==".25","independent snapshot outlives store parameters");
 check(store.decrement(id)&&store.deleteIfUnreferenced(id)&&store.size()==0&&store.statistics().created==store.statistics().destroyed,"store lifecycle balanced");
 std::cout<<"PASS material_parameter_backup_safety_checks="<<checks<<" store_initialized_and_retired=1 native_undo_notifications=NOT_RECONSTRUCTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

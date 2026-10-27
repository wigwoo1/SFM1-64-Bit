#define main preservedParameterBackupSafetyMain
#include "test_material_parameter_backup.cpp"
#undef main
int main(){try{
 need(preservedParameterBackupSafetyMain()==0,"preceding49 safety checks");unsigned checks=0;auto check=[&](bool b){need(b,"cached getter safety");++checks;};
 // Controlled cached projection: requested read count is independent of the
 // stored count. No ordinary vector-constructor claim for this manual state.
 MaterialParameterCollectionCandidate source;auto v=value(MaterialValueConstructionKindCandidate::Float);v.value.numeric.typeAndMetadata=0x12;v.value.numeric.integer=0xdeadbeef;v.value.numeric.vector={0x7fa12345,0xffa54321,0x80000000,0x3f800000};source.values={v};
 MaterialParameterBackupServicesCandidate service(source);check(service.integer(1)==0xdeadbeef);check(service.scalar(1)==0x7fe12345);
 check(service.vector(1,2)==std::array<uint32_t,4>{0x7fe12345,0xffe54321,0,0});check(service.vector(1,3)==std::array<uint32_t,4>{0x7fe12345,0xffe54321,0x80000000,0});check(service.vector(1,4)==std::array<uint32_t,4>{0x7fe12345,0xffe54321,0x80000000,0x3f800000});
 check(source.values[0].value.numeric.vector==v.value.numeric.vector);
 for(uint32_t count:{0u,1u,5u}){bool refused=false;try{service.vector(1,count);}catch(const std::runtime_error&){refused=true;}check(refused);}
 for(uintptr_t token:{uintptr_t(0),uintptr_t(2)}){bool refused=false;try{service.integer(token);}catch(const std::runtime_error&){refused=true;}check(refused);}
 MaterialMatrixMathCandidate matrix{};matrix.fill(0x7fa12345);need(materialTypedMatrixSetCandidate(v,matrix),"matrix fixture");source.values={v};MaterialParameterBackupServicesCandidate matrixService(source);check(matrixService.matrix(1)==matrix);
 auto rows=std::vector<MaterialSessionOverrideInputCandidate>{row("$value",14)};rows[0].words=matrix;const auto before=rows;check(captureMaterialParameterBackupCandidate(rows,source,0)&&same(rows,before));
 auto resource=value(MaterialValueConstructionKindCandidate::Undefined);resource.name="$resource";resource.value.numeric.typeAndMetadata=0x43;source.values={value(MaterialValueConstructionKindCandidate::Integer,42),resource};
 for(uint32_t type:{2u,3u,8u,9u,10u,11u,14u}){rows={row("$value",2),row("$resource",type)};auto initial=rows;check(!captureMaterialParameterBackupCandidate(rows,source,0)&&same(rows,initial));}
 std::cout<<"PASS cached_backup_safety_checks="<<checks<<" plus_prior=49 raw_signaling_cache_preserved=1 matrix_getter_not_quieted=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

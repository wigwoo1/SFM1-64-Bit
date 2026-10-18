#include <iostream>
#include <vector>
#include "studio_dynamic_mesh_submit_candidate.h"
struct Ops {
 StudioShaderInstanceStateCandidate state;std::array<uint32_t,16> matrix;
 std::array<float,4> modulation{0.5f,-2.f,4.f,0.f};bool supplied=false,compressed=false;uint32_t meshCount=0;
 std::vector<unsigned> order;unsigned submitted=0;uint32_t set=0;
 uintptr_t mesh=uintptr_t(0x123456789000ull),mat=uintptr_t(0xabcdef123000ull);
 StudioShaderInstanceResultCandidate instance(){order.push_back(1);return studioShaderInstanceStateFromMatrixCandidate(state,matrix);}
 void offsets(uintptr_t receiver,uint32_t& a,uint32_t& b){if(receiver!=mesh)throw 1;order.push_back(2);a=0xfffffffe;b=37;}
 uintptr_t material(){order.push_back(3);modulation.fill(19.f);return mat;}
 void rgb(uintptr_t receiver,std::array<float,3>& color){if(receiver!=mat)throw 2;order.push_back(4);color={2.f,0.25f,-0.5f};}
 float alpha(uintptr_t receiver){if(receiver!=mat)throw 3;order.push_back(5);return 0.75f;}
 uint32_t count(uintptr_t receiver){if(receiver!=mesh)throw 4;order.push_back(6);return meshCount;}
 void setCount(uint32_t value){order.push_back(7);set=value;}
 std::array<uint32_t,2> format(uintptr_t receiver){if(receiver!=mesh)throw 5;order.push_back(8);return {compressed?0x400u:0u,0xffffffffu};}
 void submit(uintptr_t receiver,uint32_t count,const StudioShaderInstanceCandidate& record,bool compression,uintptr_t a,uintptr_t b){
  order.push_back(9);if(receiver!=mesh||count!=1||compression!=compressed||a!=state.metadata1||b!=state.metadata2||set!=meshCount)throw 6;
  if(record.matrices!=reinterpret_cast<uintptr_t>(state.currentMatrix.data())||record.lighting!=state.lighting||record.scalars[11]!=0xfffffffe||record.scalars[12]!=37)throw 7;
  std::array<float,4> expected=supplied?std::array<float,4>{1.f,-0.5f,-2.f,0.f}:std::array<float,4>{2.f,0.25f,-0.5f,0.75f};
  for(unsigned i=0;i<4;++i){uint32_t bits;memcpy(&bits,&expected[i],4);if(record.scalars[15+i]!=bits)throw 8;}
  if(record.scalars[2]!=(state.matrixBatchEnabled&&meshCount?meshCount+1:1))throw 9;
  ++submitted;
 }
};
int main(){unsigned cases=0;
 for(unsigned i=0;i<256;++i)for(bool supplied:{false,true})for(bool compressed:{false,true})for(bool batch:{false,true}){
  Ops ops;ops.supplied=supplied;ops.compressed=compressed;ops.meshCount=i==255?0xffffffffu:i;
  ops.state.pendingMatrices=ops.meshCount;ops.state.matrixBatchEnabled=batch;ops.state.metadata1=ops.mat+8;ops.state.metadata2=ops.mat+6;ops.state.lighting=ops.mat+128;
  for(unsigned j=0;j<16;++j)ops.matrix[j]=i*17+j;
  studioDynamicMeshSubmitCandidate(ops.mesh,supplied?&ops.modulation:nullptr,ops);
  if(ops.order!=std::vector<unsigned>{1,2,3,4,5,6,7,8,9}||ops.submitted!=1)return 1;
  ++cases;
 }
 std::cout<<"dynamic_mesh_submit_cases="<<cases<<" captured_modulation=1 matrix_instance_integration=1 full_width_pointers=1 callback_order=1\n";
}

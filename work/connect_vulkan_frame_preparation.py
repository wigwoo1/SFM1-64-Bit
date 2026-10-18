from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include <charconv>','#include <charconv>\n#include <future>')
start=s.index(' if(sequencePathSize){auto directory=',s.index(' for(size_t frame='))
finish=s.index('\n if(frame)check',start)
old=s[start:finish]
prepare=old[old.index('auto directory='):old.index('void*vertexMapped=')]
prepare=prepare.replace('frameVertices.size()!=vertices.size()','frameVertices.size()!=expectedVertexCount')
prepare=prepare.replace('for(size_t n=0;n<vertices.size();++n)', 'result.vertices.resize(expectedVertexCount);for(size_t n=0;n<expectedVertexCount;++n)')
prepare=prepare.replace('vertices[n]', 'result.vertices[n]')
prepare=prepare.replace('camera=readRenderCameraFileCandidate','result.camera=readRenderCameraFileCandidate')
prepare=prepare.replace('transparentOrder=translucentRangeOrderCandidate(renderInputs,vertices,', 'result.transparentOrder=translucentRangeOrderCandidate(renderInputs,result.vertices,')
prepare=prepare.replace('renderCameraSortDistanceCandidate(camera,v.position)', 'renderCameraSortDistanceCandidate(result.camera,v.position)')
helper='''
 struct PreparedDrawFrameCandidate {std::vector<Vertex>vertices;RenderCameraCandidate camera;std::vector<size_t>transparentOrder;};
 const size_t expectedVertexCount=vertices.size();
 auto prepareFrame=[&](size_t frame){PreparedDrawFrameCandidate result;'''+prepare+'''return result;};
 std::future<PreparedDrawFrameCandidate>nextPreparedFrame;size_t preparationTasks=0;
 bool prefetchFrames=true;char prefetchText[8]{};auto prefetchSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_FRAME_PREFETCH",prefetchText,sizeof(prefetchText));if(prefetchSize){if(prefetchSize!=1||(prefetchText[0]!='0'&&prefetchText[0]!='1'))throw std::runtime_error("invalid frame prefetch mode");prefetchFrames=prefetchText[0]=='1';}
'''
replacement=''' if(sequencePathSize){auto prepared=nextPreparedFrame.valid()?nextPreparedFrame.get():prepareFrame(frame);vertices=std::move(prepared.vertices);camera=prepared.camera;transparentOrder=std::move(prepared.transparentOrder);void*vertexMapped=nullptr;check(map(dev,vertexMemory,0,vertices.size()*sizeof(Vertex),0,&vertexMapped));std::memcpy(vertexMapped,vertices.data(),vertices.size()*sizeof(Vertex));unmap(dev,vertexMemory);}'''
s=s[:start]+replacement+s[finish:]
marker=' VkPhysicalDeviceMemoryProperties properties{};'
assert marker in s
s=s.replace(marker,helper+marker)
s=s.replace('auto cleanup=[&](){if(pending)', 'auto cleanup=[&](){if(nextPreparedFrame.valid())nextPreparedFrame.wait();if(pending)')
marker='pending=true;check(waitFences('
assert marker in s
s=s.replace(marker,'pending=true;if(sequencePathSize&&prefetchFrames&&frame+1<frameCount){nextPreparedFrame=std::async(std::launch::async,prepareFrame,frame+1);++preparationTasks;}check(waitFences(')
s=s.replace(' queue_idle_waits=0 native_full_parity=unverified',' queue_idle_waits=0 preparation_tasks="<<preparationTasks<<" max_prepared_ahead=1 native_full_parity=unverified')
p.write_text(s)

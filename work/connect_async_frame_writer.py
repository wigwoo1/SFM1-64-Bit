from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text().replace('#include "geometry_deformation_worker_pool.h"','#include "geometry_deformation_worker_pool.h"\n#include "bounded_frame_writer_candidate.h"')
marker=' VkPhysicalDeviceMemoryProperties properties{};'
assert marker in s
s=s.replace(marker,''' bool asyncOutput=true;char outputModeText[8]{};auto outputModeSize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_ASYNC_OUTPUT",outputModeText,sizeof(outputModeText));if(outputModeSize){if(outputModeSize!=1||(outputModeText[0]!='0'&&outputModeText[0]!='1'))throw std::runtime_error("invalid async output mode");asyncOutput=outputModeText[0]=='1';}std::unique_ptr<BoundedFrameWriterCandidate>frameWriter;if(sequencePathSize&&asyncOutput)frameWriter=std::make_unique<BoundedFrameWriterCandidate>();size_t asyncWrites=0;
'''+marker)
start=s.index('std::ofstream output(sequencePathSize?');end=s.index('std::cout<<"Vulkan indexed model draw passed',start)
old=s[start:end];path=old[len('std::ofstream output('):old.index(',std::ios::binary);')]
s=s[:start]+'auto outputPath='+path+';if(frameWriter){frameWriter->submit(outputPath,std::move(rgba));++asyncWrites;}else BoundedFrameWriterCandidate::write(outputPath,rgba);'+s[end:]
s=s.replace('}if(sequencePathSize)std::cout<<"retained Vulkan sequence passed', '}if(frameWriter)frameWriter->flush();if(sequencePathSize)std::cout<<"retained Vulkan sequence passed')
s=s.replace(' native_full_parity=unverified\\n";cleanup();', ' async_output_writes="<<asyncWrites<<" pending_output_bound=1 native_full_parity=unverified\\n";cleanup();')
p.write_text(s)

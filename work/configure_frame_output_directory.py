from pathlib import Path
p=Path('work/vulkan_model_draw_candidate.h');s=p.read_text()
marker=' VkPhysicalDeviceMemoryProperties properties{};'
s=s.replace(marker,' char outputDirectoryText[32768]{};auto outputDirectorySize=GetEnvironmentVariableA("SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY",outputDirectoryText,sizeof(outputDirectoryText));if(outputDirectorySize>=sizeof(outputDirectoryText))throw std::runtime_error("output directory length");std::filesystem::path outputDirectory=outputDirectorySize?outputDirectoryText:"outputs";if(!std::filesystem::is_directory(outputDirectory))throw std::runtime_error("output directory must exist");\n'+marker)
start=s.index('auto outputPath=');end=s.index(';if(frameWriter)',start)
expression=s[start+len('auto outputPath='):end].replace('outputs/','')
s=s[:start]+'auto outputPath=outputDirectory/('+expression+');'+s[end+1:]
p.write_text(s)
